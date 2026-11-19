#pragma once
// Portable source-only pit-entry advisory; does not command a vehicle.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace latebrake::research {
struct PitEntryStation {
    double distance_m = 0.0;
    double curvature_inv_m = 0.0;
    double grip = 1.0;
    double grade_radians = 0.0; // Positive uphill.
};
struct PitEntryEnvelope {
    bool valid = false;
    std::vector<double> speed_limit_mps;
    double initial_speed_limit_mps = 0.0;
};

// Backward-reachable, curvature- and grade-limited speed envelope.
// Fails closed on invalid or unsupported telemetry; no forward acceleration.
inline PitEntryEnvelope ComputePitEntryEnvelope(
    const std::vector<PitEntryStation>& stations,
    double pit_entry_limit_mps,
    double braking_efficiency = 0.8,
    double gravity_mps2 = 9.80665) {
    PitEntryEnvelope result;
    const auto finite = [](double value) { return std::isfinite(value); };
    if (stations.size() < 2 || !finite(pit_entry_limit_mps) ||
        !finite(braking_efficiency) || !finite(gravity_mps2) ||
        pit_entry_limit_mps <= 0.0 || braking_efficiency <= 0.0 ||
        braking_efficiency > 1.0 || gravity_mps2 <= 0.0) return result;

    std::vector<double> lateral_limits(stations.size());
    for (std::size_t i = 0; i < stations.size(); ++i) {
        const auto& s = stations[i];
        if (!finite(s.distance_m) || !finite(s.curvature_inv_m) ||
            !finite(s.grip) || !finite(s.grade_radians) ||
            s.grip <= 0.0 || s.grip > 3.0 ||
            std::abs(s.grade_radians) > 0.5 ||
            (i > 0 && s.distance_m <= stations[i - 1].distance_m)) return {};
        const double curvature = std::abs(s.curvature_inv_m);
        const double normal_gravity = gravity_mps2 * std::cos(s.grade_radians);
        if (normal_gravity <= 0.0) return {};
        lateral_limits[i] = curvature > 0.0
            ? std::sqrt(s.grip * normal_gravity / curvature)
            : std::numeric_limits<double>::max();
        if (!finite(lateral_limits[i])) return {};
    }

    result.speed_limit_mps.resize(stations.size());
    result.speed_limit_mps.back() = std::min(pit_entry_limit_mps, lateral_limits.back());
    for (std::size_t i = stations.size() - 1; i > 0; --i) {
        const auto& start = stations[i - 1];
        const auto& end = stations[i];
        const double segment_m = end.distance_m - start.distance_m;
        const double grip = std::min(start.grip, end.grip);
        const double grade = std::min(start.grade_radians, end.grade_radians);
        const double normal = gravity_mps2 * std::cos(grade);
        // Downhill gravity opposes braking; uphill gravity assists.
        const double decel = braking_efficiency * grip * normal +
                             gravity_mps2 * std::sin(grade);
        if (!finite(decel) || decel <= 0.0) return {};
        const double next = result.speed_limit_mps[i];
        const double reachable = std::sqrt(next * next + 2.0 * decel * segment_m);
        if (!finite(reachable)) return {};
        result.speed_limit_mps[i - 1] = std::min(lateral_limits[i - 1], reachable);
    }
    result.initial_speed_limit_mps = result.speed_limit_mps.front();
    result.valid = true;
    return result;
}
} // namespace latebrake::research
