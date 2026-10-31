#pragma once

#include <cmath>

namespace latebrake
{
// This prototype drives an Unreal rigid body rather than a simulated gearbox.
// Bound only the measured backwards component of planar chassis travel.
// Preserve lateral (drift/impact) velocity and vertical Chaos motion.
// This is not reverse gear shifting, tire traction or an ABS system.
struct ReverseSpeedLimitOutput final
{
    bool Valid = false;
    bool Limited = false;
    double VelocityX = 0.0;
    double VelocityY = 0.0;
};

inline double MaximumReverseSpeedCmPerSecond(bool arcadeHandling) noexcept
{
    // Fixed reverse gear budgets: garage top-speed upgrades should only
    // improve forward cruising, not make the car reverse at racing speed.
    return arcadeHandling ? 1350.0 : 1000.0;
}

inline ReverseSpeedLimitOutput LimitPlanarReverseSpeed(
    double velocityX, double velocityY,
    double forwardX, double forwardY,
    bool arcadeHandling) noexcept
{
    if (!std::isfinite(velocityX) || !std::isfinite(velocityY) ||
        !std::isfinite(forwardX) || !std::isfinite(forwardY))
    {
        return {};
    }

    const double forwardLength = std::hypot(forwardX, forwardY);
    const double speed = std::hypot(velocityX, velocityY);
    if (!std::isfinite(forwardLength) || forwardLength < 0.01 ||
        !std::isfinite(speed))
    {
        return {};
    }

    const double unitForwardX = forwardX / forwardLength;
    const double unitForwardY = forwardY / forwardLength;
    const double longitudinal = velocityX * unitForwardX +
                                velocityY * unitForwardY;
    if (!std::isfinite(longitudinal))
    {
        return {};
    }

    const double maximumReverse = MaximumReverseSpeedCmPerSecond(arcadeHandling);
    // Treat sub-micrometer-per-second dot-product noise at the exact cap
    // as already limited, avoiding spurious SetPhysicsLinearVelocity calls.
    if (longitudinal >= -maximumReverse - 1e-7)
    {
        return {true, false, velocityX, velocityY};
    }

    // Move the measured velocity only along the vehicle's current forward
    // axis. A lateral slide must not disappear when reverse is limited.
    const double excessReverse = -maximumReverse - longitudinal;
    const double limitedX = velocityX + unitForwardX * excessReverse;
    const double limitedY = velocityY + unitForwardY * excessReverse;
    if (!std::isfinite(limitedX) || !std::isfinite(limitedY))
    {
        return {};
    }
    return {true, true, limitedX, limitedY};
}
} // namespace latebrake
