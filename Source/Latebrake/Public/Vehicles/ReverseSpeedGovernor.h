#pragma once

#include "Vehicles/ArcadeReverseSpeedLimit.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// The signed reverse speed cap is now a PHYSICAL, forward-axis resistive
// force, rather than a frame-dependent SetPhysicsLinearVelocity teleport.
// Source-only prototype on the real shared player/rival Chaos chassis:
// not ABS, drivetrain simulation, measured tire grip or a hard safety cap.
// Transient collisions can still briefly exceed the target while the
// bounded response settles. The old instantaneous reference limiter in
// ArcadeReverseSpeedLimit.h is deliberately NOT called by the pawn.
inline constexpr double ReverseGovernorMaximumFrameSeconds = 0.10;
inline constexpr double ReverseGovernorMaximumPlanarSpeed = 15000.0;
inline constexpr double ReverseGovernorResponsePerSecond = 3.0;
inline constexpr double ReverseGovernorArcadeMaxDeceleration = 850.0;
inline constexpr double ReverseGovernorSimulationMaxDeceleration = 650.0;

struct ReverseSpeedGovernorSample final
{
    double DeltaSeconds = 0.0;
    double VelocityX = 0.0;
    double VelocityY = 0.0;
    double ForwardX = 0.0; // current planar Chaos chassis heading
    double ForwardY = 0.0;
    double MassKg = 0.0;
    double Throttle = 0.0; // final effective signed drive demand
    double GripScale = 1.0; // surface/tire/garage/damage, applied once
    double RearTractionScale = 0.0;
    double ContactTractionScale = 0.0;
    bool ArcadeHandling = true;
    bool OtherLongitudinalBrake = false;
};

struct ReverseSpeedGovernorOutput final
{
    bool Valid = false;
    bool Active = false;
    // At the actual reverse target, block new negative motor thrust.
    // Existing physical momentum, sideways sliding and gravity continue.
    bool ReverseMotorBlocked = false;
    double ForceX = 0.0;
    double ForceY = 0.0;
    double ExcessReverseCmPerSecond = 0.0;
    double DecelerationCmPerSecondSquared = 0.0;
    // Existing lateral grip policy applies whole-body contact at dispatch.
    // Divide our rear-limited acceleration by that same contact ONCE.
    double WholeBodyBudgetDeceleration = 0.0;
};

inline ReverseSpeedGovernorOutput CalculateReverseSpeedGovernor(
    const ReverseSpeedGovernorSample& s) noexcept
{
    if (!std::isfinite(s.DeltaSeconds) ||
        s.DeltaSeconds <= 0.0 ||
        s.DeltaSeconds > ReverseGovernorMaximumFrameSeconds ||
        !std::isfinite(s.VelocityX) || !std::isfinite(s.VelocityY) ||
        !std::isfinite(s.ForwardX) || !std::isfinite(s.ForwardY) ||
        !std::isfinite(s.MassKg) || s.MassKg < 100.0 || s.MassKg > 15000.0 ||
        !std::isfinite(s.Throttle) || std::abs(s.Throttle) > 1.0 ||
        !std::isfinite(s.GripScale) ||
        s.GripScale < 0.05 || s.GripScale > 4.0 ||
        !std::isfinite(s.RearTractionScale) ||
        !std::isfinite(s.ContactTractionScale) ||
        s.ContactTractionScale < 0.0 || s.ContactTractionScale > 1.0 ||
        s.RearTractionScale < 0.0 ||
        s.RearTractionScale > s.ContactTractionScale)
    {
        return {};
    }
    const double speed = std::hypot(s.VelocityX, s.VelocityY);
    const double heading = std::hypot(s.ForwardX, s.ForwardY);
    if (!std::isfinite(speed) ||
        speed > ReverseGovernorMaximumPlanarSpeed ||
        !std::isfinite(heading) || heading < 0.01 ||
        heading > 2.5)
    {
        return {};
    }
    const double fx = s.ForwardX / heading;
    const double fy = s.ForwardY / heading;
    const double signedForward = s.VelocityX * fx + s.VelocityY * fy;
    if (!std::isfinite(signedForward)) return {};

    ReverseSpeedGovernorOutput out;
    out.Valid = true;
    const double cap = MaximumReverseSpeedCmPerSecond(s.ArcadeHandling);
    // Slight anticipation (2 cm/s) avoids repeatedly forcing the cap
    // across the threshold while the driver holds reverse. This gate
    // affects ONLY reverse motor thrust, never the actor's velocity.
    out.ReverseMotorBlocked =
        s.Throttle < -0.01 && signedForward <= -cap + 2.0;
    const double excess = -cap - signedForward;
    if (excess <= 1.0e-7 || s.OtherLongitudinalBrake ||
        s.RearTractionScale <= 1.0e-6)
    {
        return out;
    }
    // Never stack an independent governor with service/handbrake or
    // opposing-pedal engine braking. This controller operates only with
    // qualified actual rear support, and never adds air-control force.
    const double ceiling = s.ArcadeHandling
        ? ReverseGovernorArcadeMaxDeceleration
        : ReverseGovernorSimulationMaxDeceleration;
    const double nominal = std::min({
        ReverseGovernorResponsePerSecond * excess,
        ceiling * std::clamp(s.GripScale, 0.10, 2.0),
        0.80 * excess / s.DeltaSeconds,
        0.80 * std::abs(signedForward) / s.DeltaSeconds
    });
    const double decel = nominal * s.RearTractionScale;
    if (!std::isfinite(decel) || decel <= 0.0) return out;
    // The excess can settle over many frames but cannot cross the
    // measured reverse cap solely due to this controller's own force.
    out.Active = true;
    out.ForceX = s.MassKg * decel * fx;
    out.ForceY = s.MassKg * decel * fy;
    out.ExcessReverseCmPerSecond = excess;
    out.DecelerationCmPerSecondSquared = decel;
    // Rear support >0 always implies whole-body contact >0.
    out.WholeBodyBudgetDeceleration = decel / s.ContactTractionScale;
    return out;
}
} // namespace latebrake
