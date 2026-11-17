#pragma once

#include "Vehicles/LateralGripBudget.h"

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
    // SAME FRAME accepted spring/contact traction from the real chassis.
    // Zero means fully airborne/unloaded: no synthetic speed-cap brake.
    // The legacy standalone callers default to fully loaded pavement;
    // the production pawn MUST supply the actual measured contact scale.
    double ContactTractionScale = 1.0;
    // The same-frame REAL chassis correction and grip used by lateral
    // driving. Defaults only maintain older isolated source fixtures;
    // the Unreal pawn must supply actual observed and dispatched values.
    double GripScale = 1.0;
    double LateralCmPerSecond = 0.0; // measured dot(XY velocity, body right)
    double AppliedRightForce = 0.0; // actual AddForce along right AFTER contact
    bool OtherLongitudinalBrake = false; // service/opposing/reverse priority
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
        sample.SpeedCapCmPerSecond > PlanarGovernorMaximumCapCmPerSecond ||
        !std::isfinite(sample.ContactTractionScale) ||
        sample.ContactTractionScale < 0.0 ||
        sample.ContactTractionScale > 1.0 ||
        !std::isfinite(sample.GripScale) ||
        sample.GripScale < 0.05 || sample.GripScale > 4.0 ||
        !std::isfinite(sample.LateralCmPerSecond) ||
        !std::isfinite(sample.AppliedRightForce) ||
        std::abs(sample.AppliedRightForce) > 1.0e8 ||
        sample.AppliedRightForce * sample.LateralCmPerSecond > 1.0)
    {
        return {};
    }

    const double speed = std::hypot(sample.VelocityX, sample.VelocityY);
    if (!std::isfinite(speed) ||
        speed > PlanarGovernorMaximumSpeedCmPerSecond ||
        std::abs(sample.LateralCmPerSecond) > speed + 1.0)
    {
        return {};
    }

    PlanarSpeedGovernorOutput output;
    output.Valid = true;
    // Airborne travel continues under Chaos gravity/collision and the
    // separately bounded aerodynamic road load. A geometric speed cap
    // alone cannot manufacture an extra tire-like braking impulse in air.
    // One corner is also insufficient to establish qualified road grip.
    if (speed <= sample.SpeedCapCmPerSecond ||
        sample.ContactTractionScale <= 1.0e-6 ||
        sample.OtherLongitudinalBrake)
    {
        return output;
    }

    // This force opposes the FULL XY velocity, not the car's forward
    // axis. Existing lateral skid correction may point partly along
    // that exact same direction. A naive orthogonal friction-circle
    // sqrt would therefore grant more combined grip than exists.
    // Solve |L + x*D| <= G for nonnegative x: L is the already
    // delivered body-right acceleration and D is -unit(XY velocity).
    const double totalGrip = (sample.ArcadeHandling
        ? ArcadeCombinedGripBudget : SimulationCombinedGripBudget) *
        sample.GripScale * sample.ContactTractionScale;
    const double lateral = std::abs(sample.AppliedRightForce) / sample.MassKg;
    if (lateral > totalGrip + 1.0e-6)
    {
        return {}; // physically inconsistent force: fail closed
    }
    const double aligned = std::clamp(
        -sample.AppliedRightForce * sample.LateralCmPerSecond /
            (sample.MassKg * speed), 0.0, lateral);
    const double transverseSquared = std::max(
        0.0, lateral * lateral - aligned * aligned);
    const double available = std::max(0.0,
        std::sqrt(std::max(0.0, totalGrip * totalGrip -
                          transverseSquared)) - aligned);

    const double excess = speed - sample.SpeedCapCmPerSecond;
    const double limit = sample.ArcadeHandling
        ? PlanarGovernorArcadeMaxDeceleration
        : PlanarGovernorSimulationMaxDeceleration;
    // Scale the existing bounded request by this same physical contact
    // budget ONCE. With full measured load the old arcade/simulation
    // tuning is unchanged. Partial contact can only reduce deceleration;
    // losing all support immediately withdraws speed-cap brake authority.
    const double requested = std::min({
        PlanarGovernorResponsePerSecond * excess,
        limit,
        excess / sample.DeltaSeconds,
        0.8 * speed / sample.DeltaSeconds
    }) * sample.ContactTractionScale;
    // Contact and grip are budgets, not another multiplication of
    // lateral force or rear loading. This cap can only REDUCE request.
    const double decel = std::min(requested, available);
    if (decel <= 1.0e-9) return output;
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
