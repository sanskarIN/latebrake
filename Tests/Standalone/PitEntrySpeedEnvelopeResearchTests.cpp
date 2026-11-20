#include "../../Source/Latebrake/Public/Research/PitEntrySpeedEnvelopeResearch.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

using latebrake::research::ComputePitEntryEnvelope;
using latebrake::research::PitEntryStation;

#define REQUIRE(condition) do { if (!(condition)) { \
    std::cerr << "FAILED " << __FILE__ << ':' << __LINE__ << ": " #condition << '\n'; \
    return EXIT_FAILURE; \
} } while (false)

int main() {
    const std::vector<PitEntryStation> flat{{0, 0, 1, 0}, {50, 0, 1, 0}, {100, 0, 1, 0}};
    const auto a = ComputePitEntryEnvelope(flat, 15);
    REQUIRE(a.valid && a.speed_limit_mps.size() == 3);
    REQUIRE(std::abs(a.speed_limit_mps.back() - 15) < 1e-9);
    REQUIRE(a.speed_limit_mps[0] > a.speed_limit_mps[1]);
    REQUIRE(a.speed_limit_mps[1] > a.speed_limit_mps[2]);
    const auto lowGrip = ComputePitEntryEnvelope(
        {{0, 0, .2, 0}, {50, 0, .2, 0}, {100, 0, .2, 0}}, 15);
    REQUIRE(lowGrip.valid && lowGrip.initial_speed_limit_mps < a.initial_speed_limit_mps);
    const auto downhill = ComputePitEntryEnvelope(
        {{0, 0, 1, -.1}, {50, 0, 1, -.1}, {100, 0, 1, -.1}}, 15);
    REQUIRE(downhill.valid && downhill.initial_speed_limit_mps < a.initial_speed_limit_mps);
    const auto corner = ComputePitEntryEnvelope(
        {{0, .05, 1, 0}, {50, 0, 1, 0}, {100, 0, 1, 0}}, 15);
    REQUIRE(corner.valid && corner.initial_speed_limit_mps < 15);
    REQUIRE(!ComputePitEntryEnvelope({{0, 0, 1, 0}, {0, 0, 1, 0}}, 15).valid);
    REQUIRE(!ComputePitEntryEnvelope(flat, std::numeric_limits<double>::quiet_NaN()).valid);
    REQUIRE(!ComputePitEntryEnvelope(flat, 15, 0).valid);
    REQUIRE(!ComputePitEntryEnvelope({{0, 0, 1, 0}, {100, 0, 1, -0.5}}, 15, .1).valid);
    REQUIRE(!ComputePitEntryEnvelope({{0, 0, 1, 0}, {100, 0, 1, 0}}, 15, 1.1).valid);
    REQUIRE(!ComputePitEntryEnvelope({{-1, 0, 1, 0}, {100, 0, 1, 0}}, 15).valid);
    REQUIRE(!ComputePitEntryEnvelope({{0, 0, 1, 0}, {100, 0, 1, 0}}, 15,
                                     0.8, std::numeric_limits<double>::infinity()).valid);
    // Opposite-signed grades: use the steeper uphill endpoint's lower normal
    // load, even though the downhill endpoint determines gravity assistance.
    const auto mixed = ComputePitEntryEnvelope(
        {{0, 0, 1, 0.4}, {100, 0, 1, -0.1}}, 15);
    const auto downhillOnly = ComputePitEntryEnvelope(
        {{0, 0, 1, -0.1}, {100, 0, 1, -0.1}}, 15);
    REQUIRE(mixed.valid && downhillOnly.valid);
    REQUIRE(mixed.initial_speed_limit_mps < downhillOnly.initial_speed_limit_mps);
    const double expectedDecel = 0.8 * 9.80665 * std::cos(0.4)
                                 - 9.80665 * std::sin(0.1);
    REQUIRE(std::abs(mixed.initial_speed_limit_mps * mixed.initial_speed_limit_mps
                     - (15.0 * 15.0 + 200.0 * expectedDecel)) < 1e-7);
    const auto zeroCurve = ComputePitEntryEnvelope(
        {{0, 0, 1, 0}, {1, 0, 1, 0}}, 1);
    REQUIRE(zeroCurve.valid && zeroCurve.speed_limit_mps[0] >= zeroCurve.speed_limit_mps[1]);
    return EXIT_SUCCESS;
}
