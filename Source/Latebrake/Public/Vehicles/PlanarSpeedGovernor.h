#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Soft overspeed correction for the simplified real Chaos rigid-body racer.
// This is an arcade-oriented, planar resistive FORCE. It must never rewrite
// horizontal/vertical velocity or turn into a fake race result. The existing
// gearbox power taper already removes *new* forward drive near its cap.
// This small controller removes excess current horizontal speed smoothly,
// including when the nitro speed-cap multiplier ramps back down.
// It is not wheel friction, calibrated aero, or a Chaos Vehicles tire solver.
inline constexpr double PlanarGovernorMaximumFrameSeconds = 0.10;
inline constexpr double PlanarGovernorMaximumSpeedCmPerSecond = 15000.0;
inline constexpr double PlanarGovernorMinimumCapCmPerSecond = 500.0;
inline constexpr double PlanarGovernorMaximumCapCmPerSecond = 20000.0;
inline constexpr double PlanarGovernorResponsePerSecond = 3.0;
inline constexpr double PlanarGovernorArcadeMaxDeceleration = 650.0;
inline constexpr double PlanarGovernorSimulationMaxDeceleration = 500.0;

struct PlanarSpeedGovernorSample final
{
    double DeltaSeconds = 0.0;
    double VelocityX = 0.0; // world XY cm/s, true Chaos physics
    double VelocityY = 0.0;
    double MassKg = 0.0;
    double SpeedCapCmPerSecond = 0.0;
    bool ArcadeHandling = true;
};

struct PlanarSpeedGovernorOutput final
{
    bool Valid = false;
    bool Active = false;
    double ForceX = 0.0;
    double ForceY = 0.0;
    double OverspeedCmPerSecond = 0.0;
    double DecelerationCmPerSecondSquared = 0.0;
};

// A continuous proportional response after the threshold: zero force AT
// and below the cap, smoothly increasing above it. Bound the worst case
// to ~0.5-0.66g for simplified racing gameplay and limit this policy's
// one-frame deceleration to the actual positive speed excess. Never drag
// the chassis under its active cap by the governor's OWN force alone.
// Existing RoadLoad and Chaos collision are independent extra forces.
inline PlanarSpeedGovernorOutput CalculatePlanarSpeedGovernor(
    const PlanarSpeedGovernorSample& sample) noexcept
{
    if (!std::isfinite(sample.DeltaSeconds) ||
        sample.DeltaSeconds <= 0.0 ||
        sample.DeltaSeconds > PlanarGovernorMaximumFrameSeconds ||
        !std::isfinite(sample.VelocityX) ||
        !std::isfinite(sample.VelocityY) ||
        !std::isfinite(sample.MassKg) ||
        sample.MassKg < 100.0 || sample.MassKg > 15000.0 ||
        !std::isfinite(sample.SpeedCapCmPerSecond) ||
        sample.SpeedCapCmPerSecond < PlanarGovernorMinimumCapCmPerSecond ||
        sample.SpeedCapCmPerSecond > PlanarGovernorMaximumCapCmPerSecond)
    {
        return {};
    }

    const double speed = std::hypot(sample.VelocityX, sample.VelocityY);
    if (!std::isfinite(speed) ||
        speed > PlanarGovernorMaximumSpeedCmPerSecond)
    {
        return {};
    }

    PlanarSpeedGovernorOutput output;
    output.Valid = true;
    if (speed <= sample.SpeedCapCmPerSecond)
    {
        return output;
    }

    const double excess = speed - sample.SpeedCapCmPerSecond;
    const double limit = sample.ArcadeHandling
        ? PlanarGovernorArcadeMaxDeceleration
        : PlanarGovernorSimulationMaxDeceleration;
    const double decel = std::min({
        PlanarGovernorResponsePerSecond * excess,
        limit,
        excess / sample.DeltaSeconds,
        0.8 * speed / sample.DeltaSeconds
    });
    // Positive excess guarantees speed>0. Bounded unit vector and mass
    // produce no acceleration, no reversal, and no upward/downward force.
    const double scale = -sample.MassKg * decel / speed;
    output.Active = decel > 0.0;
    output.ForceX = sample.VelocityX * scale;
    output.ForceY = sample.VelocityY * scale;
    output.OverspeedCmPerSecond = excess;
    output.DecelerationCmPerSecondSquared = decel;
    return output;
}
} // namespace latebrake
