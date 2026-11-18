#include "../../Source/Latebrake/Public/Physics/TireLoadTransferResearch.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace LateBrake::PhysicsResearch;

static bool Near(double a, double b, double tolerance = 1e-8) {
    return std::abs(a - b) <= tolerance * std::max(1.0, std::abs(b));
}

static LoadTransferInput Baseline() {
    LoadTransferInput v{};
    v.massKg = 1200.0;
    v.wheelbaseM = 2.6;
    v.frontTrackM = 1.55;
    v.rearTrackM = 1.55;
    v.centerOfMassHeightM = 0.5;
    v.frontStaticFraction = 0.55;
    v.frontRollTransferFraction = 0.55;
    return v;
}

int main() {
    const auto base = Baseline();
    const auto stationary = EstimateWheelLoads(base);
    assert(stationary.status == LoadTransferStatus::Valid);
    assert(Near(stationary.totalN, base.massKg * base.gravityMps2));
    assert(Near(stationary.normalN[0], stationary.normalN[1]));
    assert(Near(stationary.normalN[2], stationary.normalN[3]));
    assert(Near(stationary.frontAxleN / stationary.totalN, 0.55));

    auto acceleration = base;
    acceleration.longitudinalAccelerationMps2 = 3.0;
    const auto accelerating = EstimateWheelLoads(acceleration);
    assert(accelerating.status == LoadTransferStatus::Valid);
    assert(accelerating.frontAxleN < stationary.frontAxleN);
    assert(accelerating.rearAxleN > stationary.rearAxleN);
    assert(Near(accelerating.frontAxleN + accelerating.rearAxleN,
                accelerating.totalN));

    acceleration.longitudinalAccelerationMps2 = -3.0;
    const auto braking = EstimateWheelLoads(acceleration);
    assert(braking.status == LoadTransferStatus::Valid);
    assert(braking.frontAxleN > stationary.frontAxleN);

    auto corner = base;
    corner.lateralAccelerationMps2 = 5.0;
    const auto left = EstimateWheelLoads(corner);
    assert(left.status == LoadTransferStatus::Valid);
    assert(left.normalN[1] > left.normalN[0]);
    assert(left.normalN[3] > left.normalN[2]);
    assert(Near(left.normalN[0] + left.normalN[1], left.frontAxleN));
    assert(Near(left.normalN[2] + left.normalN[3], left.rearAxleN));
    corner.lateralAccelerationMps2 = -5.0;
    const auto right = EstimateWheelLoads(corner);
    assert(right.status == LoadTransferStatus::Valid);
    assert(Near(right.normalN[0], left.normalN[1]));
    assert(Near(right.normalN[3], left.normalN[2]));

    corner.lateralAccelerationMps2 = 100.0;
    assert(EstimateWheelLoads(corner).status == LoadTransferStatus::WheelLift);

    auto invalid = base;
    invalid.frontTrackM = 0.0;
    assert(EstimateWheelLoads(invalid).status == LoadTransferStatus::InvalidInput);
    invalid = base;
    invalid.massKg = std::numeric_limits<double>::infinity();
    assert(EstimateWheelLoads(invalid).status == LoadTransferStatus::InvalidInput);
    invalid = base;
    invalid.frontRollTransferFraction = 1.1;
    assert(EstimateWheelLoads(invalid).status == LoadTransferStatus::InvalidInput);

    const TireLoadSensitivity tire{3000.0, 1.2, 0.1};
    assert(Near(EstimatePeakLongitudinalForceN(3000.0, tire), 3600.0));
    assert(std::isnan(EstimatePeakLongitudinalForceN(-1.0, tire)));
    assert(std::isnan(EstimatePeakLongitudinalForceN(3000.0, {0.0, 1.2, 0.1})));

    // The limit must not accept a corrupt lateral sensor merely because the
    // analytical bound is evaluated at zero lateral acceleration.
    auto badLateral = base;
    badLateral.lateralAccelerationMps2 =
        std::numeric_limits<double>::quiet_NaN();
    assert(std::isnan(EstimateWheelLiftLateralLimitMps2(badLateral)));
    badLateral.lateralAccelerationMps2 =
        std::numeric_limits<double>::infinity();
    assert(std::isnan(EstimateWheelLiftLateralLimitMps2(badLateral)));

    // Overflow of one contact patch must invalidate the entire receipt.
    auto overflow = base;
    overflow.massKg = 1.0e308;
    overflow.gravityMps2 = 1.0;
    overflow.centerOfMassHeightM = 1.0e308;
    overflow.lateralAccelerationMps2 = 2.0;
    assert(EstimateWheelLoads(overflow).status ==
           LoadTransferStatus::InvalidInput);

    const double lateralLimit = EstimateWheelLiftLateralLimitMps2(base);
    assert(std::isfinite(lateralLimit) && lateralLimit > 0.0);
    auto justInside = base;
    justInside.lateralAccelerationMps2 = lateralLimit * 0.99;
    assert(EstimateWheelLoads(justInside).status == LoadTransferStatus::Valid);
    justInside.lateralAccelerationMps2 = -lateralLimit * 0.99;
    assert(EstimateWheelLoads(justInside).status == LoadTransferStatus::Valid);
    justInside.lateralAccelerationMps2 = lateralLimit * 1.01;
    assert(EstimateWheelLoads(justInside).status == LoadTransferStatus::WheelLift);
    justInside.lateralAccelerationMps2 = -lateralLimit * 1.01;
    assert(EstimateWheelLoads(justInside).status == LoadTransferStatus::WheelLift);
    auto zeroHeight = base;
    zeroHeight.centerOfMassHeightM = 0.0;
    assert(std::isinf(EstimateWheelLiftLateralLimitMps2(zeroHeight)));
    auto invalidLimit = base;
    invalidLimit.massKg = -1.0;
    assert(std::isnan(EstimateWheelLiftLateralLimitMps2(invalidLimit)));
    auto rearward = base;
    rearward.longitudinalAccelerationMps2 = 3.0;
    assert(EstimateWheelLiftLateralLimitMps2(rearward) <
           EstimateWheelLiftLateralLimitMps2(base));
    const double fourTireForce =
        EstimateFourWheelPeakLongitudinalForceN(stationary, tire);
    assert(std::isfinite(fourTireForce) && fourTireForce > 0.0);
    const TireLoadSensitivity loadIndependent{3000.0, 1.2, 0.0};
    assert(Near(EstimateFourWheelPeakLongitudinalForceN(stationary, loadIndependent),
                stationary.totalN * 1.2));
    assert(std::isnan(EstimateFourWheelPeakLongitudinalForceN(
        EstimateWheelLoads(corner), tire)));
    assert(std::isnan(EstimateFourWheelPeakLongitudinalForceN(
        stationary, {0.0, 1.2, 0.1})));
    return 0;
}
