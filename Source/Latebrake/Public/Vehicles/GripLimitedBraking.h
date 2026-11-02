#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Longitudinal-only, grip-limited braking for the original shared Unreal
// player/rival rigid-body chassis. It cannot detect locked individual wheels
// and is NOT ABS, a hydraulic system, tire contact or a full Chaos vehicle.
// All deceleration applies opposite the REAL measured signed forward speed.
struct GripLimitedBrakingSample final
{
    double DeltaSeconds = 0.0;
    double ForwardCmPerSecond = 0.0;
    double MassKg = 0.0;
    bool ServiceBrakeHeld = false;
    bool HandbrakeHeld = false;
    bool ArcadeHandling = true;
    double SurfaceGripMultiplier = 1.0;
    double TireGripMultiplier = 1.0;
    double DamageGripMultiplier = 1.0;
    double ServiceBrakeHeatMultiplier = 1.0;
};

struct GripLimitedBrakingOutput final
{
    bool Valid = false;
    // Local-forward direction Unreal force (kg*cm/s^2), signed and
    // always opposite current longitudinal travel, including reverse.
    double ForwardForce = 0.0;
    // Requested bounded longitudinal deceleration, not measured decel:
    // actual Chaos physics may add road load, lateral grip and collisions.
    double DecelerationCmPerSecondSquared = 0.0;
};

inline constexpr double ArcadeServiceBrakeDeceleration = 900.0;
inline constexpr double SimulationServiceBrakeDeceleration = 1050.0;
inline constexpr double HandbrakeDeceleration = 470.0;
inline constexpr double MaxBrakeFrameSeconds = 0.10;
inline constexpr double MaximumBrakeSpeedCmPerSecond = 15000.0;

inline GripLimitedBrakingOutput CalculateGripLimitedBraking(
    GripLimitedBrakingSample s) noexcept
{
    if (!std::isfinite(s.DeltaSeconds) ||
        s.DeltaSeconds <= 0.0 || s.DeltaSeconds > MaxBrakeFrameSeconds ||
        !std::isfinite(s.ForwardCmPerSecond) ||
        std::abs(s.ForwardCmPerSecond) > MaximumBrakeSpeedCmPerSecond ||
        !std::isfinite(s.MassKg) || s.MassKg < 100.0 ||
        s.MassKg > 15000.0 ||
        !std::isfinite(s.SurfaceGripMultiplier) ||
        !std::isfinite(s.TireGripMultiplier) ||
        !std::isfinite(s.DamageGripMultiplier) ||
        !std::isfinite(s.ServiceBrakeHeatMultiplier) ||
        s.SurfaceGripMultiplier < 0.05 ||
        s.SurfaceGripMultiplier > 2.0 ||
        s.TireGripMultiplier < 0.05 || s.TireGripMultiplier > 2.0 ||
        s.DamageGripMultiplier < 0.05 || s.DamageGripMultiplier > 2.0 ||
        s.ServiceBrakeHeatMultiplier < 0.05 ||
        s.ServiceBrakeHeatMultiplier > 1.0)
    {
        return {};
    }
    if ((!s.ServiceBrakeHeld && !s.HandbrakeHeld) ||
        std::abs(s.ForwardCmPerSecond) <= 1.0e-9)
    {
        return {true, 0.0, 0.0};
    }

    const double grip = std::clamp(
        s.SurfaceGripMultiplier * s.TireGripMultiplier *
            s.DamageGripMultiplier, 0.10, 2.0);
    const double service = s.ServiceBrakeHeld
        ? (s.ArcadeHandling ? ArcadeServiceBrakeDeceleration
                            : SimulationServiceBrakeDeceleration) *
              s.ServiceBrakeHeatMultiplier : 0.0;
    // Holding both brakes preserves a useful parking-brake minimum even
    // with faded service brakes. Braking is NOT the sum of two arbitrary
    // impulses, which could violate the finite contact-grip budget.
    const double parking = s.HandbrakeHeld ? HandbrakeDeceleration : 0.0;
    const double wanted = std::max(service, parking) * grip;

    // Never reverse measured forward motion purely by one brake force.
    // A 15% reserve tolerates other simultaneous resistive forces, while
    // the underlying Chaos integrator remains the physical authority.
    const double frameLimit =
        0.85 * std::abs(s.ForwardCmPerSecond) / s.DeltaSeconds;
    const double decel = std::min(wanted, frameLimit);
    const double force = -std::copysign(s.MassKg * decel,
        s.ForwardCmPerSecond);
    return {true, force, decel};
}
} // namespace latebrake
