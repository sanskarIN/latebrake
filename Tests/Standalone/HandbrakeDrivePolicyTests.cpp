#include "Vehicles/HandbrakeDrivePolicy.h"
#include "Vehicles/FrameStableDamping.h"
#include <algorithm>
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
    // Signed handbrake yaw regression: a reversing crawl must already
    // steer backwards; there must be no old discontinuous threshold at
    // -75 cm/s. At 900 cm/s and beyond it preserves the previous torque
    // magnitude in either direction.
    using latebrake::SignedHandbrakeYawAuthority;
    Require(Close(SignedHandbrakeYawAuthority(0.0), 0.0) &&
            Close(SignedHandbrakeYawAuthority(-0.0), 0.0),
            "parked chassis cannot acquire handbrake yaw from steering");
    for (double speed : {-900.0, -899.0, -75.001, -75.0, -74.999,
                         -20.0, -0.001, 0.0, 0.001, 20.0, 74.999,
                         75.0, 75.001, 899.0, 900.0})
    {
        const double actual = SignedHandbrakeYawAuthority(speed);
        const double expected = std::clamp(speed / 900.0, -1.0, 1.0);
        Require(Close(actual, expected) &&
                Close(actual, -SignedHandbrakeYawAuthority(-speed)) &&
                actual >= -1.0 && actual <= 1.0,
                "signed yaw authority is smooth, mirror-symmetric and bounded");
    }
    Require(Close(SignedHandbrakeYawAuthority(10000.0), 1.0) &&
            Close(SignedHandbrakeYawAuthority(-10000.0), -1.0),
            "fully moving cars retain their former handbrake yaw magnitude");
    Require(SignedHandbrakeYawAuthority(-0.1) < 0.0 &&
            SignedHandbrakeYawAuthority(-74.999) < 0.0 &&
            SignedHandbrakeYawAuthority(-75.001) < 0.0,
            "reverse direction is consistent even below the old 75 cm/s flip");
    Require(Close(SignedHandbrakeYawAuthority(-74.999),
                  SignedHandbrakeYawAuthority(-75.001), 3e-6),
            "reverse crawl has no discontinuous handbrake yaw impulse");
    for (double invalidSpeed : invalid)
    {
        Require(SignedHandbrakeYawAuthority(invalidSpeed) == 0.0,
                "nonfinite speed cannot generate a handbrake yaw impulse");
    }
    Require(SignedHandbrakeYawAuthority(15000.0001) == 0.0 &&
            SignedHandbrakeYawAuthority(-15000.0001) == 0.0 &&
            SignedHandbrakeYawAuthority(15000.0) == 1.0 &&
            SignedHandbrakeYawAuthority(-15000.0) == -1.0,
            "reject out-of-range measured speed without oversteering");
    // Deterministic dense sweep catches sudden sign or magnitude changes
    // on forward/reverse speed transitions at every low-speed bucket.
    double lastYaw = SignedHandbrakeYawAuthority(-15000.0);
    for (int i = -149999; i <= 150000; ++i)
    {
        const double speed = static_cast<double>(i) / 10.0;
        const double yaw = SignedHandbrakeYawAuthority(speed);
        Require(std::isfinite(yaw) && yaw >= lastYaw &&
                yaw - lastYaw <= 0.000112 &&
                Close(yaw, -SignedHandbrakeYawAuthority(-speed)),
                "dense signed-speed sweep remains monotone and continuous");
        lastYaw = yaw;
    }

    std::cout << "Prototype handbrake policy tests passed.\n";
    return 0;
}
