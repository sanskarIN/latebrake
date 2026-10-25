#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Planar vehicle speed in Unreal cm/s. Steering is intentionally unchanged
// below 900 cm/s (~32 km/h), and gradually reduced through 5400 cm/s
// (~194 km/h). Arcade retains more steering than Simulation Inspired.
// This is a deterministic policy, not a replacement for tire/Chaos tuning.
inline double SteeringSpeedFactor(double speedCmPerSecond,
                                  bool arcadeHandling) noexcept
{
    if (!std::isfinite(speedCmPerSecond) || speedCmPerSecond < 0.0)
    {
        return 0.0; // A corrupt speed may never authorize steering torque.
    }
    constexpr double FullAuthoritySpeed = 900.0;
    constexpr double MinimumAuthoritySpeed = 5400.0;
    const double normalized = std::clamp(
        (speedCmPerSecond - FullAuthoritySpeed) /
            (MinimumAuthoritySpeed - FullAuthoritySpeed),
        0.0, 1.0);
    const double minimumFactor = arcadeHandling ? 0.55 : 0.40;
    return 1.0 - normalized * (1.0 - minimumFactor);
}

// Retain input sign for forward/reverse turn-direction correction in the
// Unreal pawn. The same factor applies to touch, keyboard, controller and
// rival commands after their existing smoothing/normalization.
inline double LimitSteeringAtSpeed(double smoothedSteering,
                                   double planarSpeedCmPerSecond,
                                   bool arcadeHandling) noexcept
{
    if (!std::isfinite(smoothedSteering))
    {
        return 0.0;
    }
    return std::clamp(smoothedSteering, -1.0, 1.0) *
           SteeringSpeedFactor(planarSpeedCmPerSecond, arcadeHandling);
}
} // namespace latebrake
