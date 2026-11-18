#include "../../Source/Latebrake/Public/Physics/TireLoadTransferResearch.h"

#include <cstdio>
#include <cstdlib>

// Always evaluate checks in Release/NDEBUG builds; CHECK() disappears there.
#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #condition, __FILE__, __LINE__); \
        std::abort(); \
    } \
} while (false)
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
    CHECK(stationary.status == LoadTransferStatus::Valid);
    CHECK(Near(stationary.totalN, base.massKg * base.gravityMps2));
    CHECK(Near(stationary.normalN[0], stationary.normalN[1]));
    CHECK(Near(stationary.normalN[2], stationary.normalN[3]));
    CHECK(Near(stationary.frontAxleN / stationary.totalN, 0.55));

    auto acceleration = base;
    acceleration.longitudinalAccelerationMps2 = 3.0;
    const auto accelerating = EstimateWheelLoads(acceleration);
    CHECK(accelerating.status == LoadTransferStatus::Valid);
    CHECK(accelerating.frontAxleN < stationary.frontAxleN);
    CHECK(accelerating.rearAxleN > stationary.rearAxleN);
    CHECK(Near(accelerating.frontAxleN + accelerating.rearAxleN,
                accelerating.totalN));

    acceleration.longitudinalAccelerationMps2 = -3.0;
    const auto braking = EstimateWheelLoads(acceleration);
    CHECK(braking.status == LoadTransferStatus::Valid);
    CHECK(braking.frontAxleN > stationary.frontAxleN);

    auto corner = base;
    corner.lateralAccelerationMps2 = 5.0;
    const auto left = EstimateWheelLoads(corner);
    CHECK(left.status == LoadTransferStatus::Valid);
    CHECK(left.normalN[1] > left.normalN[0]);
    CHECK(left.normalN[3] > left.normalN[2]);
    CHECK(Near(left.normalN[0] + left.normalN[1], left.frontAxleN));
    CHECK(Near(left.normalN[2] + left.normalN[3], left.rearAxleN));
    corner.lateralAccelerationMps2 = -5.0;
    const auto right = EstimateWheelLoads(corner);
    CHECK(right.status == LoadTransferStatus::Valid);
    CHECK(Near(right.normalN[0], left.normalN[1]));
    CHECK(Near(right.normalN[3], left.normalN[2]));

    corner.lateralAccelerationMps2 = 100.0;
    CHECK(EstimateWheelLoads(corner).status == LoadTransferStatus::WheelLift);

    auto invalid = base;
    invalid.frontTrackM = 0.0;
    CHECK(EstimateWheelLoads(invalid).status == LoadTransferStatus::InvalidInput);
    invalid = base;
    invalid.massKg = std::numeric_limits<double>::infinity();
    CHECK(EstimateWheelLoads(invalid).status == LoadTransferStatus::InvalidInput);
    invalid = base;
    invalid.frontRollTransferFraction = 1.1;
    CHECK(EstimateWheelLoads(invalid).status == LoadTransferStatus::InvalidInput);

    const TireLoadSensitivity tire{3000.0, 1.2, 0.1};
    CHECK(Near(EstimatePeakLongitudinalForceN(3000.0, tire), 3600.0));
    CHECK(std::isnan(EstimatePeakLongitudinalForceN(-1.0, tire)));
    CHECK(std::isnan(EstimatePeakLongitudinalForceN(3000.0, {0.0, 1.2, 0.1})));

    // The limit must not accept a corrupt lateral sensor merely because the
    // analytical bound is evaluated at zero lateral acceleration.
    auto badLateral = base;
    badLateral.lateralAccelerationMps2 =
        std::numeric_limits<double>::quiet_NaN();
    CHECK(std::isnan(EstimateWheelLiftLateralLimitMps2(badLateral)));
    badLateral.lateralAccelerationMps2 =
        std::numeric_limits<double>::infinity();
    CHECK(std::isnan(EstimateWheelLiftLateralLimitMps2(badLateral)));

    // Overflow of one contact patch must invalidate the entire receipt.
    auto overflow = base;
    overflow.massKg = 1.0e308;
    overflow.gravityMps2 = 1.0;
    overflow.centerOfMassHeightM = 1.0e308;
    overflow.lateralAccelerationMps2 = 2.0;
    CHECK(EstimateWheelLoads(overflow).status ==
           LoadTransferStatus::InvalidInput);

    const double lateralLimit = EstimateWheelLiftLateralLimitMps2(base);
    CHECK(std::isfinite(lateralLimit) && lateralLimit > 0.0);
    auto justInside = base;
    justInside.lateralAccelerationMps2 = lateralLimit * 0.99;
    CHECK(EstimateWheelLoads(justInside).status == LoadTransferStatus::Valid);
    justInside.lateralAccelerationMps2 = -lateralLimit * 0.99;
    CHECK(EstimateWheelLoads(justInside).status == LoadTransferStatus::Valid);
    justInside.lateralAccelerationMps2 = lateralLimit * 1.01;
    CHECK(EstimateWheelLoads(justInside).status == LoadTransferStatus::WheelLift);
    justInside.lateralAccelerationMps2 = -lateralLimit * 1.01;
    CHECK(EstimateWheelLoads(justInside).status == LoadTransferStatus::WheelLift);
    auto zeroHeight = base;
    zeroHeight.centerOfMassHeightM = 0.0;
    CHECK(std::isinf(EstimateWheelLiftLateralLimitMps2(zeroHeight)));
    auto invalidLimit = base;
    invalidLimit.massKg = -1.0;
    CHECK(std::isnan(EstimateWheelLiftLateralLimitMps2(invalidLimit)));
    auto rearward = base;
    rearward.longitudinalAccelerationMps2 = 3.0;
    CHECK(EstimateWheelLiftLateralLimitMps2(rearward) <
           EstimateWheelLiftLateralLimitMps2(base));
    const double fourTireForce =
        EstimateFourWheelPeakLongitudinalForceN(stationary, tire);
    CHECK(std::isfinite(fourTireForce) && fourTireForce > 0.0);
    const TireLoadSensitivity loadIndependent{3000.0, 1.2, 0.0};
    CHECK(Near(EstimateFourWheelPeakLongitudinalForceN(stationary, loadIndependent),
                stationary.totalN * 1.2));
    CHECK(std::isnan(EstimateFourWheelPeakLongitudinalForceN(
        EstimateWheelLoads(corner), tire)));
    CHECK(std::isnan(EstimateFourWheelPeakLongitudinalForceN(
        stationary, {0.0, 1.2, 0.1})));
    // Combined-demand advisory cannot grant more longitudinal grip than
    // the unloaded lateral case. A fully utilized patch has no remainder.
    const std::array<double, 4> noLateral{0.0, 0.0, 0.0, 0.0};
    const double unconstrained =
        EstimateRemainingLongitudinalForceN(stationary, tire, noLateral);
    CHECK(Near(unconstrained, fourTireForce));
    std::array<double, 4> partialLateral{};
    std::array<double, 4> saturatedLateral{};
    for (std::size_t wheel = 0; wheel < partialLateral.size(); ++wheel) {
        const double peak =
            EstimatePeakLongitudinalForceN(stationary.normalN[wheel], tire);
        partialLateral[wheel] = peak * 0.6;
        saturatedLateral[wheel] = peak;
    }
    const double reduced =
        EstimateRemainingLongitudinalForceN(stationary, tire, partialLateral);
    CHECK(Near(reduced, fourTireForce * 0.8));
    CHECK(Near(EstimateRemainingLongitudinalForceN(
        stationary, tire, saturatedLateral), 0.0));
    partialLateral[0] = std::numeric_limits<double>::infinity();
    CHECK(std::isnan(EstimateRemainingLongitudinalForceN(
        stationary, tire, partialLateral)));
    partialLateral[0] = saturatedLateral[0] * 1.01;
    CHECK(std::isnan(EstimateRemainingLongitudinalForceN(
        stationary, tire, partialLateral)));
    CHECK(std::isnan(EstimateRemainingLongitudinalForceN(
        EstimateWheelLoads(corner), tire, noLateral)));
    CHECK(std::isnan(EstimateRemainingLongitudinalForceN(
        stationary, {0.0, 1.2, 0.1}, noLateral)));
    return 0;
}
