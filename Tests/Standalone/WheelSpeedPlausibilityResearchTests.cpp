#include "../../Source/Latebrake/Public/Physics/WheelSpeedPlausibilityResearch.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>

using namespace LateBrake::PhysicsResearch;

// Unlike REQUIRE(), REQUIRE remains executable in Release/RelWithDebInfo
// CTest builds. It also keeps every tested observation used under -Werror.
#define REQUIRE(expression) do { \
    if (!(expression)) { \
        std::fprintf(stderr, "Wheel speed check failed: %s at line %d\\n", \
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
    return 0;
}
