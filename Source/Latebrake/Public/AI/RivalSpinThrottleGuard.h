#pragma once

#include "AI/RivalPilot.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Last-stage propulsion safety policy for the physically simulated AI rival.
// Measured Chaos yaw and planar sideways speed are required; this does not
// steer, brake, teleport, modify route progress or control the human player.
struct RivalSpinObservation final
{
    double FrameSeconds = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double LateralSpeedCmPerSecond = 0.0;
    double WorldYawRadiansPerSecond = 0.0;
    bool PhysicsReady = false;
};

class RivalSpinThrottleGuard final
{
public:
    static constexpr double MaximumFrameSeconds = 0.10;
    static constexpr double MaximumSpeedCmPerSecond = 10000.0;
    static constexpr double MaximumYawRadiansPerSecond = 10.0;
    static constexpr double HoldSeconds = 0.16;

    void Reset() noexcept
    {
        RemainingSeconds = 0.0;
        HeldThrottleCeiling = 1.0;
    }

    double RemainingHoldSeconds() const noexcept { return RemainingSeconds; }

    PilotCommand Apply(PilotCommand command,
                       const RivalSpinObservation& sample) noexcept
    {
        if (!sample.PhysicsReady ||
            !std::isfinite(sample.FrameSeconds) ||
            sample.FrameSeconds <= 0.0 ||
            sample.FrameSeconds > MaximumFrameSeconds ||
            !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
            sample.PlanarSpeedCmPerSecond < 0.0 ||
            sample.PlanarSpeedCmPerSecond > MaximumSpeedCmPerSecond ||
            !std::isfinite(sample.LateralSpeedCmPerSecond) ||
            std::abs(sample.LateralSpeedCmPerSecond) >
                sample.PlanarSpeedCmPerSecond + 5.0 ||
            !std::isfinite(sample.WorldYawRadiansPerSecond) ||
            std::abs(sample.WorldYawRadiansPerSecond) >
                MaximumYawRadiansPerSecond ||
            !std::isfinite(command.Throttle) ||
            command.Throttle < 0.0 || command.Throttle > 1.0 ||
            !std::isfinite(command.Steering) ||
            std::abs(command.Steering) > 1.0)
        {
            Reset();
            return {}; // malformed physics may not authorize motor power
        }
        if (command.Arrived)
        {
            Reset();
            return command;
        }
        if (command.Brake)
        {
            // Never replace stronger corner, traffic or emergency braking.
            return command;
        }

        const double speed = sample.PlanarSpeedCmPerSecond;
        const double yaw = std::abs(sample.WorldYawRadiansPerSecond);
        const double slip = std::clamp(
            std::abs(sample.LateralSpeedCmPerSecond) /
                std::max(500.0, speed), 0.0, 1.0);
        const bool sliding = speed >= 1250.0 && yaw >= 1.05 && slip >= 0.22;
        if (sliding)
        {
            // Cut propulsion, not steering or service brakes; braking a
            // fast-spinning chassis can further destabilize an arcade car.
            const double ceiling = yaw >= 1.90 && slip >= 0.38
                ? 0.0 : 0.30;
            HeldThrottleCeiling = ceiling;
            RemainingSeconds = HoldSeconds;
        }
        else if (RemainingSeconds > 0.0)
        {
            RemainingSeconds = std::max(0.0,
                RemainingSeconds - sample.FrameSeconds);
            if (RemainingSeconds == 0.0)
            {
                HeldThrottleCeiling = 1.0;
            }
        }
        command.Throttle = std::min(command.Throttle, HeldThrottleCeiling);
        return command;
    }

private:
    double RemainingSeconds = 0.0;
    double HeldThrottleCeiling = 1.0;
};
} // namespace latebrake
