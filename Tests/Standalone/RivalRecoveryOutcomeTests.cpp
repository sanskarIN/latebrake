#include "AI/RivalRouteProgress.h"
#include "Racing/RecoveryTeleportVerification.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool passed, const char* label)
{
    if (!passed)
    {
        std::cerr << "Rival recovery outcome FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::RecoveryTeleportObservation SoundRecovery()
{
    latebrake::RecoveryTeleportObservation sample{
        true, 2600.0, -1500.0, 180.0, 90.0,
        2600.0, -1500.0, 180.0, 90.0
    };
    sample.ChassisReadyAfterMove = true;
    return sample;
}
}

int main()
{
    using namespace latebrake;
    RivalRouteProgress route(4, 3);
    Require(!route.HasPendingRecovery(), "new route has no pending teleport");
    Require(!route.ReportRecoveryResult(true),
            "cannot credit unsolicited positive engine result");

    Require(route.Observe(2600.0, 0.0, 8.0).NeedsRecovery &&
                route.GetRecoveryCount() == 1 && route.HasPendingRecovery(),
            "stalled rival reserves one recovery attempt");
    Require(!route.Observe(0.0, 300.0, 1.0).Advanced &&
                route.GetNextWaypoint() == 0,
            "while the teleport outcome is unresolved route cannot advance");
    Require(route.ReportRecoveryResult(
                IsRecoveryTeleportVerified(SoundRecovery())) &&
                route.GetSuccessfulRecoveryCount() == 1 &&
                route.GetFailedRecoveryCount() == 0 &&
                route.GetRecoveryCount() == 1 &&
                !route.HasPendingRecovery(),
            "real production verifier supplies successful recovery result");
    Require(!route.ReportRecoveryResult(false) &&
                route.GetFailedRecoveryCount() == 0,
            "a late reject cannot overwrite acknowledged verified placement");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    auto failed = SoundRecovery();
    failed.ActualY += 25.0;
    Require(EvaluateRecoveryTeleport(failed) ==
                RecoveryTeleportDecision::PositionMismatch,
            "position error is genuinely rejected by production verifier");
    Require(route.Observe(2600.0, 0.0, 8.0).NeedsRecovery &&
                route.GetRecoveryCount() == 2,
            "stalled rival may retry after a fresh interval");
    Require(route.ReportRecoveryResult(IsRecoveryTeleportVerified(failed)) &&
                route.GetSuccessfulRecoveryCount() == 1 &&
                route.GetFailedRecoveryCount() == 1 &&
                route.GetRecoveryCount() == 2,
            "failed engine placement increments rejected not verified count");

    failed = SoundRecovery();
    failed.ChassisReadyAfterMove = false;
    Require(route.Observe(2600.0, 0.0, 8.0).NeedsRecovery &&
                route.ReportRecoveryResult(IsRecoveryTeleportVerified(failed)) &&
                route.GetFailedRecoveryCount() == 2,
            "physically unusable chassis does not count as recovered");

    failed = SoundRecovery();
    failed.PostAngularVelocityZDegrees = 2.0;
    Require(route.Observe(2600.0, 0.0, 8.0).NeedsRecovery &&
                route.ReportRecoveryResult(IsRecoveryTeleportVerified(failed)) &&
                route.GetFailedRecoveryCount() == 3,
            "unstable post-move rotation is not successful recovery");

    failed = SoundRecovery();
    failed.ActualRollDegrees = nan;
    Require(route.Observe(2600.0, 0.0, 8.0).NeedsRecovery &&
                route.ReportRecoveryResult(IsRecoveryTeleportVerified(failed)) &&
                route.GetFailedRecoveryCount() == 4,
            "nonfinite observed rotation cannot be counted as success");

    Require(route.GetRecoveryCount() ==
                route.GetSuccessfulRecoveryCount() + route.GetFailedRecoveryCount(),
            "all synchronous attempts accounted once with no unverified free moves");
    route.Reset();
    Require(!route.HasPendingRecovery() &&
                route.GetRecoveryCount() == 0 &&
                route.GetFailedRecoveryCount() == 0 &&
                route.GetSuccessfulRecoveryCount() == 0,
            "restart resets recovery result audit");
    std::cout << "Rival recovery outcome tests passed (no Unreal).\n";
    return EXIT_SUCCESS;
}
