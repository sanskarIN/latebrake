#include "Vehicles/HandbrakeDrivePolicy.h"
#include "Vehicles/FrameStableDamping.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <limits>

namespace
{
void Require(bool ok, const char* msg)
{
    if (!ok)
    {
        std::cerr << "Handbrake policy: " << msg << '\n';
        std::exit(1);
    }
}
bool Close(double x, double y) { return std::abs(x - y) < 1e-12; }
}

int main()
{
    using latebrake::SelectHandbrakeDriveProfile;
    for (bool arcade : {false, true})
    {
        for (bool service : {false, true})
        {
            for (double throttle : {-1.0, 0.0, 0.75, 1.0})
            {
                const auto normal = SelectHandbrakeDriveProfile(
                    false, service, arcade, 8.5, throttle);
                Require(Close(normal.LateralGripRate, 8.5),
                        "normal handling grip preserved");
                Require(Close(normal.Throttle, throttle), "motor input unchanged");
                Require(normal.Braking == service && !normal.HandbrakeActive,
                        "ordinary brake identity preserved");
                Require(Close(normal.ForwardResistanceRate,
                              service ? 7.0 : 0.55), "existing brake rate retained");
                const auto held = SelectHandbrakeDriveProfile(
                    true, service, arcade, 8.5, throttle);
                Require(held.HandbrakeActive && held.Braking && Close(held.Throttle, 0.0),
                        "handbrake never drives the motor");
                Require(Close(held.LateralGripRate, arcade ? 1.8 : 1.1),
                        "arcade and simulation tuning remain distinct");
                Require(Close(held.ForwardResistanceRate, service ? 7.0 : 4.2),
                        "ordinary brake retains precedence while handbrake held");
            }
        }
    }
    // A missing axis or invalid garage multiplier cannot reach a Chaos force
    // as a nonfinite value even when the handbrake was not requested.
    const double invalid[] = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()
    };
    for (double value : invalid)
    {
        const auto sanitized = SelectHandbrakeDriveProfile(
            false, false, true, value, value);
        Require(Close(sanitized.LateralGripRate, 0.0) &&
                Close(sanitized.Throttle, 0.0),
                "reject nonfinite tuning and throttle");
    }
    Require(Close(SelectHandbrakeDriveProfile(
        false, false, true, -3.0, 0.5).LateralGripRate, 0.0),
        "negative grip cannot generate energy");
    Require(Close(SelectHandbrakeDriveProfile(
        false, false, true, 60.0, 0.5).LateralGripRate, 0.0),
        "unreasonable grip never yields extreme force");
    Require(Close(SelectHandbrakeDriveProfile(
        false, false, false, 2.0, 7.0).Throttle, 1.0),
        "unexpected input never exceeds normalized throttle");
    for (double dt : {1.0 / 240.0, 1.0 / 60.0, 1.0 / 30.0, 0.2, 0.6})
    {
        const auto held = SelectHandbrakeDriveProfile(
            true, false, false, 4.25, 0.8);
        const double grip = latebrake::FrameStableDampingRate(
            held.LateralGripRate, dt);
        const double brake = latebrake::FrameStableDampingRate(
            held.ForwardResistanceRate, dt);
        Require(grip * dt <= 1.0 + 1e-12 &&
                brake * dt <= 1.0 + 1e-12,
                "handbrake resists no more than the measured component per frame");
    }
    std::cout << "Prototype handbrake policy tests passed.\n";
    return 0;
}
