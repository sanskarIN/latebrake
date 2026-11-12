#include "Racing/RacePitService.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
void Require(bool ok, const char* reason)
{
    if (!ok) throw std::runtime_error(reason);
}
}

int main()
{
    using latebrake::PitServiceGuidance;
    using latebrake::PitServiceObservation;
    using latebrake::DiagnosePitService;
    using latebrake::RacePitService;

    PitServiceObservation sample{0.05, 0.0, -1300.0, 0.0, 0.0,
                                 true, false, true, true, true};
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::Servicing,
            "valid held physical pit input reports servicing");
    Require(DiagnosePitService(sample, false) ==
                PitServiceGuidance::ExitToRearm,
            "completed bay cannot accept second service without exit");

    sample.Authorized = false;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::Unavailable,
            "countdown, pause or invalid driver is unavailable");
    sample.Authorized = true;
    sample.DeltaSeconds = 0.2;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::Unavailable,
            "large frame never reports eligible service");
    sample.DeltaSeconds = 0.05;
    sample.X = std::numeric_limits<double>::quiet_NaN();
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::Unavailable,
            "invalid physics coordinates fail closed");
    sample.X = 0.0;
    sample.Y = -1800.0;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::ApproachBay,
            "physical racing line is not the pit bay");
    sample.Y = -1300.0;
    sample.MaintenanceNeeded = false;
    sample.PlanarSpeedCmPerSecond = 900.0;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::NoMaintenance,
            "fully healthy car gets honest no-maintenance reason");
    sample.MaintenanceNeeded = true;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::SlowDown,
            "too-fast car must brake before service");
    sample.PlanarSpeedCmPerSecond = 120.0;
    sample.RequestedThrottle = 0.20;
    sample.NitroHeld = true;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::ReleaseThrottle,
            "throttle outranks nitro when both controls are held");
    sample.RequestedThrottle = 0.0;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::ReleaseNitro,
            "nitro prevents pit service even with held PIT button");
    sample.NitroHeld = false;
    sample.ButtonHeld = false;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::HoldButton,
            "eligible stopped car receives instruction to hold");
    sample.ButtonHeld = true;
    sample.StableGround = false;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::SettleChassis,
            "partial ground contact and bouncing get specific guidance");
    sample.StableGround = true;
    Require(DiagnosePitService(sample, true) ==
                PitServiceGuidance::Servicing,
            "valid PIT hold reports progress-ready state");

    RacePitService pit;
    Require(pit.Guidance() == PitServiceGuidance::Unavailable,
            "no stale status before first accepted observation");
    for (int frame = 0; frame < 49; ++frame)
    {
        Require(!pit.Observe(sample), "no early pit repair");
        Require(pit.Guidance() == PitServiceGuidance::Servicing,
                "partial frame reports true held progress");
    }
    Require(pit.Observe(sample), "2.5 seconds completes physical service");
    Require(pit.Guidance() == PitServiceGuidance::Completed,
            "one completed service has explicit result status");
    Require(!pit.Observe(sample) &&
                pit.Guidance() == PitServiceGuidance::ExitToRearm,
            "same bay cannot award another service");
    sample.X = 500.0;
    Require(!pit.Observe(sample) && pit.IsArmed() &&
                pit.Guidance() == PitServiceGuidance::ApproachBay,
            "leaving the bay rearms and updates guidance");
    sample.X = 0.0;
    sample.PlanarSpeedCmPerSecond = 130.0;
    Require(!pit.Observe(sample) &&
                pit.Guidance() == PitServiceGuidance::SlowDown &&
                pit.ProgressFraction() == 0.0,
            "slow-down warning never accumulates false service time");
    sample.PlanarSpeedCmPerSecond = 120.0;
    Require(!pit.Observe(sample) &&
                pit.Guidance() == PitServiceGuidance::Servicing,
            "exact 120 cm/s threshold is valid and begins hold");
    sample.StableGround = false;
    Require(!pit.Observe(sample) &&
                pit.Guidance() == PitServiceGuidance::SettleChassis &&
                pit.IsInBay() && pit.ProgressFraction() == 0.0,
            "unstable physical contact resets accrued service progress");
    sample.StableGround = true;
    Require(!pit.Observe(sample) &&
                pit.Guidance() == PitServiceGuidance::Servicing &&
                pit.ProgressFraction() > 0.0 &&
                pit.ProgressFraction() < 0.03,
            "settled chassis must restart the hold from zero");
    sample.Authorized = false;
    Require(!pit.Observe(sample) &&
                pit.Guidance() == PitServiceGuidance::Unavailable &&
                pit.ProgressFraction() == 0.0,
            "interrupted race never preserves progress or guidance");
    sample.Authorized = true;
    Require(!pit.Observe(sample) &&
                pit.Guidance() == PitServiceGuidance::Servicing,
            "new authorized hold starts from zero");
    pit.Suspend();
    Require(pit.Guidance() == PitServiceGuidance::Unavailable &&
                pit.ProgressFraction() == 0.0,
            "OS focus loss removes stale HUD guidance and hold");
    pit.Reset();
    Require(pit.IsArmed() && pit.StopCount() == 0 &&
                pit.Guidance() == PitServiceGuidance::Unavailable,
            "new race resets both service history and guidance");

    std::cout << "RacePitGuidance: accurate blockers and safe lifecycle\n";
    return 0;
}
