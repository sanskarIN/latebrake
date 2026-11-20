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
        if (!finite(s.distance_m) || s.distance_m < 0.0 ||
            !finite(s.curvature_inv_m) ||
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
        // Finite station coordinates can still overflow when subtracted.
        // Reject unusable geometry before evaluating any braking square roots.
        if (!finite(segment_m) || segment_m <= 0.0) return {};
        const double grip = std::min(start.grip, end.grip);
        const double grade = std::min(start.grade_radians, end.grade_radians);
        // Use the least normal load at either endpoint. The most downhill
        // grade is not necessarily the steepest slope (or least cosine).
        const double normal = gravity_mps2 * std::min(
            std::cos(start.grade_radians), std::cos(end.grade_radians));
        // Combined tire force budget: lateral cornering consumes capacity
        // that cannot simultaneously be used for longitudinal braking.
        // Use the maximum sampled curvature throughout the segment and the
        // candidate *entry* speed for a conservative constant-force bound.
        const double curvature = std::max(std::abs(start.curvature_inv_m),
                                          std::abs(end.curvature_inv_m));
        const double capacity = grip * normal;
        const double gravity_decel = gravity_mps2 * std::sin(grade);
        const double segment_lateral_cap = curvature > 0.0
            ? std::sqrt(capacity / curvature)
            : std::numeric_limits<double>::max();
        if (!finite(segment_lateral_cap)) return {};
        // The downstream station must satisfy the whole approaching
        // segment's lateral bound, not only its own sampled curvature.
        result.speed_limit_mps[i] = std::min(result.speed_limit_mps[i], segment_lateral_cap);
        const double constrained_next = result.speed_limit_mps[i];
        // Check the *squared* reachable speed before taking sqrt or min.
        // std::min(max_double, sqrt(infinity)) is still a finite max_double;
        // that silently admits absurd finite-length geometry and can make
        // a later infinity <= infinity feasibility comparison return true.
        const double optimisticDecel =
            braking_efficiency * capacity + std::max(0.0, gravity_decel);
        const double optimisticTravelSquared =
            2.0 * segment_m * optimisticDecel;
        const double reachableSpeedSquared =
            constrained_next * constrained_next + optimisticTravelSquared;
        if (!finite(optimisticTravelSquared) ||
            !finite(reachableSpeedSquared) ||
            reachableSpeedSquared < 0.0) return {};
        const double upper = std::min(
            std::min(lateral_limits[i - 1], segment_lateral_cap),
            std::sqrt(reachableSpeedSquared));
        if (!finite(upper)) return {};
        // Even at zero lateral demand, insufficient downhill deceleration
        // means no feasible backward-reachable braking envelope.
        if (braking_efficiency * capacity + gravity_decel <= 0.0) return {};
        const auto feasible = [&](double speed) {
            const double lateral = curvature * speed * speed;
            const double remaining = std::sqrt(
                std::max(0.0, capacity * capacity - lateral * lateral));
            const double decel = braking_efficiency * remaining + gravity_decel;
            return decel >= 0.0 &&
                   speed * speed <= constrained_next * constrained_next + 2.0 * segment_m * decel;
        };
        double lo = 0.0;
        double hi = upper;
        if (feasible(hi)) {
            lo = hi;
        } else {
            // Feasibility is monotone: demand increases with entry speed
            // while the available braking force decreases.
            for (int iteration = 0; iteration < 64; ++iteration) {
                const double mid = lo + (hi - lo) * 0.5;
                if (feasible(mid)) lo = mid;
                else hi = mid;
            }
        }
        result.speed_limit_mps[i - 1] = lo;
    }
    result.initial_speed_limit_mps = result.speed_limit_mps.front();
    result.valid = true;
    return result;
}
} // namespace latebrake::research
