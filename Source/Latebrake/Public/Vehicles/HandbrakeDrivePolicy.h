#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Prototype rigid-body parking-brake behavior, not a simulated wheel-lock or
// hydraulic/ABS system. Both player brake inputs remain independent.
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
