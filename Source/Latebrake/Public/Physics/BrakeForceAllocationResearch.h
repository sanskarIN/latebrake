#pragma once
#include "TireLoadTransferResearch.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace LateBrake::PhysicsResearch {

// Source-only combined-grip brake allocation; never commands Chaos actuators.
// Wheel order: FL, FR, RL, RR. All forces are nonnegative newtons.
struct BrakeAllocationInput {
    WheelLoadEstimate loads{};
    TireLoadSensitivity tire{};
    std::array<double, 4> measuredLateralForceN{};
    double requestedBrakeForceN = 0.0;
    double preferredFrontFraction = 0.0;
};
struct BrakeAllocationResult {
    std::array<double, 4> brakeForceN{};
    double achievedForceN = 0.0;
    double unmetForceN = 0.0;
    bool valid = false;
    bool saturated = false;
};

inline BrakeAllocationResult AllocateCombinedGripBraking(
    const BrakeAllocationInput& input) noexcept {
    BrakeAllocationResult out{};
    if (!ConsistentWheelLoads(input.loads) ||
        !std::isfinite(input.requestedBrakeForceN) ||
        input.requestedBrakeForceN < 0.0 ||
        !std::isfinite(input.preferredFrontFraction) ||
        input.preferredFrontFraction < 0.0 ||
        input.preferredFrontFraction > 1.0) return out;
    std::array<double, 4> capacity{};
    for (std::size_t i = 0; i < capacity.size(); ++i) {
        const double lateral = input.measuredLateralForceN[i];
        const double peak = EstimatePeakLongitudinalForceN(
            input.loads.normalN[i], input.tire);
        if (!std::isfinite(lateral) || !std::isfinite(peak) ||
            peak <= 0.0 || std::abs(lateral) > peak) return out;
        const double fraction = lateral / peak;
        capacity[i] = peak * std::sqrt(std::max(0.0, 1.0 - fraction * fraction));
        if (!std::isfinite(capacity[i])) return out;
    }
    const double front = capacity[0] + capacity[1];
    const double rear = capacity[2] + capacity[3];
    const double total = front + rear;
    if (!std::isfinite(front) || !std::isfinite(rear) ||
        !std::isfinite(total) || total <= 0.0) return out;
    const double target = std::min(input.requestedBrakeForceN, total);
    double f = std::min(front, target * input.preferredFrontFraction);
    double r = std::min(rear, std::max(0.0, target - f));
    f += std::min(front - f, std::max(0.0, target - f - r));
    r += std::min(rear - r, std::max(0.0, target - f - r));
    if (front > 0.0) {
        out.brakeForceN[0] = f * capacity[0] / front;
        out.brakeForceN[1] = f - out.brakeForceN[0];
    }
    if (rear > 0.0) {
        out.brakeForceN[2] = r * capacity[2] / rear;
        out.brakeForceN[3] = r - out.brakeForceN[2];
    }
    for (std::size_t i = 0; i < capacity.size(); ++i) {
        if (!std::isfinite(out.brakeForceN[i]) || out.brakeForceN[i] < 0.0 ||
            out.brakeForceN[i] > capacity[i] +
                1e-10 * std::max(1.0, capacity[i])) return BrakeAllocationResult{};
    }
    out.achievedForceN = f + r;
    out.unmetForceN = std::max(0.0, input.requestedBrakeForceN - out.achievedForceN);
    out.saturated = out.unmetForceN >
        1e-9 * std::max(1.0, input.requestedBrakeForceN);
    out.valid = std::isfinite(out.achievedForceN) &&
                std::isfinite(out.unmetForceN);
    return out.valid ? out : BrakeAllocationResult{};
}
} // namespace LateBrake::PhysicsResearch
