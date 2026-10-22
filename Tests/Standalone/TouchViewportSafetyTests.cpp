#include "UI/TouchViewportSafety.h"
#include "UI/TouchPressTracker.h"
#include "Vehicles/DriverInputSafety.h"
#include "Vehicles/SteeringFilter.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool good, const char* detail)
{
    if (!good)
    {
        std::cerr << "Touch viewport residual safety FAIL: " << detail << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void AcquireAccelerator(latebrake::TouchDriveState& drive,
                        const latebrake::TouchControlLayout& layout,
                        int finger)
{
    drive.Press(layout, finger,
        layout.Throttle.X + layout.Throttle.Width * 0.5,
        layout.Throttle.Y + layout.Throttle.Height * 0.5);
}
}

int main()
{
    using namespace latebrake;
    TouchViewportGuard viewport;
    TouchDriveState drive;
    TouchPressTracker fingers;
    float smoothGas = 0.f;
    float smoothTurn = 0.f;

    Require(!NeutralizeTouchOnViewportChange(
        viewport, drive, smoothGas, smoothTurn, 1280, 720),
        "first valid viewport establishes baseline");
    const auto wide = CalculateTouchControlLayout(1280, 720);
    Require(wide.Valid, "wide layout is usable");
    Require(fingers.Press(1) && fingers.Press(2),
            "two physical finger presses are distinct");
    AcquireAccelerator(drive, wide, 1);
    drive.Press(wide, 2,
        wide.Steering.X + wide.Steering.Width * 0.85,
        wide.Steering.Y + wide.Steering.Height * 0.5);
    Require(drive.ThrottleActive() && drive.SteeringActive(),
            "gas and steering are simultaneously held");
    smoothGas = static_cast<float>(SmoothThrottle(0., 1., .24));
    smoothTurn = static_cast<float>(SmoothSteering(0., drive.Steering(), .24, 3.8));
    Require(smoothGas > 0.f && smoothTurn > 0.f,
            "ordinary input smoothing builds positive applied commands");

    Require(!NeutralizeTouchOnViewportChange(
        viewport, drive, smoothGas, smoothTurn, 1280, 720) &&
        drive.ThrottleActive() && drive.SteeringActive() &&
        smoothGas > 0.f && smoothTurn > 0.f,
        "unchanged viewport keeps both fingers and smooth drive commands");

    Require(NeutralizeTouchOnViewportChange(
        viewport, drive, smoothGas, smoothTurn, 720, 1280) &&
        !drive.ThrottleActive() && !drive.SteeringActive() &&
        smoothGas == 0.f && smoothTurn == 0.f,
        "rotation cancels held commands AND their residual force/torque");
    Require(fingers.IsDown(1) && fingers.IsDown(2) &&
            !fingers.Press(1) && !fingers.Press(2),
            "old physically held fingers never become new action presses");
    smoothGas = static_cast<float>(SmoothThrottle(
        smoothGas, drive.ThrottleActive() ? 1.0 : 0.0, 1.0 / 60.0));
    smoothTurn = static_cast<float>(SmoothSteering(
        smoothTurn, drive.Steering(), 1.0 / 60.0, 3.8));
    Require(smoothGas == 0.f && smoothTurn == 0.f,
            "subsequent physics tick cannot reapply residual motor/steering");

    const auto portrait = CalculateTouchControlLayout(720, 1280);
    drive.Move(portrait, 1,
        portrait.Throttle.X + 5, portrait.Throttle.Y + 5);
    Require(!drive.ThrottleActive(), "late move from pre-rotation finger is inert");
    fingers.Release(1);
    Require(fingers.Press(1), "fresh release and press is accepted");
    AcquireAccelerator(drive, portrait, 1);
    Require(drive.ThrottleActive(), "new touch controls can be acquired");
    smoothGas = 0.85f;
    smoothTurn = 0.55f;
    Require(NeutralizeTouchOnViewportChange(
        viewport, drive, smoothGas, smoothTurn, 721, 1280) &&
        !drive.ThrottleActive() && smoothGas == 0.f && smoothTurn == 0.f,
        "small inset/split-screen resize clears stored drive impulse");

    fingers.Release(1);
    Require(fingers.Press(1), "press after small resize accepted");
    const auto resized = CalculateTouchControlLayout(721, 1280);
    AcquireAccelerator(drive, resized, 1);
    smoothGas = 0.7f;
    smoothTurn = -0.35f;
    Require(NeutralizeTouchOnViewportChange(
        viewport, drive, smoothGas, smoothTurn, 0, 0) &&
        !viewport.HasBaseline() && smoothGas == 0.f && smoothTurn == 0.f,
        "temporary zero-size surface clears active and smoothed controls");
    Require(fingers.IsDown(1), "invalid viewport does not fake finger release");
    Require(!NeutralizeTouchOnViewportChange(
        viewport, drive, smoothGas, smoothTurn, 1280, 720) &&
        !drive.ThrottleActive() && smoothGas == 0.f,
        "returning to valid screen never replays stale acceleration");
    Require(!fingers.Press(1), "repeated press still suppressed after restore");
    Require(NeutralizeTouchOnViewportChange(
        viewport, drive, smoothGas, smoothTurn, 100, 100),
        "unusable small viewport remains a cancellation condition");
    Require(!viewport.HasBaseline() &&
            smoothGas == 0.f && smoothTurn == 0.f,
            "invalid view cannot retain smoothed steering or throttle");

    std::cout << "Viewport residual-command cancellation tests passed.\n";
    return EXIT_SUCCESS;
}
