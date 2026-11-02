#include "Vehicles/SpeedAwareYawTorque.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* text)
{
    if (!condition)
    {
        std::cerr << "SpeedAwareYawTorque FAIL: " << text << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Close(double a, double b, double tolerance = 1.0e-7)
{
    return std::abs(a-b) <= tolerance;
}
}

int main()
{
    using namespace latebrake;
    static_assert(SteeringEffectiveWheelbaseCm >= 250.0);
    static_assert(SteeringTorqueLimitPerMass > 0.0);
    auto calc = [](double forward, double yaw, double steer,
                   bool arcade = true, bool offRoad = false,
                   double planar = -1.0, double dt = 1.0 / 60.0)
    {
        return CalculateSpeedAwareYawTorque({
            dt, forward, planar < 0.0 ? std::abs(forward) : planar,
            yaw, steer, 1100.0, arcade, offRoad});
    };
    const auto parked = calc(0.0, 0.0, 1.0);
    Check(parked.Valid && parked.TargetYawRadiansPerSecond == 0.0 &&
          parked.TorqueZ == 0.0, "cannot turn rigid chassis in place");
    const auto low = calc(400.0, 0.0, 0.7);
    const auto cruising = calc(2000.0, 0.0, 0.7);
    const auto fast = calc(6000.0, 0.0, 0.7);
    Check(low.Valid && cruising.Valid && fast.Valid &&
          low.TargetYawRadiansPerSecond > 0.0 &&
          cruising.TargetYawRadiansPerSecond > 0.0 &&
          fast.TargetYawRadiansPerSecond > 0.0,
          "responsive turn-in at actual measured driving speeds");
    Check(fast.TargetYawRadiansPerSecond <
              cruising.TargetYawRadiansPerSecond &&
          fast.TargetYawRadiansPerSecond <=
              SteeringArcadeLateralAcceleration / 6000.0 + 1e-9,
          "faster corners have appropriately limited yaw rate");
    Check(cruising.TorqueZ > 0.0, "actual forward turn has positive Z torque");

    const auto left = calc(2000.0, 0.0, -0.7);
    const auto backwards = calc(-2000.0, 0.0, 0.7);
    Check(Close(cruising.TargetYawRadiansPerSecond,
                -left.TargetYawRadiansPerSecond) &&
          Close(cruising.TorqueZ, -left.TorqueZ),
          "mirror-symmetric left and right steering");
    Check(Close(backwards.TargetYawRadiansPerSecond,
                -cruising.TargetYawRadiansPerSecond),
          "reverse steering turns opposite forward travel");

    const auto onCourse = calc(2500.0, 0.0, 1.0, true, false);
    const auto offCourse = calc(2500.0, 0.0, 1.0, true, true);
    const auto simulation = calc(2500.0, 0.0, 1.0, false, false);
    Check(offCourse.Valid && simulation.Valid &&
          offCourse.TargetYawRadiansPerSecond <
              onCourse.TargetYawRadiansPerSecond &&
          simulation.TargetYawRadiansPerSecond <
              onCourse.TargetYawRadiansPerSecond,
          "grass and simulation mode reduce permitted lateral turn");

    const auto sameYaw = calc(2000.0,
          cruising.TargetYawRadiansPerSecond, 0.7);
    const auto tooMuchYaw = calc(2000.0, 1.5, 0.7);
    Check(sameYaw.Valid && tooMuchYaw.Valid &&
          sameYaw.TorqueZ < cruising.TorqueZ &&
          tooMuchYaw.TorqueZ < 0.0,
          "observed rotation damps overshoot instead of unbounded turning");
    const auto counterSteer = calc(1000.0, -0.9, 0.6);
    Check(counterSteer.Valid && counterSteer.TorqueZ > 0.0,
          "opposite slip preserves active recovery steering");
    const auto neutralSpin = calc(1500.0, 0.7, 0.0);
    Check(neutralSpin.Valid && neutralSpin.TorqueZ < 0.0,
          "neutral steering damps existing unwanted spin");
    Check(std::abs(calc(1000.0, -8.0, 1.0).TorqueZ) <=
              SteeringTorqueLimitPerMass * 1100.0,
          "rapid spin correction must never exceed bounded torque budget");

    const auto invalid = CalculateSpeedAwareYawTorque({
        0.016, 2000.0, 2000.0,
        std::numeric_limits<double>::quiet_NaN(),
        1.0, 1100.0, true, false});
    Check(!invalid.Valid && invalid.TorqueZ == 0.0,
          "corrupt angular velocity never enables motor torque");
    Check(!calc(1200.0, 0.0, 1.1).Valid &&
          !calc(1200.0, 0.0, 1.0, true, false, 1200.0, 0.5).Valid &&
          !calc(1200.0, 0.0, 1.0, true, false, 500.0).Valid,
          "invalid axis, hitch or impossible speed projection rejected");
    Check(!CalculateSpeedAwareYawTorque({
        0.016, 1000.0, 1000.0, 0.0, 0.5, -1.0, true, false}).Valid,
          "invalid mass cannot authorize yaw torque");
    Check(Close(calc(2000.0, 0.0, 0.7, true, false, -1.0, 1.0/30.0)
                    .TargetYawRadiansPerSecond,
                cruising.TargetYawRadiansPerSecond),
          "target yaw profile independent of normal frame cadence");
    std::cout << "Speed/rotation-aware steering torque validated.\n";
    return EXIT_SUCCESS;
}
