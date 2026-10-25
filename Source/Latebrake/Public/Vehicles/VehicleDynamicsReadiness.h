#pragma once

namespace latebrake
{
// Additional Chaos rigid-body values required before an input-authoritative
// car is allowed to advance the clock or apply forces. Unreal only collects
// the live snapshot; this policy is portable C++20 and has no engine headers.
struct VehicleDynamicsSample final
{
    double MassKilograms = 0.0;
    double AngularVelocityXDegrees = 0.0;
    double AngularVelocityYDegrees = 0.0;
    double AngularVelocityZDegrees = 0.0;
};

enum class VehicleDynamicsDecision
{
    Ready,
    InvalidMass,
    InvalidAngularVelocity
};

VehicleDynamicsDecision EvaluateVehicleDynamics(
    const VehicleDynamicsSample& sample) noexcept;

bool AreVehicleDynamicsReady(const VehicleDynamicsSample& sample) noexcept;

const char* VehicleDynamicsDecisionName(VehicleDynamicsDecision decision) noexcept;
} // namespace latebrake
