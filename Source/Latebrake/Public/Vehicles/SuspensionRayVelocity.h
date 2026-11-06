#pragma once

#include "Vehicles/SprungChassisDynamics.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
struct SuspensionRayVelocitySample final
{
    double PointVelocityX = 0.0;
    double PointVelocityY = 0.0;
    double PointVelocityZ = 0.0;
    double NormalX = 0.0;
    double NormalY = 0.0;
    double NormalZ = 0.0;
};

struct SuspensionRayVelocityOutput final
{
    bool Valid = false;
    double DistanceSpeedCmPerSecond = 0.0;
    bool RateLimited = false;
};

// For a world-down ray from A to a STATIC tangent plane, distance is
// d = (N dot A - planeOffset) / Nz. Therefore d' = (N dot pointVelocity)/Nz.
// Tangential driving along a slope leaves d constant even though Vz != 0.
// This uses the existing hit normal and measured Chaos velocity AT the
// corner; it does not estimate speed from earlier rays or add contact.
// It is not suitable for moving-platform suspension or a per-wheel solver.
inline SuspensionRayVelocityOutput MeasureSuspensionRayVelocity(
    const SuspensionRayVelocitySample& s) noexcept
{
    if (!std::isfinite(s.PointVelocityX) ||
        !std::isfinite(s.PointVelocityY) ||
        !std::isfinite(s.PointVelocityZ) ||
        !std::isfinite(s.NormalX) || !std::isfinite(s.NormalY) ||
        !std::isfinite(s.NormalZ) ||
        std::abs(s.NormalX) > 1.0 || std::abs(s.NormalY) > 1.0 ||
        std::abs(s.NormalZ) > 1.0)
    {
        return {};
    }
    const double normalSquared = s.NormalX * s.NormalX +
        s.NormalY * s.NormalY + s.NormalZ * s.NormalZ;
    if (std::abs(normalSquared - 1.0) > 1.0e-3)
    {
        return {};
    }
    // A steep wall/underside is a valid trace observation, but the spring
    // policy cannot use it as road support. Never divide by a near-zero Nz.
    if (s.NormalZ < SuspensionMinNormalZ)
    {
        return {true, 0.0, false};
    }
    const double rate = (s.NormalX * s.PointVelocityX +
        s.NormalY * s.PointVelocityY + s.NormalZ * s.PointVelocityZ) / s.NormalZ;
    if (!std::isfinite(rate))
    {
        return {};
    }
    // Preserve the spring policy's existing finite damping domain. A fast
    // but finite approach saturates the already bounded spring response;
    // it must not become a malformed frame that freezes a valid body.
    const double bounded = std::clamp(rate,
        -SuspensionMaximumDistanceSpeedCmPerSecond,
        SuspensionMaximumDistanceSpeedCmPerSecond);
    return {true, bounded, bounded != rate};
}
} // namespace latebrake
