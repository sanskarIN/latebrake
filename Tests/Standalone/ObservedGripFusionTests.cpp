#include "../../Source/Latebrake/Public/Research/ObservedGripFusion.h"
#include <cstdlib>
#include <cmath>
#include <limits>
using namespace latebrake::research;
// Assertions must execute in Release builds where NDEBUG disables assert().
#define CHECK(expr) do { if (!(expr)) std::abort(); } while (false)
int main() {
    ObservedGripFusion fusion;
    auto s = GripWitness{7, 22, 1, 0.01, 1000, 300, 400, 0.01, 0.01, true};
    CHECK(fusion.Observe(s));
    auto a = fusion.Estimate();
    CHECK(a.valid && a.samples == 1 && a.excited_samples == 0);
    CHECK(std::abs(a.demonstrated_grip - 0.5) < 1e-12);
    CHECK(a.candidate_upper == 1.8);
    CHECK(!fusion.Observe(s)); // duplicate frame
    s.frame = 2; s.time_s = 0.02; s.slip_ratio = 0.1;
    CHECK(fusion.Observe(s));
    a = fusion.Estimate();
    CHECK(a.valid && a.candidate_upper < 1.8 && a.excited_samples == 1);
    s.frame = 3; s.time_s = 0.03; s.road_cell = 23;
    CHECK(!fusion.Observe(s)); // cross-cell witness
    s.road_cell = 22; s.normal_force_n = 0;
    CHECK(!fusion.Observe(s));
    s.normal_force_n = 1000;
    s.longitudinal_force_n = std::numeric_limits<double>::quiet_NaN();
    CHECK(!fusion.Observe(s));
    s.longitudinal_force_n = 300;
    CHECK(fusion.Observe(s));
    // Later measured force must not be erased by a premature slip ceiling.
    s.frame = 4; s.time_s = 0.04;
    s.longitudinal_force_n = 900; s.lateral_force_n = 0;
    s.slip_ratio = 0.01;
    CHECK(fusion.Observe(s));
    a = fusion.Estimate();
    CHECK(a.valid && std::abs(a.demonstrated_grip - 0.9) < 1e-12);
    CHECK(a.candidate_upper >= a.demonstrated_grip);
    fusion.Reset();
    CHECK(!fusion.Estimate().valid);
    CHECK(fusion.Observe(s));
    ObservedGripFusion invalid(0);
    CHECK(!invalid.Observe(s));
    return 0;
}
