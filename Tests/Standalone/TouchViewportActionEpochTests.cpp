#include "UI/TouchViewportSafety.h"
#include "UI/TouchPressDispatch.h"
#include "UI/TouchPitHold.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool condition, const char* detail)
{
    if (!condition)
    {
        std::cerr << "Touch viewport action epoch: " << detail << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    TouchViewportGuard viewport;
    TouchDriveState drive;
    TouchPressTracker presses;
    TouchPitHold pit;
    float throttle = 0.9f;
    float steering = -0.6f;

    Check(!NeutralizeTouchOnViewportChange(
              viewport, drive, throttle, steering, 1280, 720),
          "first valid landscape viewport establishes baseline");
    const auto landscape = CalculateTouchControlLayout(1280, 720);
    const double gasX = landscape.Throttle.X + 12.0;
    const double gasY = landscape.Throttle.Y + 12.0;
    const double steerX = landscape.Steering.X + 12.0;
    const double steerY = landscape.Steering.Y + 12.0;

    Check(AdmitTouchPress(presses, drive, 1, gasX, gasY) ==
              TouchPressAdmission::NewPress &&
          AdmitTouchPress(presses, drive, 2, steerX, steerY) ==
              TouchPressAdmission::NewPress,
          "two physical driving fingers are independently recorded");
    drive.Press(landscape, 1, gasX, gasY);
    drive.Press(landscape, 2, steerX, steerY);
    Check(drive.ThrottleActive() && drive.SteeringActive(),
          "both owners have previously valid driving input");
    Check(pit.Begin(3, TouchControl::PitService, true),
          "a separate held pit finger begins before the resize");

    const auto portrait = CalculateTouchControlLayout(720, 1280);
    Check(portrait.Valid, "new portrait touch UI is geometrically valid");
    const double recoverX = portrait.Recover.X + 10.0;
    const double recoverY = portrait.Recover.Y + 10.0;
    const bool firstAction = AdmitTouchPress(
        presses, drive, 4, recoverX, recoverY) ==
        TouchPressAdmission::NewPress;
    Check(firstAction, "new RECOVER finger is tracked as physically pressed");
    const bool changed = NeutralizeTouchOnViewportChange(
        viewport, drive, throttle, steering, 720, 1280);
    if (changed) pit.Clear(); // The real pawn also clears PitService.
    const bool allowAction =
        firstAction && !SuppressFreshTouchOnInvalidatedViewport(
            firstAction, changed);
    Check(changed && !allowAction && !drive.ThrottleActive() &&
          !drive.SteeringActive() && throttle == 0.f &&
          steering == 0.f && !pit.IsHeld(),
          "first post-rotate RECOVER tap cannot earn a recovery penalty");
    Check(presses.IsDown(1) && presses.IsDown(2) &&
          presses.IsDown(3) && presses.IsDown(4),
          "viewport cancellation must preserve physical-down one-shot latches");
    Check(AdmitTouchPress(presses, drive, 4, recoverX, recoverY) ==
              TouchPressAdmission::Duplicate,
          "same held RECOVER finger cannot replay action on stable viewport");
    Check(!SuppressFreshTouchOnInvalidatedViewport(false, true),
          "move/repeat is not a newly admitted discrete action");

    presses.Release(4);
    Check(AdmitTouchPress(presses, drive, 4, recoverX, recoverY) ==
              TouchPressAdmission::NewPress,
          "real lift then repress can be a fresh action");
    const bool stable = NeutralizeTouchOnViewportChange(
        viewport, drive, throttle, steering, 720, 1280);
    Check(!stable &&
          !SuppressFreshTouchOnInvalidatedViewport(true, stable),
          "genuine fresh RECOVER press on stable portrait viewport is allowed");

    // The same protection applies to PAUSE/RESTART and all menu/career
    // actions, since the Unreal pawn checks before any action routing.
    for (int i = 0; i < 100000; ++i)
    {
        const bool makeChange = i % 2 == 0;
        const int width = makeChange ? 1280 : 720;
        const int height = makeChange ? 720 : 1280;
        const auto layout = CalculateTouchControlLayout(width, height);
        Check(layout.Valid, "stress fixture stays in supported layout range");
        presses.Release(5);
        const auto admission = AdmitTouchPress(
            presses, drive, 5,
            layout.Pause.X + 8.0, layout.Pause.Y + 8.0);
        Check(admission == TouchPressAdmission::NewPress,
              "new physical finger is admitted exactly once per press");
        const bool resized = NeutralizeTouchOnViewportChange(
            viewport, drive, throttle, steering, width, height);
        const bool suppressed = SuppressFreshTouchOnInvalidatedViewport(
            admission == TouchPressAdmission::NewPress, resized);
        Check(resized == suppressed,
              "fresh race action must be suppressed exactly on resize");
        Check(presses.IsDown(5) &&
              AdmitTouchPress(presses, drive, 5,
                layout.Pause.X + 8.0, layout.Pause.Y + 8.0) ==
                  TouchPressAdmission::Duplicate,
              "no duplicate action even if Unreal repeats a press callback");
    }

    // Unusable/zero-sized screens cannot dispatch actions. When the OS
    // restores a valid viewport, the prior held finger stays latched.
    Check(NeutralizeTouchOnViewportChange(
              viewport, drive, throttle, steering, 0, 0) &&
          SuppressFreshTouchOnInvalidatedViewport(true, true),
          "zero viewport cannot produce a restart/recover action");
    Check(presses.IsDown(5), "invalid screen cannot fake a finger release");
    Check(!NeutralizeTouchOnViewportChange(
              viewport, drive, throttle, steering, 720, 1280),
          "new valid viewport establishes baseline without rearming fingers");
    Check(!presses.Press(5),
          "held pre-invalidity finger cannot act again on the new baseline");

    std::cout << "Touch viewport discrete-action epoch passed (100k transitions).\n";
    return EXIT_SUCCESS;
}
