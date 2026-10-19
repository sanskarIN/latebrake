#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace latebrake
{
// Pure C++20 steering planner. Unreal hosts the rigid-body simulation.
// Positions and speeds use Unreal's centimetre units; heading is in radians.
struct PilotCommand final
{
    double Throttle = 0.0;
    double Steering = 0.0;
    bool Brake = true;
    bool Arrived = false;
};

inline PilotCommand PlanRivalTurn(
    double positionX, double positionY, double headingRadians,
    double speedCmPerSecond, double waypointX, double waypointY,
    double arrivalRadiusCm = 440.0) noexcept
{
    const bool valid = std::isfinite(positionX) && std::isfinite(positionY) &&
        std::isfinite(headingRadians) && std::isfinite(speedCmPerSecond) &&
        std::isfinite(waypointX) && std::isfinite(waypointY) &&
        std::isfinite(arrivalRadiusCm) && arrivalRadiusCm > 0.0;
    if (!valid)
    {
        return {};
    }

    const double dx = waypointX - positionX;
    const double dy = waypointY - positionY;
    const double distance = std::hypot(dx, dy);
    if (!std::isfinite(distance))
    {
        return {};
    }
    if (distance <= arrivalRadiusCm)
    {
        return {0.0, 0.0, false, true};
    }

    const double desiredHeading = std::atan2(dy, dx);
    const double headingError = std::atan2(
        std::sin(desiredHeading - headingRadians),
        std::cos(desiredHeading - headingRadians));
    const double errorMagnitude = std::abs(headingError);
    const double steering = std::clamp(headingError / 0.82, -1.0, 1.0);

    // Approach corners at lower target speed. No instantaneous heading jumps.
    const double turnFactor = std::clamp(
        errorMagnitude / (std::numbers::pi / 2.0), 0.0, 1.0);
    const double targetSpeed = 4100.0 - 2950.0 * turnFactor;
    const double speed = std::max(0.0, speedCmPerSecond);
    const bool braking = speed > targetSpeed + 220.0;
    const double throttle = (!braking && speed < targetSpeed - 100.0) ? 0.85 : 0.0;

    return {throttle, steering, braking, false};
}
} // namespace latebrake
