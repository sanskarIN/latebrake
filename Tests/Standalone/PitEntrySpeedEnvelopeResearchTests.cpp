#include "../../Source/Latebrake/Public/Research/PitEntrySpeedEnvelopeResearch.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

using latebrake::research::ComputePitEntryEnvelope;
using latebrake::research::PitEntryStation;

int main() {
    const std::vector<PitEntryStation> flat{{0, 0, 1, 0}, {50, 0, 1, 0}, {100, 0, 1, 0}};
    const auto a = ComputePitEntryEnvelope(flat, 15);
    assert(a.valid && a.speed_limit_mps.size() == 3);
    assert(std::abs(a.speed_limit_mps.back() - 15) < 1e-9);
    assert(a.speed_limit_mps[0] > a.speed_limit_mps[1]);
    assert(a.speed_limit_mps[1] > a.speed_limit_mps[2]);
    const auto lowGrip = ComputePitEntryEnvelope(
        {{0, 0, .2, 0}, {50, 0, .2, 0}, {100, 0, .2, 0}}, 15);
    assert(lowGrip.valid && lowGrip.initial_speed_limit_mps < a.initial_speed_limit_mps);
    const auto downhill = ComputePitEntryEnvelope(
        {{0, 0, 1, -.1}, {50, 0, 1, -.1}, {100, 0, 1, -.1}}, 15);
    assert(downhill.valid && downhill.initial_speed_limit_mps < a.initial_speed_limit_mps);
    const auto corner = ComputePitEntryEnvelope(
        {{0, .05, 1, 0}, {50, 0, 1, 0}, {100, 0, 1, 0}}, 15);
    assert(corner.valid && corner.initial_speed_limit_mps < 15);
    assert(!ComputePitEntryEnvelope({{0, 0, 1, 0}, {0, 0, 1, 0}}, 15).valid);
    assert(!ComputePitEntryEnvelope(flat, std::numeric_limits<double>::quiet_NaN()).valid);
    assert(!ComputePitEntryEnvelope(flat, 15, 0).valid);
    assert(!ComputePitEntryEnvelope({{0, 0, 1, 0}, {100, 0, 1, -0.5}}, 15, .1).valid);
    assert(!ComputePitEntryEnvelope({{0, 0, 1, 0}, {100, 0, 1, 0}}, 15, 1.1).valid);
}
