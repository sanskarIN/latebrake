#include "Vehicles/VehicleDynamicsReadiness.h"

#include <cmath>

namespace latebrake
{
VehicleDynamicsDecision EvaluateVehicleDynamics(
    const VehicleDynamicsSample& sample) noexcept
{
    // A zero, NaN, infinite or negative mass cannot produce safe acceleration
    // from the chassis's forces (which are tuned for a physical rigid body).
    if (!std::isfinite(sample.MassKilograms) || sample.MassKilograms <= 0.0)
    {
        return VehicleDynamicsDecision::InvalidMass;
    }
    // Finite linear velocity is insufficient: Chaos angular velocity may
    // independently become nonfinite after unstable collisions/constraints.
    if (!std::isfinite(sample.AngularVelocityXDegrees) ||
        !std::isfinite(sample.AngularVelocityYDegrees) ||
        !std::isfinite(sample.AngularVelocityZDegrees))
    {
        return VehicleDynamicsDecision::InvalidAngularVelocity;
    }
    return VehicleDynamicsDecision::Ready;
}

bool AreVehicleDynamicsReady(const VehicleDynamicsSample& sample) noexcept
{
    return EvaluateVehicleDynamics(sample) == VehicleDynamicsDecision::Ready;
}

const char* VehicleDynamicsDecisionName(VehicleDynamicsDecision decision) noexcept
{
    switch (decision)
    {
    case VehicleDynamicsDecision::Ready: return "ready";
    case VehicleDynamicsDecision::InvalidMass: return "invalid-mass";
    case VehicleDynamicsDecision::InvalidAngularVelocity:
        return "invalid-angular-velocity";
    }
    return "unknown";
}
} // namespace latebrake
