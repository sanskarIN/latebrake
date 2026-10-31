#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Arcade-only, human-only positive propulsion reduction for an actual
// high-speed physical spin. This is an engine-independent final throttle
// policy: no steering corrections, brake overrides, teleports, rewards,
// checkpoint changes, save mutations or new velocity impulses.
//
// Units match Unreal Chaos: forward/sideways chassis speed in cm/s and
// angular velocity around the world Z axis in radians/s. Do not substitute
// requested steering input or a cosmetic drift meter for measured physics.
inline double ModerateHumanSpinThrottle(
    double requestedThrottle, double signedForwardCmPerSecond,
    double lateralCmPerSecond, double yawRadiansPerSecond,
    bool arcadeHandling, bool locallyControlledHuman,
    bool handbrakeHeld) noexcept
{
    // Invalid physics is never permission to add more motor force.
    if (!std::isfinite(requestedThrottle) ||
        !std::isfinite(signedForwardCmPerSecond) ||
        !std::isfinite(lateralCmPerSecond) ||
        !std::isfinite(yawRadiansPerSecond))
    {
        return 0.0;
    }
    const double command = std::clamp(requestedThrottle, -1.0, 1.0);

    // The AI has its own spin guard. Simulation Inspired remains deliberately
    // unassisted; handbrake drifting and all reverse/engine-brake demands
    // retain exactly their preexisting physics response.
    if (!arcadeHandling || !locallyControlledHuman || handbrakeHeld ||
        command <= 0.0)
    {
        return command;
    }
    if (signedForwardCmPerSecond <= 1400.0 ||
        std::abs(yawRadiansPerSecond) <= 1.10)
    {
        return command;
    }

    const double planarSpeed = std::hypot(
        signedForwardCmPerSecond, lateralCmPerSecond);
    if (!std::isfinite(planarSpeed) || planarSpeed <= 0.0)
    {
        return 0.0;
    }
    const double slipRatio = std::abs(lateralCmPerSecond) / planarSpeed;
    if (!std::isfinite(slipRatio) || slipRatio <= 0.25)
    {
        return command;
    }

    // Three continuous blends avoid a throttle step at corner entry.
    // Strong assistance requires high forward speed, actual large sideways
    // movement AND an already fast-spinning chassis, not merely steering.
    const double speedBlend = std::clamp(
        (signedForwardCmPerSecond - 1400.0) / 1000.0, 0.0, 1.0);
    const double slipBlend = std::clamp(
        (slipRatio - 0.25) / 0.25, 0.0, 1.0);
    const double yawBlend = std::clamp(
        (std::abs(yawRadiansPerSecond) - 1.10) / 1.50, 0.0, 1.0);
    // Lower bound retains enough positive engine authority for recovery.
    // Earlier lateral-grip and corner throttle policies still apply.
    const double reduction = 0.70 * speedBlend * slipBlend * yawBlend;
    return command * (1.0 - reduction);
}
} // namespace latebrake
