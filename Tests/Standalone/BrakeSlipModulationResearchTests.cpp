#include "../../Source/Latebrake/Public/Physics/BrakeSlipModulationResearch.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "CHECK failed: %s line %d\n", #x, __LINE__); std::abort(); } } while (false)
using namespace LateBrake::PhysicsResearch;

int main() {
    BrakeSlipObservation observation{};
    observation.vehicleSpeedMps = 20.0;
    observation.wheelSurfaceSpeedMps = {20.0, 20.0, 20.0, 20.0};
    observation.contactValid = {true, true, true, true};
    observation.sampleSeconds = 0.02;
    BrakeSlipCalibration calibration{};
    BrakeSlipState state{};

    auto r = AdvanceBrakeSlip(observation, calibration, state);
    CHECK(r.valid && !r.active);
    for (double pressure : r.next.pressureFraction) CHECK(pressure == 1.0);

    observation.wheelSurfaceSpeedMps[0] = 12.0;
    r = AdvanceBrakeSlip(observation, calibration, r.next);
    CHECK(r.valid && r.active && r.next.releasing[0]);
    CHECK(r.slip[0] > calibration.entrySlip);
    CHECK(r.next.pressureFraction[0] < 1.0);
    CHECK(r.next.pressureFraction[1] == 1.0);
    const double released = r.next.pressureFraction[0];

    observation.wheelSurfaceSpeedMps[0] = 17.0;
    r = AdvanceBrakeSlip(observation, calibration, r.next);
    CHECK(r.valid && r.next.releasing[0]); // Hysteresis holds in middle band.
    CHECK(r.next.pressureFraction[0] < released);

    observation.wheelSurfaceSpeedMps[0] = 19.0;
    r = AdvanceBrakeSlip(observation, calibration, r.next);
    CHECK(r.valid && !r.next.releasing[0]);
    CHECK(r.next.pressureFraction[0] > released);

    observation.wheelSurfaceSpeedMps[0] = 0.0;
    for (int n = 0; n < 100; ++n) {
        r = AdvanceBrakeSlip(observation, calibration, r.next);
        CHECK(r.valid);
        CHECK(r.next.pressureFraction[0] >= calibration.minimumPressureFraction);
        CHECK(r.next.pressureFraction[0] <= 1.0);
    }
    CHECK(r.next.pressureFraction[0] == calibration.minimumPressureFraction);

    observation.contactValid[0] = false;
    r = AdvanceBrakeSlip(observation, calibration, r.next);
    CHECK(r.valid && !r.next.releasing[0]);
    CHECK(r.next.pressureFraction[0] > calibration.minimumPressureFraction);

    observation.vehicleSpeedMps = 1.0;
    observation.contactValid[0] = true;
    r = AdvanceBrakeSlip(observation, calibration, r.next);
    CHECK(r.valid && !r.next.releasing[0] && r.slip[0] == 0.0);

    const auto stable = r.next;
    observation.sampleSeconds = 0.0;
    CHECK(!AdvanceBrakeSlip(observation, calibration, stable).valid);
    observation.sampleSeconds = 0.2;
    CHECK(!AdvanceBrakeSlip(observation, calibration, stable).valid);
    observation.sampleSeconds = 0.02;
    observation.wheelSurfaceSpeedMps[2] = std::numeric_limits<double>::quiet_NaN();
    CHECK(!AdvanceBrakeSlip(observation, calibration, stable).valid);
    observation.wheelSurfaceSpeedMps[2] = 20.0;
    calibration.recoverySlip = calibration.entrySlip;
    CHECK(!AdvanceBrakeSlip(observation, calibration, stable).valid);
    calibration = {};
    // Very large finite slew rates must saturate at physical pressure bounds,
    // not produce NaN or an out-of-range control proposal.
    calibration.releaseRatePerSecond = std::numeric_limits<double>::max();
    calibration.restoreRatePerSecond = std::numeric_limits<double>::max();
    observation.vehicleSpeedMps = 20.0;
    observation.wheelSurfaceSpeedMps[0] = 0.0;
    r = AdvanceBrakeSlip(observation, calibration, stable);
    CHECK(r.valid && r.next.pressureFraction[0] == calibration.minimumPressureFraction);
    CHECK(r.next.pressureFraction[1] == 1.0);
    calibration = {};
    auto corrupt = stable;
    corrupt.pressureFraction[3] = std::numeric_limits<double>::infinity();
    CHECK(!AdvanceBrakeSlip(observation, calibration, corrupt).valid);
    return 0;
}
