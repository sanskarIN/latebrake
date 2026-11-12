#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// A small deterministic road-load approximation for the ACTUAL Unreal
// physics chassis. Drag rises with squared PLANAR speed, while low-speed
// rolling resistance fades in without an instant brake-like jump. This is
// not tire contact, aero downforce, wind, gears, suspension or a CFD model.
// Force points precisely opposite measured motion, including reverse and
// sideslip; the vertical Chaos/gravity component is never modified.
struct ProgressiveRoadLoadSample final
{
    double DeltaSeconds = 0.0;
    double VelocityX = 0.0;      // Unreal cm/s, world XY
    double VelocityY = 0.0;
    double ChassisMassKg = 0.0;
    bool ArcadeHandling = true;
    bool OffPavement = false;
    // Measured live Chaos spring-load support: zero when airborne.
    double SprungLoadFraction = 1.0;
};

struct ProgressiveRoadLoadOutput final
{
    bool Valid = false;
    double ForceX = 0.0;         // Unreal kg*cm/s^2
    double ForceY = 0.0;
    double DecelerationCmPerSecondSquared = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
};

inline constexpr double RoadLoadMaxFrameSeconds = 0.10;
inline constexpr double RoadLoadMaxPlanarSpeed = 15000.0;
inline constexpr double RoadLoadMinimumMassKg = 100.0;
inline constexpr double RoadLoadMaximumMassKg = 15000.0;

// Intentional tuning targets for the simple arcade rigid body: lower
// constant Chaos damping handles very slow rolling; quadratic drag is
// the dominant high-speed resistance. The coefficients are gameplay
// estimates and MUST be tuned in a real installed UE build.
inline constexpr double RoadLoadChassisLinearDamping = 0.08;
inline constexpr double RoadLoadArcadeQuadraticDrag = 0.000018;
inline constexpr double RoadLoadSimulationQuadraticDrag = 0.000024;
inline constexpr double RoadLoadArcadeRollingDeceleration = 30.0;
inline constexpr double RoadLoadSimulationRollingDeceleration = 45.0;
inline constexpr double RoadLoadOffPavementResistance = 1.45;

inline ProgressiveRoadLoadOutput CalculateProgressiveRoadLoad(
    ProgressiveRoadLoadSample sample) noexcept
{
    if (!std::isfinite(sample.DeltaSeconds) ||
        sample.DeltaSeconds <= 0.0 ||
        sample.DeltaSeconds > RoadLoadMaxFrameSeconds ||
        !std::isfinite(sample.VelocityX) ||
        !std::isfinite(sample.VelocityY) ||
        !std::isfinite(sample.ChassisMassKg) ||
        sample.ChassisMassKg < RoadLoadMinimumMassKg ||
        sample.ChassisMassKg > RoadLoadMaximumMassKg ||
        !std::isfinite(sample.SprungLoadFraction) ||
        sample.SprungLoadFraction < 0.0 ||
        sample.SprungLoadFraction > 1.0)
    {
        return {};
    }

    const double speed = std::hypot(sample.VelocityX, sample.VelocityY);
    if (!std::isfinite(speed) || speed > RoadLoadMaxPlanarSpeed)
        return {};
    if (speed <= 1.0e-9)
        return {true, 0.0, 0.0, 0.0, 0.0};

    const double quadratic = sample.ArcadeHandling
        ? RoadLoadArcadeQuadraticDrag
        : RoadLoadSimulationQuadraticDrag;
    const double rolling = sample.ArcadeHandling
        ? RoadLoadArcadeRollingDeceleration
        : RoadLoadSimulationRollingDeceleration;
    // Near a stop, avoid a constant Coulomb force that can reverse the car
    // across the origin; use a bounded continuous ramp (0 at exactly zero).
    const double rollingRamp = std::min(1.0, speed / 250.0);
    // Only real sprung contact can generate tire/road resistance.
    // Aerodynamic drag remains active while the car is airborne.
    const double support = sample.SprungLoadFraction;
    const double surface = sample.OffPavement
        ? 1.0 + (RoadLoadOffPavementResistance - 1.0) * support : 1.0;
    const double demand = surface *
        (quadratic * speed * speed + rolling * rollingRamp * support);
    // Reserve 20% of the current speed against a one-frame reversal from
    // THIS additional resistance alone. Physics substeps/contact remain
    // the Chaos engine's responsibility.
    const double maxDeceleration = 0.8 * speed / sample.DeltaSeconds;
    const double deceleration = std::min(demand, maxDeceleration);
    const double forceScale =
        -sample.ChassisMassKg * deceleration / speed;
    return {true, sample.VelocityX * forceScale,
            sample.VelocityY * forceScale, deceleration, speed};
}
} // namespace latebrake
