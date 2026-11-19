#include "../../Source/Latebrake/Public/Physics/BrakeSlipModulationResearch.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "CHECK failed: %s line %d\n", #x, __LINE__); std::abort(); } } while (false)
using namespace LateBrake::PhysicsResearch;

// Regression: hysteresis is per physical wheel and restoring a recovered
// front contact must not change the ongoing rear release.
static void CheckIndependentCorners() {
    BrakeSlipObservation sample{};
    sample.vehicleSpeedMps = 20.0;
    sample.wheelSurfaceSpeedMps = {0.0, 20.0, 20.0, 0.0};
    sample.contactValid = {true, true, true, true};
    sample.sampleSeconds = 0.02;
    const BrakeSlipCalibration calibration{};
    const BrakeSlipState initial{};
    const auto first = AdvanceBrakeSlip(sample, calibration, initial);
    CHECK(first.valid && first.active);
    CHECK(first.next.releasing[0] && first.next.releasing[3]);
    CHECK(!first.next.releasing[1] && !first.next.releasing[2]);
    CHECK(first.next.pressureFraction[0] < 1.0);
    CHECK(first.next.pressureFraction[1] == 1.0);
    CHECK(first.next.pressureFraction[2] == 1.0);
    CHECK(first.next.pressureFraction[3] < 1.0);

    sample.wheelSurfaceSpeedMps[0] = 20.0;
    sample.wheelSurfaceSpeedMps[3] = 15.0; // Still above recovery slip.
    const auto second = AdvanceBrakeSlip(sample, calibration, first.next);
    CHECK(second.valid && second.active);
    CHECK(!second.next.releasing[0] && second.next.releasing[3]);
    CHECK(second.next.pressureFraction[0] > first.next.pressureFraction[0]);
    CHECK(second.next.pressureFraction[3] < first.next.pressureFraction[3]);
    CHECK(second.next.pressureFraction[1] == 1.0);
    CHECK(second.next.pressureFraction[2] == 1.0);

    sample.contactValid[3] = false; // Unsupported corner clears its latch.
    const auto third = AdvanceBrakeSlip(sample, calibration, second.next);
    CHECK(third.valid && !third.active && !third.next.releasing[3]);
    CHECK(third.next.pressureFraction[3] > second.next.pressureFraction[3]);
    CHECK(third.next.pressureFraction[0] > second.next.pressureFraction[0]);
}

// Regression: two equivalent observation cadences integrate to equivalent
// pressure changes until the physical floor, independent of the platform.
static void CheckCadenceAndInvalidReadings() {
    BrakeSlipObservation sample{};
    sample.vehicleSpeedMps = 20.0;
    sample.wheelSurfaceSpeedMps = {0.0, 20.0, 20.0, 20.0};
    sample.contactValid = {true, true, true, true};
    const BrakeSlipCalibration calibration{};
    const BrakeSlipState initial{};

    sample.sampleSeconds = 0.1;
    const auto coarse = AdvanceBrakeSlip(sample, calibration, initial);
    CHECK(coarse.valid && coarse.next.releasing[0]);
    sample.sampleSeconds = 0.02;
    auto fine = initial;
    for (int i = 0; i < 5; ++i) {
        const auto frame = AdvanceBrakeSlip(sample, calibration, fine);
        CHECK(frame.valid);
        fine = frame.next;
    }
    CHECK(std::abs(fine.pressureFraction[0] -
                   coarse.next.pressureFraction[0]) < 1e-12);
    CHECK(fine.pressureFraction[1] == 1.0);

    // A nonfinite sensor must invalidate the whole receipt, even when the
    // wheel has lost contact; never silently reuse a stale corner reading.
    sample.contactValid[3] = false;
    sample.wheelSurfaceSpeedMps[3] = std::numeric_limits<double>::infinity();
    CHECK(!AdvanceBrakeSlip(sample, calibration, fine).valid);
    sample.wheelSurfaceSpeedMps[3] = 0.0;
    sample.vehicleSpeedMps = std::numeric_limits<double>::quiet_NaN();
    CHECK(!AdvanceBrakeSlip(sample, calibration, fine).valid);
    sample.vehicleSpeedMps = 20.0;
    sample.sampleSeconds = std::numeric_limits<double>::infinity();
    CHECK(!AdvanceBrakeSlip(sample, calibration, fine).valid);
    sample.sampleSeconds = 0.02;
    auto broken = fine;
    broken.pressureFraction[1] = -0.1;
    CHECK(!AdvanceBrakeSlip(sample, calibration, broken).valid);
    broken = fine;
    broken.pressureFraction[1] = 1.01;
    CHECK(!AdvanceBrakeSlip(sample, calibration, broken).valid);
}

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

    // The middle-band sample released pressure once more. Recovery must
    // improve on that *immediate* predecessor, not leap past an earlier
    // pressure in a single frame when restore is slower than release.
    const double pressureBeforeRecovery = r.next.pressureFraction[0];
    observation.wheelSurfaceSpeedMps[0] = 19.0;
    r = AdvanceBrakeSlip(observation, calibration, r.next);
    CHECK(r.valid && !r.next.releasing[0]);
    CHECK(r.next.pressureFraction[0] > pressureBeforeRecovery);
    CHECK(r.next.pressureFraction[0] < released);

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
    CheckIndependentCorners();
    CheckCadenceAndInvalidReadings();
    return 0;
}
