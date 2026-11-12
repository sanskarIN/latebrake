#pragma once

#include "Vehicles/LateralGripBudget.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Simplified bicycle-like yaw controller for the shared UE Chaos player/rival
// rigid-body chassis. It uses the ACTUAL measured longitudinal speed and yaw
// rate, not an animated wheel angle, and issues bounded physical Z torque.
// This is an approximation: no actual tire contact, force saturation per
// wheel, suspension, or complete vehicle inertia tensor is modeled.
struct SpeedAwareYawSample final
{
    double DeltaSeconds = 0.0;
    double ForwardSpeedCmPerSecond = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double YawRadiansPerSecond = 0.0;
    double SteeringAxis = 0.0;     // already speed filtered in the pawn
    double MassKg = 0.0;
    bool ArcadeHandling = true;
    bool OffPavement = false;
    // Actual same-frame requested longitudinal brake from the EXISTING
    // shared lateral-grip budget: service/handbrake, opposite pedal or
    // rear reverse overspeed, as applicable. The default preserves
    // older unbraked source fixtures; Unreal must supply real values.
    double BrakeDecelerationCmPerSecondSquared = 0.0;
    double GripScale = 1.0; // surface * tire * garage * collision grip
};

struct SpeedAwareYawOutput final
{
    bool Valid = false;
    double TargetYawRadiansPerSecond = 0.0;
    double TorqueZ = 0.0;         // Unreal AddTorqueInRadians units
};

// Keep this equal to real Chaos Chassis->SetAngularDamping(). Controller
// compensates part of the existing damping at nonzero desired yaw.
// Limits are provisional tuning knobs, not certified tire physics.
inline constexpr double SteeringChassisAngularDamping = 7.0;
inline constexpr double SteeringEffectiveWheelbaseCm = 260.0;
inline constexpr double SteeringYawResponsePerSecond = 8.0;
inline constexpr double SteeringArcadeLateralAcceleration = 1176.0;
inline constexpr double SteeringSimulationLateralAcceleration = 784.0;
inline constexpr double SteeringTorqueLimitPerMass = 105000.0;

inline SpeedAwareYawOutput CalculateSpeedAwareYawTorque(
    SpeedAwareYawSample s) noexcept
{
    if (!std::isfinite(s.DeltaSeconds) || s.DeltaSeconds <= 0.0 ||
        s.DeltaSeconds > 0.10 ||
        !std::isfinite(s.ForwardSpeedCmPerSecond) ||
        !std::isfinite(s.PlanarSpeedCmPerSecond) ||
        !std::isfinite(s.YawRadiansPerSecond) ||
        !std::isfinite(s.SteeringAxis) ||
        !std::isfinite(s.MassKg) ||
        s.MassKg < 100.0 || s.MassKg > 15000.0 ||
        s.PlanarSpeedCmPerSecond < 0.0 ||
        s.PlanarSpeedCmPerSecond > 15000.0 ||
        std::abs(s.ForwardSpeedCmPerSecond) > s.PlanarSpeedCmPerSecond + 1.0 ||
        std::abs(s.YawRadiansPerSecond) > 12.0 ||
        std::abs(s.SteeringAxis) > 1.0 ||
        !std::isfinite(s.BrakeDecelerationCmPerSecondSquared) ||
        s.BrakeDecelerationCmPerSecondSquared < 0.0 ||
        s.BrakeDecelerationCmPerSecondSquared > 5000.0 ||
        !std::isfinite(s.GripScale) ||
        s.GripScale < 0.05 || s.GripScale > 4.0)
    {
        return {};
    }

    const double maxAngle = s.ArcadeHandling ? 0.36 : 0.30;
    const double roadAccel = s.ArcadeHandling
        ? SteeringArcadeLateralAcceleration
        : SteeringSimulationLateralAcceleration;
    const double lateralLimit = s.OffPavement
        ? roadAccel * 0.65 : roadAccel;

    // Bicycle curvature changes sign when reversing; front-wheel steering
    // should not spin the car in place without longitudinal movement.
    const double geometricYaw = s.ForwardSpeedCmPerSecond *
        std::tan(s.SteeringAxis * maxAngle) /
        SteeringEffectiveWheelbaseCm;
    // Lateral-acceleration-limited cornering prevents arbitrary sideways
    // yaw at full speed. An absolute cap also controls low-speed turn-in.
    const double maximumYaw = std::min(
        s.ArcadeHandling ? 1.4 : 1.05,
        lateralLimit / std::max(200.0, s.PlanarSpeedCmPerSecond));
    // Steering asks for lateral contact grip even when the braking
    // policy is already using part of the same finite friction circle.
    // A hard service/reverse brake must therefore reduce fresh turn-in,
    // not create an identical steering impulse as an unbraked car.
    // Reuse the exact unscaled whole-body brake request and same physical
    // tire/surface/grip budget as CalculateLateralGripForce. The real
    // sprung contact still gates final front steering torque separately.
    const double totalGrip = (s.ArcadeHandling
        ? ArcadeCombinedGripBudget : SimulationCombinedGripBudget) *
        s.GripScale;
    const double usedBrake = std::min(
        s.BrakeDecelerationCmPerSecondSquared, totalGrip);
    const double brakingFraction = usedBrake / totalGrip;
    const double steeringGripFraction = std::sqrt(std::max(
        0.0, 1.0 - brakingFraction * brakingFraction));
    const double target = std::clamp(
        geometricYaw * steeringGripFraction, -maximumYaw, maximumYaw);

    // Approximate yaw inertia of a rigid car body with 380x180 cm plan.
    // This estimate is deliberately explicit until true UE chassis/tire
    // inertia can be calibrated in the editor; no kinematic rotation.
    constexpr double bodyLengthCm = 380.0;
    constexpr double bodyWidthCm = 180.0;
    const double yawInertia = s.MassKg *
        (bodyLengthCm * bodyLengthCm + bodyWidthCm * bodyWidthCm) / 12.0;
    const double accelerationRequest =
        SteeringChassisAngularDamping * target +
        SteeringYawResponsePerSecond * (target - s.YawRadiansPerSecond);
    const double cap = SteeringTorqueLimitPerMass * s.MassKg;
    const double torque = std::clamp(
        yawInertia * accelerationRequest, -cap, cap);
    return {true, target, torque};
}
} // namespace latebrake
