#include "../../Source/Latebrake/Public/Physics/WheelSpeedPlausibilityResearch.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>

using namespace LateBrake::PhysicsResearch;

// Unlike the standard assert() macro, REQUIRE remains executable in Release/RelWithDebInfo
// CTest builds. It also keeps every tested observation used under -Werror.
#define REQUIRE(expression) do { \
    if (!(expression)) { \
        std::fprintf(stderr, "Wheel speed check failed: %s at line %d\n", \
                     #expression, __LINE__); \
        std::abort(); \
    } \
} while (false)


int main() {
    WheelSpeedLimits limits{};
    WheelSpeedSample sample{};
    sample.frameSeconds = 0.02;
    sample.vehicleSpeedMps = 20.0;
    sample.angularSpeedRadPerSecond.fill(60.0);
    sample.rollingRadiusMeters.fill(0.33);
    sample.inContact.fill(true);
    WheelSpeedHistory history{};
    const auto first = ReviewWheelSpeedSample(sample, limits, history);
    REQUIRE(first.valid && first.next.initialized);
    for (std::size_t i = 0; i < 4; ++i) {
        REQUIRE(first.plausible[i]);
        REQUIRE(std::abs(first.surfaceSpeedMps[i] - 19.8) < 1e-9);
    }
    sample.angularSpeedRadPerSecond[0] = 120.0;
    const auto spike = ReviewWheelSpeedSample(sample, limits, first.next);
    REQUIRE(spike.valid && !spike.plausible[0]);
    REQUIRE(spike.plausible[1] && spike.plausible[2] && spike.plausible[3]);
    REQUIRE(spike.next.surfaceSpeedMps[0] == first.next.surfaceSpeedMps[0]);
    sample.angularSpeedRadPerSecond[0] = 61.0;
    const auto recovered = ReviewWheelSpeedSample(sample, limits, spike.next);
    REQUIRE(recovered.valid && recovered.plausible[0]);

    sample.inContact[1] = false;
    const auto airborne = ReviewWheelSpeedSample(sample, limits, recovered.next);
    REQUIRE(airborne.valid && !airborne.plausible[1]);
    sample.inContact[1] = true;
    sample.angularSpeedRadPerSecond[1] = 64.0;
    const auto landed = ReviewWheelSpeedSample(sample, limits, airborne.next);
    REQUIRE(landed.valid && landed.plausible[1]);

    sample.vehicleSpeedMps = 90.0;
    const auto vehicleJump = ReviewWheelSpeedSample(sample, limits, landed.next);
    REQUIRE(!vehicleJump.valid);
    REQUIRE(!vehicleJump.next.initialized);
    sample.vehicleSpeedMps = 20.0;

    sample.angularSpeedRadPerSecond[3] =
        std::numeric_limits<double>::quiet_NaN();
    REQUIRE(!ReviewWheelSpeedSample(sample, limits, landed.next).valid);
    sample.angularSpeedRadPerSecond[3] = 60.0;
    sample.rollingRadiusMeters[2] = 0.0;
    REQUIRE(!ReviewWheelSpeedSample(sample, limits, landed.next).valid);
    sample.rollingRadiusMeters[2] = 0.33;
    sample.frameSeconds = 0.2;
    REQUIRE(!ReviewWheelSpeedSample(sample, limits, landed.next).valid);
    sample.frameSeconds = 0.02;
    limits.maximumWheelAccelerationMps2 = -1.0;
    REQUIRE(!ReviewWheelSpeedSample(sample, limits, landed.next).valid);

    // Reverse wheel spin is physically valid. It must retain its sign in both
    // the public review and the history used for temporal acceleration checks.
    WheelSpeedSample reverse{};
    reverse.frameSeconds = 0.02;
    reverse.vehicleSpeedMps = -12.0;
    reverse.angularSpeedRadPerSecond.fill(-40.0);
    reverse.rollingRadiusMeters.fill(0.30);
    reverse.inContact.fill(true);
    const auto reverseFirst = ReviewWheelSpeedSample(
        reverse, WheelSpeedLimits{}, WheelSpeedHistory{});
    REQUIRE(reverseFirst.valid && reverseFirst.next.initialized);
    for (std::size_t i = 0; i < 4; ++i) {
        REQUIRE(reverseFirst.plausible[i]);
        REQUIRE(std::abs(reverseFirst.surfaceSpeedMps[i] + 12.0) < 1e-9);
        REQUIRE(std::abs(reverseFirst.next.surfaceSpeedMps[i] + 12.0) < 1e-9);
    }

    reverse.angularSpeedRadPerSecond.fill(-39.0);
    const auto reverseNext = ReviewWheelSpeedSample(
        reverse, WheelSpeedLimits{}, reverseFirst.next);
    REQUIRE(reverseNext.valid);
    for (bool plausible : reverseNext.plausible)
        REQUIRE(plausible);

    // A wheel that suddenly reports forward rotation while the vehicle is
    // reversing is suspect, not a reason to poison its previous valid sample.
    reverse.angularSpeedRadPerSecond[0] = 39.0;
    const auto signFlip = ReviewWheelSpeedSample(
        reverse, WheelSpeedLimits{}, reverseNext.next);
    REQUIRE(signFlip.valid && !signFlip.plausible[0]);
    REQUIRE(signFlip.plausible[1] && signFlip.plausible[2] &&
            signFlip.plausible[3]);
    REQUIRE(signFlip.next.surfaceSpeedMps[0] ==
            reverseNext.next.surfaceSpeedMps[0]);

    reverse.angularSpeedRadPerSecond[0] = -39.5;
    const auto signRecovery = ReviewWheelSpeedSample(
        reverse, WheelSpeedLimits{}, signFlip.next);
    REQUIRE(signRecovery.valid && signRecovery.plausible[0]);

    // A controlled physical deceleration, stop and forward acceleration is
    // allowed if each actual measured speed step fits the time/accel limits.
    WheelSpeedHistory changingDirection = reverseFirst.next;
    for (int step = 1; step <= 30; ++step) {
        const double signedSpeed = -12.0 + 0.8 * step;
        reverse.vehicleSpeedMps = signedSpeed;
        reverse.angularSpeedRadPerSecond.fill(signedSpeed / 0.30);
        const auto observation = ReviewWheelSpeedSample(
            reverse, WheelSpeedLimits{}, changingDirection);
        REQUIRE(observation.valid);
        for (std::size_t i = 0; i < 4; ++i) {
            REQUIRE(observation.plausible[i]);
            REQUIRE(std::abs(observation.surfaceSpeedMps[i] -
                             signedSpeed) < 1e-9);
        }
        changingDirection = observation.next;
    }
    REQUIRE(changingDirection.vehicleSpeedMps > 0.0);

    // The negative side of the maximum rotational-surface-speed bound is
    // just as strict as the existing positive overspeed rejection.
    reverse.angularSpeedRadPerSecond[2] = -500.0;
    REQUIRE(!ReviewWheelSpeedSample(
        reverse, WheelSpeedLimits{}, changingDirection).valid);
    reverse.angularSpeedRadPerSecond[2] =
        -std::numeric_limits<double>::infinity();
    REQUIRE(!ReviewWheelSpeedSample(
        reverse, WheelSpeedLimits{}, changingDirection).valid);
    return 0;
}
