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
    // A curve at the segment exit consumes braking grip even if the
    // entry station itself is straight. Never use full straight-line decel.
    const auto sharedGrip = ComputePitEntryEnvelope(
        {{0, 0, 1, 0}, {100, .025, 1, 0}}, 15);
    const auto straightGrip = ComputePitEntryEnvelope(
        {{0, 0, 1, 0}, {100, 0, 1, 0}}, 15);
    REQUIRE(sharedGrip.valid && straightGrip.valid);
    REQUIRE(sharedGrip.initial_speed_limit_mps < straightGrip.initial_speed_limit_mps);
    REQUIRE(sharedGrip.initial_speed_limit_mps <= std::sqrt(9.80665 / .025));
    // Reversing which endpoint carries curvature must not invent grip.
    const auto entryCurve = ComputePitEntryEnvelope(
        {{0, .025, 1, 0}, {100, 0, 1, 0}}, 15);
    REQUIRE(entryCurve.valid);
    REQUIRE(entryCurve.initial_speed_limit_mps <= sharedGrip.initial_speed_limit_mps + 1e-8);
    const auto zeroCurve = ComputePitEntryEnvelope(
        {{0, 0, 1, 0}, {1, 0, 1, 0}}, 1);
    REQUIRE(zeroCurve.valid && zeroCurve.speed_limit_mps[0] >= zeroCurve.speed_limit_mps[1]);
    // Segment-wide curvature also constrains the downstream station.
    // A straight exit sample cannot erase a sharp preceding bend.
    const auto sharpToStraight = ComputePitEntryEnvelope(
        {{0, .08, 1, 0}, {30, 0, 1, 0}, {130, 0, 1, 0}}, 20);
    REQUIRE(sharpToStraight.valid);
    const double sharpCap = std::sqrt(9.80665 / .08);
    REQUIRE(sharpToStraight.speed_limit_mps[0] <= sharpCap + 1e-9);
    REQUIRE(sharpToStraight.speed_limit_mps[1] <= sharpCap + 1e-9);
    REQUIRE(sharpToStraight.speed_limit_mps[2] <= 20.0);
    // The next straight segment can accelerate only in an independent
    // forward planner; this backward braking advisory never adds speed.
    const auto allStraight = ComputePitEntryEnvelope(
        {{0, 0, 1, 0}, {30, 0, 1, 0}, {130, 0, 1, 0}}, 20);
    REQUIRE(allStraight.valid);
    REQUIRE(sharpToStraight.speed_limit_mps[1] < allStraight.speed_limit_mps[1]);
    // Reverse endpoint ordering must enforce the same segment cap.
    const auto straightToSharp = ComputePitEntryEnvelope(
        {{0, 0, 1, 0}, {30, .08, 1, 0}, {130, 0, 1, 0}}, 20);
    REQUIRE(straightToSharp.valid);
    REQUIRE(straightToSharp.speed_limit_mps[0] <= sharpCap + 1e-9);
    REQUIRE(straightToSharp.speed_limit_mps[1] <= sharpCap + 1e-9);
    // A very small but positive segment must not be rejected as a duplicate
    // station, nor should it produce NaN or infinity in the envelope.
    const double tiny = std::numeric_limits<double>::denorm_min();
    const auto tinyInterval = ComputePitEntryEnvelope(
        {{0, 0, 1, 0}, {tiny, 0, 1, 0}}, 15);
    REQUIRE(tinyInterval.valid);
    REQUIRE(std::isfinite(tinyInterval.initial_speed_limit_mps));
    // An extreme finite distance should fail closed if squaring the
    // reachable speed exceeds the floating-point representation.
    const auto hugeInterval = ComputePitEntryEnvelope(
        {{0, 0, 1, 0}, {std::numeric_limits<double>::max(), 0, 1, 0}}, 15);
    REQUIRE(!hugeInterval.valid);
    // Force magnitudes above sqrt(DBL_MAX) must not overflow when
    // computing the shared lateral/longitudinal friction circle.
    // A tiny but representable segment keeps the speed bound finite.
    const auto highForce = ComputePitEntryEnvelope(
        {{0, 1, 1, 0}, {1e-160, 1, 1, 0}}, 1, 0.8, 1e160);
    REQUIRE(highForce.valid);
    REQUIRE(std::isfinite(highForce.initial_speed_limit_mps));
    REQUIRE(highForce.initial_speed_limit_mps >= 1.0);
    REQUIRE(highForce.initial_speed_limit_mps <= std::sqrt(2.6) + 1e-10);
    return EXIT_SUCCESS;
}
