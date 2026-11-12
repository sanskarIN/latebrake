#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Prototype rigid-body parking-brake behavior, not a simulated wheel-lock or
// hydraulic/ABS system. Both player brake inputs remain independent.
// The held handbrake's deliberately simple yaw torque still needs a
// continuous physical travel direction. An old +/-75 cm/s threshold made
// the car briefly steer as if moving FORWARD while slowly reversing, then
// snapped to reverse steering at -75 cm/s. Use the same measured signed
// longitudinal speed as ordinary yaw control instead; the authority is
// zero at rest, changes sign smoothly, and reaches its former full strength
// at +/-900 cm/s. No tire contact or extra torque is invented here.
inline constexpr double HandbrakeYawFullAuthorityCmPerSecond = 900.0;
inline constexpr double HandbrakeYawMaximumMeasuredCmPerSecond = 15000.0;
inline double SignedHandbrakeYawAuthority(
    double signedForwardCmPerSecond) noexcept
{
    if (!std::isfinite(signedForwardCmPerSecond) ||
        std::abs(signedForwardCmPerSecond) >
            HandbrakeYawMaximumMeasuredCmPerSecond)
    {
        return 0.0;
    }
    return std::clamp(signedForwardCmPerSecond /
                          HandbrakeYawFullAuthorityCmPerSecond,
                      -1.0, 1.0);
}

struct HandbrakeDriveProfile final
{
    double LateralGripRate;
    double ForwardResistanceRate;
    double Throttle;
    bool Braking;
    bool HandbrakeActive;
};

inline HandbrakeDriveProfile SelectHandbrakeDriveProfile(
    bool handbrakeHeld, bool serviceBrakeHeld, bool arcadeHandling,
    double normalLateralGripRate, double requestedThrottle) noexcept
{
    // Preserve ordinary handling and AI output when the handbrake is off.
    // Invalid tuning must never become nonfinite forces.
    const double safeGrip =
        std::isfinite(normalLateralGripRate) &&
        normalLateralGripRate >= 0.0 && normalLateralGripRate <= 40.0
            ? normalLateralGripRate : 0.0;
    const double safeThrottle = std::isfinite(requestedThrottle)
        ? std::clamp(requestedThrottle, -1.0, 1.0) : 0.0;
    if (!handbrakeHeld)
    {
        return {safeGrip, serviceBrakeHeld ? 7.0 : 0.55,
                safeThrottle, serviceBrakeHeld, false};
    }

    // Holding a human handbrake disengages the motor; it deliberately
    // retains more lateral momentum for a low-speed arcade slide. If the
    // ordinary brake is also held, preserve its stronger stopping demand.
    // Resistive force terms are additionally clamped by FrameStableDamping.
    return {arcadeHandling ? 1.8 : 1.1,
            serviceBrakeHeld ? 7.0 : 4.2,
            0.0, true, true};
}
} // namespace latebrake
