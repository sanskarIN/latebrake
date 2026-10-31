#pragma once

#include "AI/RivalPilot.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Prototype yaw-rate feedback for the real Chaos-controlled rival.
// The existing waypoint planner remains responsible for desired steering;
// this policy only backs off already-requested steering when the chassis
// is rotating the SAME direction quickly. No route, brake, throttle,
// scoring or real tire/suspension authority is added.
struct RivalYawRateSample final
{
    double DeltaSeconds = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double WorldYawRadiansPerSecond = 0.0;
    bool PhysicsReady = false;
};

inline PilotCommand ModerateRivalYawRate(
    PilotCommand command, RivalYawRateSample sample) noexcept
{
    if (!sample.PhysicsReady ||
        !std::isfinite(sample.DeltaSeconds) ||
        sample.DeltaSeconds <= 0.0 || sample.DeltaSeconds > 0.10 ||
        !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
        sample.PlanarSpeedCmPerSecond < 0.0 ||
        sample.PlanarSpeedCmPerSecond > 10000.0 ||
        !std::isfinite(sample.WorldYawRadiansPerSecond) ||
        std::abs(sample.WorldYawRadiansPerSecond) > 10.0 ||
        !std::isfinite(command.Throttle) ||
        command.Throttle < 0.0 || command.Throttle > 1.0 ||
        !std::isfinite(command.Steering) ||
        std::abs(command.Steering) > 1.0)
    {
        // Invalid Chaos motion must not authorize another AI force tick.
        return {};
    }
    if (command.Arrived || command.Steering == 0.0)
    {
        return command;
    }

    const double yawRate = sample.WorldYawRadiansPerSecond;
    // Never weaken a requested correction against an existing spin.
    // Likewise, no new countersteer torque is invented in neutral.
    if (yawRate * command.Steering <= 0.0)
    {
        return command;
    }

    // No yaw damping at launch or in slow maneuvers. As the actual
    // planar velocity and same-direction yaw both rise, remove at most
    // 65% of the request. Keep >=35% directional steering authority.
    constexpr double YawStart = 0.30; // rad/s
    constexpr double YawFull = 1.50; // rad/s
    constexpr double SpeedStart = 950.0; // cm/s
    constexpr double SpeedFull = 3150.0; // cm/s
    const double yawBlend = std::clamp(
        (std::abs(yawRate) - YawStart) / (YawFull - YawStart),
        0.0, 1.0);
    const double speedBlend = std::clamp(
        (sample.PlanarSpeedCmPerSecond - SpeedStart) /
            (SpeedFull - SpeedStart),
        0.0, 1.0);
    const double factor = 1.0 - 0.65 * yawBlend * speedBlend;
    command.Steering *= factor;
    return command;
}
} // namespace latebrake
