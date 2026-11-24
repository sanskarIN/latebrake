#pragma once
#include "BrakeForceAllocationResearch.h"
#include "BrakeSlipModulationResearch.h"
#include <array>
#include <cmath>
#include <cstddef>

namespace LateBrake::PhysicsResearch {

// Advisory-only coupling of the load-sensitive grip envelope and per-wheel
// ABS pressure fractions. Does not command Chaos brakes or certify road grip.
// The allocator is rerun with the unmodulated request; pressure modulation
// can only reduce its per-wheel recommendation, never transfer that released
// demand to another wheel during the same observation.
struct ModulatedBrakeAllocation {
    std::array<double, 4> brakeForceN{};
    double achievedForceN = 0.0;
    double unmetForceN = 0.0;
    bool valid = false;
    bool pressureLimited = false;
};

inline ModulatedBrakeAllocation ApplyBrakeSlipPressure(
    const BrakeAllocationInput& request,
    const BrakeSlipResult& modulation) noexcept {
    ModulatedBrakeAllocation out{};
    if (!modulation.valid) return out;
    const auto allocation = AllocateCombinedGripBraking(request);
    if (!allocation.valid) return out;

    double achieved = 0.0;
    for (std::size_t i = 0; i < out.brakeForceN.size(); ++i) {
        const double pressure = modulation.next.pressureFraction[i];
        if (!std::isfinite(pressure) || pressure < 0.0 || pressure > 1.0)
            return ModulatedBrakeAllocation{};
        const double force = allocation.brakeForceN[i] * pressure;
        if (!std::isfinite(force) || force < 0.0 ||
            force > allocation.brakeForceN[i])
            return ModulatedBrakeAllocation{};
        out.brakeForceN[i] = force;
        // Avoid overflowing an intermediate sum before verifying the result.
        if (force > request.requestedBrakeForceN - achieved)
            return ModulatedBrakeAllocation{};
        achieved += force;
        if (pressure < 1.0 && allocation.brakeForceN[i] > 0.0)
            out.pressureLimited = true;
    }
    out.achievedForceN = achieved;
    out.unmetForceN = request.requestedBrakeForceN - achieved;
    out.valid = std::isfinite(out.unmetForceN) && out.unmetForceN >= 0.0;
    return out.valid ? out : ModulatedBrakeAllocation{};
}
} // namespace LateBrake::PhysicsResearch
