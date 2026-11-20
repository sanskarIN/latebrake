#include "../../Source/Latebrake/Public/Physics/BrakeForceAllocationResearch.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "failed %s at %d\n", #x, __LINE__); std::abort(); } } while (false)
using namespace LateBrake::PhysicsResearch;
static bool Near(double a, double b) {
    return std::abs(a - b) <= 1e-8 * std::max(1.0, std::abs(b));
}
int main() {
    LoadTransferInput chassis{};
    chassis.massKg = 1200.0;
    chassis.wheelbaseM = 2.6;
    chassis.frontTrackM = chassis.rearTrackM = 1.55;
    chassis.centerOfMassHeightM = 0.5;
    chassis.frontStaticFraction = chassis.frontRollTransferFraction = 0.55;
    BrakeAllocationInput input{};
    input.loads = EstimateWheelLoads(chassis);
    input.tire = {3000.0, 1.2, 0.1};
    input.preferredFrontFraction = 0.65;
    input.requestedBrakeForceN = 3000.0;
    auto result = AllocateCombinedGripBraking(input);
    CHECK(result.valid && !result.saturated);
    CHECK(Near(result.achievedForceN, 3000.0));
    CHECK(Near(result.brakeForceN[0] + result.brakeForceN[1], 1950.0));
    CHECK(Near(result.brakeForceN[2] + result.brakeForceN[3], 1050.0));
    input.requestedBrakeForceN = 1e6;
    result = AllocateCombinedGripBraking(input);
    CHECK(result.valid && result.saturated && result.unmetForceN > 0.0);
    CHECK(Near(result.achievedForceN + result.unmetForceN, 1e6));
    input.requestedBrakeForceN = 0.0;
    result = AllocateCombinedGripBraking(input);
    CHECK(result.valid && !result.saturated && Near(result.achievedForceN, 0.0));
    input.requestedBrakeForceN = 3000.0;
    input.preferredFrontFraction = 1.0;
    CHECK(Near(AllocateCombinedGripBraking(input).achievedForceN, 3000.0));
    input.preferredFrontFraction = 0.0;
    CHECK(Near(AllocateCombinedGripBraking(input).achievedForceN, 3000.0));
    input.measuredLateralForceN[0] = std::numeric_limits<double>::infinity();
    CHECK(!AllocateCombinedGripBraking(input).valid);
    input.measuredLateralForceN[0] = 0.0;
    input.requestedBrakeForceN = -1.0;
    CHECK(!AllocateCombinedGripBraking(input).valid);
    input.requestedBrakeForceN = 3000.0;
    input.loads.normalN[0] = 0.0;
    CHECK(!AllocateCombinedGripBraking(input).valid);
    // Extreme but finite load receipts must not overflow intermediate
    // products when distributing braking across contact patches.
    input.loads = EstimateWheelLoads(chassis);
    for (double& normal : input.loads.normalN) normal *= 1e300;
    input.loads.frontAxleN *= 1e300;
    input.loads.rearAxleN *= 1e300;
    input.loads.totalN *= 1e300;
    input.loads.minimumWheelN *= 1e300;
    input.tire = {3000.0, 1.0, 0.0};
    input.requestedBrakeForceN = 1e303;
    input.preferredFrontFraction = 0.6;
    result = AllocateCombinedGripBraking(input);
    CHECK(result.valid && !result.saturated);
    CHECK(std::isfinite(result.achievedForceN));
    CHECK(std::abs(result.achievedForceN / 1e303 - 1.0) < 1e-12);
    for (double force : result.brakeForceN) {
        CHECK(std::isfinite(force) && force >= 0.0);
    }
    return 0;
}
