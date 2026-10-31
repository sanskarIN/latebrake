#pragma once

#include "Vehicles/ArcadeReverseSpeedLimit.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// The starter car uses direct rigid-body propulsion, not a drivetrain.
// Fade only *with-direction* motor thrust as measured chassis speed nears
// the existing forward or reverse cap. This reduces the impulse repeatedly
// driving into the hard cap without changing brake, gear-change or lateral
// physics. Return a bounded scalar shared by baseline, nitro and drafting.
inline double GearAwareMotorPowerFactor(
    double requestedThrottle,
    double measuredLongitudinalCmPerSecond,
    double forwardSpeedCapCmPerSecond,
    bool arcadeHandling) noexcept
{
    if (!std::isfinite(requestedThrottle) ||
        !std::isfinite(measuredLongitudinalCmPerSecond) ||
        !std::isfinite(forwardSpeedCapCmPerSecond) ||
        forwardSpeedCapCmPerSecond < 500.0 ||
        forwardSpeedCapCmPerSecond > 20000.0)
    {
        // Malformed speed/cap/axis data cannot authorize motor thrust.
        return 0.0;
    }

    const double demand = std::clamp(requestedThrottle, -1.0, 1.0);
    if (demand == 0.0)
    {
        return 1.0;
    }

    // If the driver is requesting torque *against* current travel, retain
    // the existing engine-brake/reverse-transition behavior untouched.
    if ((demand > 0.0 && measuredLongitudinalCmPerSecond <= 0.0) ||
        (demand < 0.0 && measuredLongitudinalCmPerSecond >= 0.0))
    {
        return 1.0;
    }

    const bool reversing = demand < 0.0;
    const double speedCap = reversing
        ? MaximumReverseSpeedCmPerSecond(arcadeHandling)
        : forwardSpeedCapCmPerSecond;
    // Preserve full launch authority, then apply a continuous C1 ramp.
    // Reverse begins tapering earlier to avoid repeated hard clamps
    // immediately after a small change in heading.
    const double startRatio = reversing ? 0.74 : 0.82;
    const double fadeStart = speedCap * startRatio;
    const double speed = std::abs(measuredLongitudinalCmPerSecond);
    if (speed <= fadeStart)
    {
        return 1.0;
    }
    if (speed >= speedCap)
    {
        return 0.0;
    }
    const double fraction = std::clamp(
        (speed - fadeStart) / (speedCap - fadeStart), 0.0, 1.0);
    const double smoothstep = fraction * fraction * (3.0 - 2.0 * fraction);
    return 1.0 - smoothstep;
}
} // namespace latebrake
