#include "../../Source/Latebrake/Public/Physics/ModulatedBrakeAllocationResearch.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "CHECK failed: %s at %d\n", #x, __LINE__); std::abort(); } } while (false)
using namespace LateBrake::PhysicsResearch;

int main() {
    BrakeAllocationInput input{};
    input.loads.status = LoadTransferStatus::Valid;
    input.loads.normalN = {3000.0, 3000.0, 2000.0, 2000.0};
    input.loads.frontAxleN = 6000.0;
    input.loads.rearAxleN = 4000.0;
    input.loads.totalN = 10000.0;
    input.loads.minimumWheelN = 2000.0;
    input.tire = {2500.0, 1.0, 0.0};
    input.requestedBrakeForceN = 2000.0;
    input.preferredFrontFraction = 0.6;
    BrakeSlipResult pressure{};
    pressure.valid = true;
    pressure.next.pressureFraction = {1.0, 1.0, 1.0, 1.0};

    // The existing tire and load validation must accept the fixture.
    CHECK(ConsistentWheelLoads(input.loads));
    const auto base = AllocateCombinedGripBraking(input);
    CHECK(base.valid);
    const auto unchanged = ApplyBrakeSlipPressure(input, pressure);
    CHECK(unchanged.valid && !unchanged.pressureLimited);
    CHECK(std::abs(unchanged.achievedForceN - base.achievedForceN) < 1e-9);

    pressure.next.pressureFraction = {0.5, 1.0, 0.25, 1.0};
    const auto reduced = ApplyBrakeSlipPressure(input, pressure);
    CHECK(reduced.valid && reduced.pressureLimited);
    double total = 0.0;
    for (std::size_t i = 0; i < 4; ++i) {
        CHECK(reduced.brakeForceN[i] >= 0.0);
        CHECK(reduced.brakeForceN[i] <= base.brakeForceN[i]);
        CHECK(std::abs(reduced.brakeForceN[i] -
                       base.brakeForceN[i] * pressure.next.pressureFraction[i]) < 1e-9);
        total += reduced.brakeForceN[i];
    }
    CHECK(std::abs(total - reduced.achievedForceN) < 1e-9);
    CHECK(std::abs(reduced.achievedForceN + reduced.unmetForceN -
                   input.requestedBrakeForceN) < 1e-9);
    pressure.valid = false;
    CHECK(!ApplyBrakeSlipPressure(input, pressure).valid);
    pressure.valid = true;
    pressure.next.pressureFraction[0] = std::numeric_limits<double>::quiet_NaN();
    CHECK(!ApplyBrakeSlipPressure(input, pressure).valid);
    pressure.next.pressureFraction[0] = 1.01;
    CHECK(!ApplyBrakeSlipPressure(input, pressure).valid);
    pressure.next.pressureFraction[0] = -0.01;
    CHECK(!ApplyBrakeSlipPressure(input, pressure).valid);
    pressure.next.pressureFraction[0] = 1.0;
    input.requestedBrakeForceN = -1.0;
    CHECK(!ApplyBrakeSlipPressure(input, pressure).valid);
    return 0;
}
