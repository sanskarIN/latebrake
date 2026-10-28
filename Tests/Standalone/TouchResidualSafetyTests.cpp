#include "UI/TouchResidualSafety.h"
#include "UI/TouchPressTracker.h"
#include "Vehicles/DriverInputSafety.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool actual, const char* why)
{
    if (!actual)
    {
        std::cerr << "Touch residual safety FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;

    TouchDriveState drive;
    TouchPressTracker physical;
    const auto layout = CalculateTouchControlLayout(1280., 720.);
    Require(layout.Valid, "fixture has valid landscape viewport");
    const double gasX = layout.Throttle.X + 15.;
    const double gasY = layout.Throttle.Y + 15.;
    const double steerX = layout.Steering.X + layout.Steering.Width;
    const double steerY = layout.Steering.Y + 15.;
    const double neutralY =
        (layout.Throttle.Y + layout.Throttle.Height + layout.Brake.Y) / 2.;
    float smoothGas = 0.9f;
    float smoothSteer = 0.7f;

    Require(physical.Press(1) && physical.Press(2),
            "two different down fingers are tracked");
    drive.Press(layout, 1, gasX, gasY);
    drive.Press(layout, 2, steerX, steerY);
    Require(drive.ThrottleActive() && drive.SteeringActive() &&
            drive.PedalOwner() == 1 && drive.SteeringOwner() == 2,
            "pedal and steering are independently captured");

    MoveTouchWithResidualSafety(
        drive, layout, 1, gasX, neutralY, smoothGas, smoothSteer);
    Require(!drive.ThrottleActive() && !drive.BrakeActive() &&
            drive.PedalOwner() == 1 && smoothGas == 0.f &&
            smoothSteer == 0.7f && drive.SteeringActive(),
            "GAS to neutral gap cuts motor force without affecting steering");
    Require(SmoothThrottle(smoothGas, drive.ThrottleActive() ? 1. : 0.,
                           1. / 60.) == 0.,
            "next physics step cannot reapply a stale gas command");

    MoveTouchWithResidualSafety(
        drive, layout, 1, gasX, gasY, smoothGas, smoothSteer);
    Require(drive.ThrottleActive(),
            "same physical pedal can reacquire throttle after neutral drag");
    smoothGas = 0.8f;
    MoveTouchWithResidualSafety(
        drive, layout, 1, gasX, layout.Brake.Y + 10.,
        smoothGas, smoothSteer);
    Require(drive.BrakeActive() && !drive.ThrottleActive() &&
            smoothGas == 0.f && smoothSteer == 0.7f,
            "GAS to BRAKE drag stops residual gas and preserves steering");
    MoveTouchWithResidualSafety(
        drive, layout, 1, gasX, gasY, smoothGas, smoothSteer);
    Require(drive.ThrottleActive(), "brake-to-gas drag still drives under same owner");

    smoothGas = 0.85f;
    // A finger moving back into the new midpoint band immediately clears
    // residual steering, rather than slowly decaying a former full lock.
    const double steerCenter = layout.Steering.X + layout.Steering.Width * 0.5;
    MoveTouchWithResidualSafety(
        drive, layout, 2, steerCenter + layout.Steering.Width * 0.02,
        steerY, smoothGas, smoothSteer);
    Require(drive.SteeringOwner() == 2 && drive.Steering() == 0.0 &&
            smoothSteer == 0.f && drive.ThrottleActive() &&
            smoothGas == 0.85f,
            "center band immediately neutralizes residual turning, not gas");
    MoveTouchWithResidualSafety(
        drive, layout, 2, steerX, steerY, smoothGas, smoothSteer);
    Require(drive.Steering() == 1.0 && drive.ThrottleActive(),
            "steering reacquires outside deadzone without a new finger press");
    smoothSteer = 0.7f;
    MoveTouchWithResidualSafety(
        drive, layout, 2, steerX,
        layout.Steering.Y - 40., smoothGas, smoothSteer);
    Require(drive.SteeringActive() && drive.Steering() == 0. &&
            smoothSteer == 0.f && smoothGas == 0.85f &&
            drive.ThrottleActive(),
            "leaving steering band cuts torque without changing held pedal");
    MoveTouchWithResidualSafety(
        drive, layout, 2, steerX, steerY, smoothGas, smoothSteer);
    Require(drive.Steering() == 1.,
            "steering can return after leaving vertical band");

    smoothSteer = 0.65f;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    MoveTouchWithResidualSafety(
        drive, layout, 1, gasX, nan, smoothGas, smoothSteer);
    Require(drive.PedalOwner() == -1 && !drive.ThrottleActive() &&
            smoothGas == 0.f && drive.SteeringOwner() == 2 &&
            smoothSteer == 0.65f,
            "malformed pedal MOVE cancels only pedal residual");
    Require(physical.IsDown(1) && !physical.Press(1),
            "malformed MOVE must not fake a finger-up and rearm actions");
    smoothGas = 0.11f;
    MoveTouchWithResidualSafety(
        drive, layout, 1, gasX, gasY, smoothGas, smoothSteer);
    Require(!drive.ThrottleActive() && smoothGas == 0.11f,
            "canceled owner cannot acquire GAS via later MOVE");
    smoothGas = 0.f;

    CancelTouchOwnerAndOutput(drive, 99, smoothGas, smoothSteer);
    Require(drive.SteeringOwner() == 2 && smoothSteer == 0.65f,
            "unrelated release cannot zero another finger's smooth steering");
    CancelTouchOwnerAndOutput(drive, 2, smoothGas, smoothSteer);
    physical.Release(2);
    Require(!drive.SteeringActive() && drive.SteeringOwner() == -1 &&
            smoothSteer == 0.f && !physical.IsDown(2),
            "real steering release cancels residual torque and finger latch");
    Require(!drive.ThrottleActive(), "release of steering never invents gas");

    physical.Release(1);
    Require(physical.Press(1), "fresh press after actual lift is admitted");
    drive.Press(layout, 1, gasX, gasY);
    smoothGas = 0.95f;
    smoothSteer = 0.45f;
    CancelTouchOwnerAndOutput(drive, 1, smoothGas, smoothSteer);
    physical.Release(1);
    Require(!drive.ThrottleActive() && smoothGas == 0.f &&
            smoothSteer == 0.45f && !physical.IsDown(1),
            "actual GAS release stops motor without cancelling unrelated axes");

    drive.Press(layout, 3, gasX, gasY);
    drive.Press(layout, 4, steerX, steerY);
    smoothGas = 0.75f;
    smoothSteer = 0.6f;
    MoveTouchWithResidualSafety(
        drive, layout, 4, nan, steerY, smoothGas, smoothSteer);
    Require(drive.SteeringOwner() == -1 && smoothSteer == 0.f &&
            smoothGas == 0.75f && drive.ThrottleActive(),
            "NaN steering MOVE cancels only steering filter");
    MoveTouchWithResidualSafety(
        drive, layout, 3, gasX,
        std::numeric_limits<double>::infinity(), smoothGas, smoothSteer);
    Require(!drive.ThrottleActive() && smoothGas == 0.f,
            "infinite pedal MOVE cancels throttle filter");

    drive.Press(layout, 7, gasX, gasY);
    drive.Press(layout, 8, steerX, steerY);
    smoothGas = 0.77f;
    smoothSteer = -0.5f;
    MoveTouchWithResidualSafety(
        drive, {}, 7, gasX, gasY, smoothGas, smoothSteer);
    Require(!drive.ThrottleActive() && !drive.SteeringActive() &&
            smoothGas == 0.f && smoothSteer == 0.f,
            "invalid viewport globally neutralizes both residual commands");

    std::cout << "Touch residual safety tests passed (no Unreal SDK).\n";
    return EXIT_SUCCESS;
}
