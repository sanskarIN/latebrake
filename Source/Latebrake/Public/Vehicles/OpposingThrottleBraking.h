#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Opposing accelerator/reverse demand brakes the existing rigid-body
// drivetrain before changing travel direction. This is a bounded rear-axle
// approximation, not a simulated gearbox or per-wheel engine-brake solver.
inline constexpr double ArcadeOpposingThrottleDeceleration = 650.0;
inline constexpr double SimulationOpposingThrottleDeceleration = 750.0;

struct OpposingThrottleBrakeSample final
{
    double DeltaSeconds = 0.0;
    double ForwardCmPerSecond = 0.0;
    double PlanarCmPerSecond = 0.0;
    double MassKg = 0.0;
    double EffectiveThrottle = 0.0;
    double GripScale = 1.0; // same surface/tire/garage/damage as service brake
    double RearTractionScale = 0.0; // qualified, reattached rear-axle load
    double ContactTractionScale = 0.0; // whole-body ceiling on lateral grip
    bool EffectiveBrakeHeld = false; // service/handbrake takes precedence
    bool ArcadeHandling = true;
};

struct OpposingThrottleBrakeOutput final
{
    bool Valid = false;
    // Shared motor/brake classification: the pawn must not independently
    // classify the same input differently and apply forward motor thrust.
    bool OpposingThrottle = false;
    double ForwardForce = 0.0; // ALREADY scaled by rear traction, exactly once
    double DecelerationCmPerSecondSquared = 0.0; // actual planned rear force / mass
    // Lateral force is multiplied by whole-body contact at dispatch. Divide
    // the rear-limited deceleration by that SAME contact ceiling here, so
    // their resulting forces draw from one consistently scaled grip circle.
    double WholeBodyBudgetDeceleration = 0.0;
};

inline OpposingThrottleBrakeOutput CalculateOpposingThrottleBraking(
    const OpposingThrottleBrakeSample& s) noexcept
{
    if (!std::isfinite(s.DeltaSeconds) || s.DeltaSeconds <= 0.0 ||
        s.DeltaSeconds > 0.10 ||
        !std::isfinite(s.ForwardCmPerSecond) ||
        !std::isfinite(s.PlanarCmPerSecond) ||
        s.PlanarCmPerSecond < 0.0 || s.PlanarCmPerSecond > 15000.0 ||
        std::abs(s.ForwardCmPerSecond) > s.PlanarCmPerSecond + 0.01 ||
        !std::isfinite(s.MassKg) || s.MassKg < 100.0 || s.MassKg > 15000.0 ||
        !std::isfinite(s.EffectiveThrottle) ||
        std::abs(s.EffectiveThrottle) > 1.0 ||
        !std::isfinite(s.GripScale) || s.GripScale < 0.05 || s.GripScale > 4.0 ||
        !std::isfinite(s.RearTractionScale) ||
        !std::isfinite(s.ContactTractionScale) ||
        s.ContactTractionScale < 0.0 || s.ContactTractionScale > 1.0 ||
        s.RearTractionScale < 0.0 ||
        s.RearTractionScale > s.ContactTractionScale)
    {
        return {};
    }

    OpposingThrottleBrakeOutput out;
    out.Valid = true;
    // Opposite-pedal braking depends on actual signed longitudinal travel,
    // NOT total planar speed. During a large sideways slide the car can have
    // 2,000 cm/s of lateral velocity but almost no forward motion. Using
    // planar speed incorrectly marks either pedal as an opposing brake,
    // discarding motor authority with zero useful deceleration. Preserve
    // the original 100 cm/s near-stop allowance along the actual drive axis.
    // Neutral input coasts; service/handbrake always takes precedence.
    out.OpposingThrottle = !s.EffectiveBrakeHeld &&
        std::abs(s.EffectiveThrottle) >= 0.01 &&
        std::abs(s.ForwardCmPerSecond) >= 100.0 &&
        s.ForwardCmPerSecond * s.EffectiveThrottle < 0.0;
    if (!out.OpposingThrottle || s.RearTractionScale <= 1.0e-6)
    {
        return out;
    }

    const double speed = std::abs(s.ForwardCmPerSecond);
    const double ceiling = s.ArcadeHandling
        ? ArcadeOpposingThrottleDeceleration
        : SimulationOpposingThrottleDeceleration;
    // Keep the old low-speed damping shape, but cap its racing-speed
    // request by actual grip and analog pedal demand instead of 3*speed
    // (which demanded ~20g at 6500 cm/s). Never reverse the sampled axis
    // with this force alone, even in a long qualified frame.
    const double nominal = std::min({
        3.0 * speed,
        ceiling * std::min(s.GripScale, 2.0) *
            std::abs(s.EffectiveThrottle),
        0.80 * speed / s.DeltaSeconds});
    out.DecelerationCmPerSecondSquared = nominal * s.RearTractionScale;
    out.ForwardForce = -std::copysign(
        s.MassKg * out.DecelerationCmPerSecondSquared,
        s.ForwardCmPerSecond);
    // Positive rear traction necessarily implies positive whole-body grip.
    out.WholeBodyBudgetDeceleration =
        out.DecelerationCmPerSecondSquared / s.ContactTractionScale;
    return out;
}
} // namespace latebrake
