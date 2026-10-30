#pragma once

#include "AI/RivalPilot.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// A source-level corner pace limiter for the real autonomous rival.
// This estimates risk from the rival's previous measured tire condition,
// steering demand and measured lateral chassis slip. It is not per-wheel
// traction control, a race overtaking planner, or an authority over laps.
struct RivalGripPaceSample final
{
    double FrameSeconds = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double LateralSpeedCmPerSecond = 0.0;
    double TireGripMultiplier = 1.0;
    bool RealPhysicsSampleReady = false;
};

class RivalGripPace final
{
public:
    static constexpr double MaximumFrameSeconds = 0.10;
    static constexpr double MaximumMeasuredSpeed = 10000.0;
    static constexpr double MinimumSampleGrip = 0.68;
    static constexpr double BrakeHysteresisSeconds = 0.12;

    void Reset() noexcept
    {
        BrakeHoldSeconds = 0.0;
        LastQualifiedTargetSpeed = 0.0;
        LastGripLimited = false;
    }

    // An interruption or teleport must never preserve the previous
    // braking hold into a fresh race/physics session.
    PilotCommand Apply(PilotCommand command,
                       const RivalGripPaceSample& sample) noexcept
    {
        const double dt = sample.FrameSeconds;
        const double speed = sample.PlanarSpeedCmPerSecond;
        const double lateral = sample.LateralSpeedCmPerSecond;
        const double grip = sample.TireGripMultiplier;
        if (!sample.RealPhysicsSampleReady ||
            !std::isfinite(dt) || dt <= 0.0 ||
            dt > MaximumFrameSeconds ||
            !std::isfinite(speed) || speed < 0.0 ||
            speed > MaximumMeasuredSpeed ||
            !std::isfinite(lateral) ||
            std::abs(lateral) > speed + 5.0 ||
            !std::isfinite(grip) || grip < MinimumSampleGrip ||
            grip > 1.0 ||
            !std::isfinite(command.Throttle) ||
            command.Throttle < 0.0 || command.Throttle > 1.0 ||
            !std::isfinite(command.Steering) ||
            std::abs(command.Steering) > 1.0)
        {
            Reset();
            return {}; // fail closed: no motor acceleration, safe brake
        }
        if (command.Arrived || command.Brake)
        {
            Reset();
            return command; // checkpoint arrival and prior brake win
        }

        LastGripLimited = false;
        LastQualifiedTargetSpeed = 0.0;
        const double steering = std::abs(command.Steering);
        const double slip = std::clamp(
            std::abs(lateral) / std::max(500.0, speed), 0.0, 1.0);
        const double gripLoss = 1.0 - grip;
        const bool cornerRisk =
            speed >= 1100.0 && steering >= 0.22 &&
            (gripLoss >= 0.03 || slip >= 0.12);
        if (cornerRisk)
        {
            // Always bounded and never the authority for steering or
            // official route timing. Lower available grip and greater
            // measured sideways motion lower the cornering pace.
            const double target = std::clamp(
                3600.0 - 1650.0 * steering -
                1500.0 * gripLoss - 950.0 * slip,
                1150.0, 3600.0);
            LastQualifiedTargetSpeed = target;
            LastGripLimited = true;
            if (speed > target + 150.0)
            {
                BrakeHoldSeconds = BrakeHysteresisSeconds;
                command.Throttle = 0.0;
                command.Brake = true;
                return command;
            }
        }

        if (BrakeHoldSeconds > 0.0)
        {
            BrakeHoldSeconds = std::max(0.0, BrakeHoldSeconds - dt);
            if (BrakeHoldSeconds > 0.0)
            {
                command.Throttle = 0.0;
                command.Brake = true;
                return command;
            }
        }
        if (cornerRisk)
        {
            // Allow progress but never add throttle or alter steering.
            const double permitted = std::clamp(
                0.85 - 0.40 * steering - 0.30 * gripLoss -
                0.25 * slip, 0.18, 0.85);
            command.Throttle = std::min(command.Throttle, permitted);
        }
        return command;
    }

    double RemainingHoldSeconds() const noexcept { return BrakeHoldSeconds; }
    double LastTargetSpeed() const noexcept { return LastQualifiedTargetSpeed; }
    bool WasGripLimited() const noexcept { return LastGripLimited; }

private:
    double BrakeHoldSeconds = 0.0;
    double LastQualifiedTargetSpeed = 0.0;
    bool LastGripLimited = false;
};
} // namespace latebrake
