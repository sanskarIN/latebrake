#include "../../Source/Latebrake/Public/Research/ObservedGripFusion.h"
#include <cassert>
#include <cmath>
#include <limits>
using namespace latebrake::research;
int main() {
    ObservedGripFusion fusion;
    auto s = GripWitness{7, 22, 1, 0.01, 1000, 300, 400, 0.01, 0.01, true};
    assert(fusion.Observe(s));
    auto a = fusion.Estimate();
    assert(a.valid && a.samples == 1 && a.excited_samples == 0);
    assert(std::abs(a.demonstrated_grip - 0.5) < 1e-12);
    assert(a.candidate_upper == 1.8);
    assert(!fusion.Observe(s)); // duplicate frame
    s.frame = 2; s.time_s = 0.02; s.slip_ratio = 0.1;
    assert(fusion.Observe(s));
    a = fusion.Estimate();
    assert(a.valid && a.candidate_upper < 1.8 && a.excited_samples == 1);
    s.frame = 3; s.time_s = 0.03; s.road_cell = 23;
    assert(!fusion.Observe(s)); // cross-cell witness
    s.road_cell = 22; s.normal_force_n = 0;
    assert(!fusion.Observe(s));
    s.normal_force_n = 1000;
    s.longitudinal_force_n = std::numeric_limits<double>::quiet_NaN();
    assert(!fusion.Observe(s));
    s.longitudinal_force_n = 300;
    assert(fusion.Observe(s));
    fusion.Reset();
    assert(!fusion.Estimate().valid);
    assert(fusion.Observe(s));
    ObservedGripFusion invalid(0);
    assert(!invalid.Observe(s));
    return 0;
}
