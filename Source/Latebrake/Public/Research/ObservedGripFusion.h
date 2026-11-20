#pragma once
// Portable research-only road grip evidence; never commands a vehicle.
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace latebrake::research {
struct GripWitness {
    std::uint64_t session = 0, road_cell = 0, frame = 0;
    double time_s = 0, normal_force_n = 0;
    double longitudinal_force_n = 0, lateral_force_n = 0;
    double slip_ratio = 0, slip_angle_rad = 0;
    bool stable_contact = false;
};
struct GripEstimate {
    bool valid = false;
    double demonstrated_grip = 0, advisory_lower = 0, candidate_upper = 0;
    unsigned samples = 0, excited_samples = 0;
};
class ObservedGripFusion {
public:
    explicit ObservedGripFusion(double plausible_max = 1.8) noexcept
        : plausible_max_(plausible_max), upper_(plausible_max) {}
    bool Observe(const GripWitness& s) noexcept {
        if (!s.session || !s.road_cell || !s.frame || !s.stable_contact ||
            !std::isfinite(s.time_s) || !std::isfinite(s.normal_force_n) ||
            !std::isfinite(s.longitudinal_force_n) ||
            !std::isfinite(s.lateral_force_n) || !std::isfinite(s.slip_ratio) ||
            !std::isfinite(s.slip_angle_rad) || s.time_s < 0 ||
            s.normal_force_n < 100 || std::abs(s.slip_ratio) > 1 ||
            std::abs(s.slip_angle_rad) > 1 || !std::isfinite(plausible_max_) ||
            plausible_max_ <= 0 || plausible_max_ > 3) return false;
        if (session_ && (s.session != session_ || s.road_cell != road_cell_ ||
                         s.frame <= frame_ || s.time_s <= time_s_)) return false;
        // hypot avoids intermediate force-squared overflow.
        const double demand = std::hypot(s.longitudinal_force_n,
                                         s.lateral_force_n) / s.normal_force_n;
        if (!std::isfinite(demand) || demand > plausible_max_ + 1e-9)
            return false;
        const bool excited = std::abs(s.slip_ratio) >= 0.08 ||
                             std::abs(s.slip_angle_rad) >= 0.06;
        if (!session_) { session_ = s.session; road_cell_ = s.road_cell; }
        frame_ = s.frame;
        time_s_ = s.time_s;
        peak_ = std::max(peak_, demand);
        // An excited tire suggests a candidate ceiling, not a certified
        // physical friction limit. The bound is advisory and never authoritative.
        if (excited) {
            upper_ = std::min(upper_, std::min(plausible_max_,
                                               demand * 1.25 + 0.05));
            ++excited_samples_;
        }
        ++samples_;
        return true;
    }
    GripEstimate Estimate() const noexcept {
        if (!samples_) return {};
        const double lower = peak_ * 0.75;
        if (lower > upper_ + 1e-9) return {};
        return {true, peak_, lower, upper_, samples_, excited_samples_};
    }
    void Reset() noexcept {
        session_ = road_cell_ = frame_ = 0;
        time_s_ = peak_ = 0;
        samples_ = excited_samples_ = 0;
        upper_ = plausible_max_;
    }
private:
    double plausible_max_ = 1.8, upper_ = 1.8, peak_ = 0, time_s_ = 0;
    std::uint64_t session_ = 0, road_cell_ = 0, frame_ = 0;
    unsigned samples_ = 0, excited_samples_ = 0;
};
} // namespace latebrake::research
