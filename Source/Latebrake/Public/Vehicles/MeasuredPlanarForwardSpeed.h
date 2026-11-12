#pragma once

#include <cmath>
#include <limits>

namespace latebrake
{
// World-space Chaos velocity is cm/s. Launch timing needs the signed
// HORIZONTAL road-speed component in the car's projected forward direction.
// A pitched chassis can move vertically without moving along the road.
// This policy cannot affect vehicle forces, checkpoints, saves or rewards.
inline constexpr double RoadCmPerSecondToKmH = 0.036;
inline constexpr double MinimumProjectedForwardLength = 1.0e-4;

inline double MeasuredPlanarForwardSpeedKmh(
    double velocityX, double velocityY,
    double chassisForwardX, double chassisForwardY) noexcept
{
    const double invalid = std::numeric_limits<double>::quiet_NaN();
    if (!std::isfinite(velocityX) || !std::isfinite(velocityY) ||
        !std::isfinite(chassisForwardX) ||
        !std::isfinite(chassisForwardY))
    {
        return invalid;
    }

    // Unreal's forward vector is unit length in 3D. Projection to XY
    // must be re-normalized after pitch, not left shortened by cos(pitch).
    // Near vertical, the XY heading is unreliable: do not invent a launch
    // reading from arbitrary yaw or the body's vertical fall speed.
    const double horizontalLength =
        std::hypot(chassisForwardX, chassisForwardY);
    if (!std::isfinite(horizontalLength) ||
        horizontalLength < MinimumProjectedForwardLength ||
        horizontalLength > 1.000001)
    {
        return invalid;
    }
    const double roadSpeed = velocityX *
        (chassisForwardX / horizontalLength) +
        velocityY * (chassisForwardY / horizontalLength);
    const double speedKmh = roadSpeed * RoadCmPerSecondToKmH;
    return std::isfinite(speedKmh) ? speedKmh : invalid;
}
} // namespace latebrake
