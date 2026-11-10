#include "AI/RivalRecoveryOccupancy.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* reason)
{
    if (!pass)
    {
        std::cerr << "RivalRecoveryOccupancy FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    static_assert(MaxRecoveryObstacles == 5);
    std::array<RecoveryObstacle, MaxRecoveryObstacles> observed{};
    observed[0] = {true, 0, 101,
        {{10000.0, 10000.0}, 0.0}, 0.0, 0.0};
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPose recovery = RivalRecovery(gate);
        Require(RecoveryChassisFitsPhysicalAsphalt(recovery),
                "all genuine four rival recovery poses fit a complete slab");
        Require(EvaluateRivalRecoveryClearance(
            recovery, observed, 1).Safe(),
            "valid distant physical player cannot block recovery");
    }

    const CircuitPose target = RivalRecovery(0);
    observed[0].Pose = target;
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 1).Reason ==
            RecoveryClearanceReason::TrafficCollision,
        "overlapping actual human chassis blocks rival teleport");
    observed[0].Pose.Position.X = target.Position.X + 1200.0;
    observed[0].VelocityX = -4000.0;
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 1).Reason ==
            RecoveryClearanceReason::TrafficCollision,
        "fast approaching player blocks future swept landing before impact");
    observed[0].VelocityX = 4000.0;
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 1).Safe(),
        "measured motion away from the candidate does not block recovery");
    observed[0].VelocityX = 0.0;
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 1).Safe(),
        "stationary distant player leaves the landing available");

    // Swept SAT captures fast cross-throughs between synthetic 30/120 Hz
    // frame samples. A fixed snapshot at 0 or 0.4 seconds would miss this.
    observed[0].Pose.Position.X = target.Position.X - 1400.0;
    observed[0].VelocityX = 7000.0;
    Require(PredictedRecoveryFootprintsOverlap(
        target, observed[0].Pose, observed[0].VelocityX, 0.0),
        "continuous path through proposed landing is rejected");
    observed[0].VelocityX = -7000.0;
    Require(!PredictedRecoveryFootprintsOverlap(
        target, observed[0].Pose, observed[0].VelocityX, 0.0),
        "moving away preserves non-overlap");

    Require(EvaluateRivalRecoveryClearance(
        target, observed, 0).Reason ==
            RecoveryClearanceReason::UnknownOccupancy &&
            EvaluateRivalRecoveryClearance(
                target, observed, 6).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
        "missing and oversized physical-observation sets fail closed");

    observed[0].VelocityX = 0.0;
    observed[0].ActorToken = 0;
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 1).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
        "fake anonymous physical obstacle cannot authorize movement");
    observed[0].ActorToken = 101;
    observed[0].VelocityY = std::numeric_limits<double>::infinity();
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 1).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
        "invalid measured player velocity blocks teleport");
    observed[0].VelocityY = 0.0;
    observed[0].Verified = false;
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 1).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
        "unverified physical sample cannot authorize recovery");
    observed[0].Verified = true;

    auto corrupt = target;
    corrupt.Position.X = std::numeric_limits<double>::quiet_NaN();
    Require(EvaluateRivalRecoveryClearance(
        corrupt, observed, 1).Reason ==
            RecoveryClearanceReason::InvalidTarget,
        "nonfinite recovery target rejected");
    corrupt = target;
    corrupt.Position = {0.0, 0.0};
    Require(EvaluateRivalRecoveryClearance(
        corrupt, observed, 1).Reason ==
            RecoveryClearanceReason::OffPavement,
        "centre dirt rejects entire recovery footprint");
    corrupt = target;
    corrupt.Position.Y = SouthY + 720.0;
    Require(IsOnPavedSurface(corrupt.Position) &&
            EvaluateRivalRecoveryClearance(
                corrupt, observed, 1).Reason ==
                    RecoveryClearanceReason::OffPavement,
        "paved center with overhanging wheels cannot be a landing");

    observed[0] = {true, 0, 101,
        {{10000.0, 10000.0}, 0.0}, 0.0, 0.0};
    observed[1] = {true, 2, 202, {{8000.0, 8000.0}, 0.0}, 0.0, 0.0};
    observed[2] = {true, 3, 303, {{7000.0, 7000.0}, 0.0}, 0.0, 0.0};
    observed[3] = {true, 4, 404, {{6000.0, 6000.0}, 0.0}, 0.0, 0.0};
    observed[4] = {true, 5, 505, {{5000.0, 5000.0}, 0.0}, 0.0, 0.0};
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 5).Safe(),
        "five individually observed nonblocking physical actors admitted");
    observed[4].ActorToken = 202;
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 5).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
        "duplicate actor identity cannot masquerade as separate rival");
    observed[4].ActorToken = 505;
    observed[4].Slot = 2;
    Require(EvaluateRivalRecoveryClearance(
        target, observed, 5).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
        "duplicate slot identity cannot masquerade as separate obstacle");
    observed[4].Slot = 5;
    observed[4].Pose = target;
    const RecoveryClearanceResult blocked =
        EvaluateRivalRecoveryClearance(target, observed, 5);
    Require(blocked.Reason == RecoveryClearanceReason::TrafficCollision &&
            blocked.ObstacleSlot == 5,
            "any future real rival can veto overlapping recovery");

    for (int degree = 0; degree < 3600; ++degree)
    {
        const double heading = degree * 0.1;
        const CircuitPose rotated{target.Position, heading};
        Require(PredictedRecoveryFootprintsOverlap(
            rotated, target, 0.0, 0.0),
            "all orientations reject coincident occupied candidate");
        const CircuitPose far{{10000.0, 10000.0}, heading};
        Require(!PredictedRecoveryFootprintsOverlap(
            rotated, far, 0.0, 0.0),
            "all orientations distinguish distant vehicles");
    }

    std::cout << "Swept recovery occupant clearance contracts passed.\n";
    return EXIT_SUCCESS;
}
