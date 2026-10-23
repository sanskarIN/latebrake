#include "UI/TouchDriveState.h"

#include <cstdlib>
#include <iostream>
#include <limits>

static void Check(bool ok, const char* label)
{
    if (!ok) { std::cerr << "Touch drive state FAIL: " << label << '\n'; std::exit(1); }
}
int main()
{
    using namespace latebrake;
    TouchDriveState drive;
    const auto layout = CalculateTouchControlLayout(1280, 720);
    const double center = layout.Steering.X + layout.Steering.Width * 0.5;
    const double sy = layout.Steering.Y + layout.Steering.Height * 0.5;
    const double gasX = layout.Throttle.X + 15;
    const double gasY = layout.Throttle.Y + 15;
    const double brakeY = layout.Brake.Y + 15;

    Check(layout.Valid && !drive.SteeringActive() && !drive.ThrottleActive(),
          "fresh state is neutral");
    // The lower-level state must enforce the same physical touch range as
    // TouchPressTracker, even when called without the Unreal Pawn wrapper.
    drive.Press(layout, -1, center, sy);
    drive.Press(layout, MaxTrackedTouchFingers, gasX, gasY);
    drive.Press(layout, std::numeric_limits<int>::max(), center, sy);
    Check(!drive.SteeringActive() && !drive.ThrottleActive(),
          "out-of-range presses cannot acquire driving inputs");
    drive.Press(layout, MaxTrackedTouchFingers - 1, center, sy);
    Check(drive.SteeringOwner() == MaxTrackedTouchFingers - 1,
          "highest representable finger can own steering");
    drive.Move(layout, std::numeric_limits<int>::max(),
               layout.Steering.X - 100.0, sy);
    Check(drive.Steering() == 0.0,
          "invalid move cannot move valid steering owner");
    drive.Release(MaxTrackedTouchFingers - 1);
    Check(!drive.SteeringActive(), "valid boundary finger can release");
    drive.Press(layout, 1, center, sy);
    drive.Press(layout, 2, gasX, gasY);
    Check(drive.SteeringActive() && drive.ThrottleActive() &&
          drive.SteeringOwner() == 1 && drive.PedalOwner() == 2,
          "two independent fingers steer and accelerate");

    drive.Move(layout, 1, layout.Steering.X - 100, sy);
    Check(drive.Steering() == -1.0 && drive.ThrottleActive(),
          "horizontal steering clamp does not interfere with gas");
    drive.Move(layout, 1, layout.Steering.X + layout.Steering.Width + 100, sy);
    Check(drive.Steering() == 1.0, "right edge clamps to full right");
    drive.Move(layout, 1, center, sy - layout.Steering.Height * 2);
    Check(drive.Steering() == 0.0 && drive.SteeringActive(),
          "leaving steering vertical band neutralizes but keeps ownership");
    drive.Move(layout, 1, center, sy);
    Check(drive.Steering() == 0.0, "return to center remains neutral");

    drive.Move(layout, 2, gasX, brakeY);
    Check(drive.BrakeActive() && !drive.ThrottleActive(),
          "pedal owner may slide from gas into brake");
    drive.Move(layout, 2, gasX,
               (layout.Throttle.Y + layout.Throttle.Height + layout.Brake.Y) / 2.0);
    Check(!drive.BrakeActive() && !drive.ThrottleActive(),
          "neutral pedal gap immediately disables both pedals");
    drive.Move(layout, 2, gasX, gasY);
    Check(drive.ThrottleActive(), "owned finger resumes gas only in correct pad");
    drive.Move(layout, 2, layout.Pause.X + 10, layout.Pause.Y + 10);
    Check(!drive.ThrottleActive() && !drive.BrakeActive() &&
          drive.PedalOwner() == 2, "pedal drag over pause never performs a press action");
    drive.Move(layout, 2, gasX, gasY);
    Check(drive.ThrottleActive(), "pedal press ownership remains stable across neutral drag");

    drive.Press(layout, 3, center, sy);
    drive.Press(layout, 4, gasX, brakeY);
    Check(drive.SteeringOwner() == 1 && drive.PedalOwner() == 2 &&
          drive.ThrottleActive() && !drive.BrakeActive(),
          "extra fingers do not steal held controls");
    drive.Release(3);
    drive.Release(4);
    Check(drive.ThrottleActive(), "unowned fingers cannot release controls");
    drive.Release(1);
    Check(!drive.SteeringActive() && drive.Steering() == 0.0 && drive.ThrottleActive(),
          "steer release leaves independent pedal untouched");
    drive.Release(2);
    Check(!drive.ThrottleActive() && drive.PedalOwner() < 0,
          "pedal release clears throttle immediately");
    drive.Release(2);
    Check(!drive.ThrottleActive(), "duplicate release is safe");

    // A drag into the pad cannot capture a finger whose initial press was elsewhere.
    drive.Press(layout, 5, 5.0, 5.0);
    drive.Move(layout, 5, gasX, gasY);
    Check(!drive.ThrottleActive() && drive.PedalOwner() < 0,
          "touch outside driving pads cannot acquire on move");

    drive.Press(layout, 7, gasX, gasY);
    drive.Press(layout, 8, center, sy);
    Check(drive.ThrottleActive() && drive.SteeringActive(), "controls reacquire after release");
    drive.Move(layout, 7, std::numeric_limits<double>::quiet_NaN(), gasY);
    Check(!drive.ThrottleActive() && drive.PedalOwner() < 0 &&
          drive.SteeringOwner() == 8,
          "NaN owned pedal input cancels only that finger");
    drive.Move(layout, 8, center, std::numeric_limits<double>::infinity());
    Check(!drive.SteeringActive() && drive.Steering() == 0.0,
          "infinite steering input cancels captured finger");

    drive.Press(layout, 10, gasX, gasY);
    drive.Press(layout, 11, layout.Steering.X + 3, sy);
    drive.Move({}, 10, gasX, gasY);
    Check(!drive.ThrottleActive() && drive.PedalOwner() < 0 &&
          !drive.SteeringActive() && drive.SteeringOwner() < 0,
          "invalid viewport layout cancels BOTH steering and pedal");
    drive.Press({}, 11, gasX, gasY);
    Check(drive.PedalOwner() < 0, "invalid viewport cannot acquire gas");

    // Press may be the first input arriving after orientation/viewport
    // invalidation; it must cancel the PREVIOUS finger's controls, not
    // simply reject acquisition for the newly arriving finger.
    drive.Press(layout, 1, gasX, gasY);
    drive.Press(layout, 2, center, sy);
    Check(drive.ThrottleActive() && drive.SteeringActive(),
          "viewport press regression establishes two captured controls");
    drive.Press({}, 3, gasX, gasY);
    Check(!drive.ThrottleActive() && !drive.BrakeActive() &&
          !drive.SteeringActive() && drive.PedalOwner() == -1 &&
          drive.SteeringOwner() == -1 && drive.Steering() == 0.0,
          "invalid-layout press immediately neutralizes all OLD owners");

    drive.Press(layout, 4, gasX, gasY);
    drive.Press(layout, 5, center, sy);
    drive.Press({}, MaxTrackedTouchFingers, gasX, gasY);
    Check(!drive.ThrottleActive() && !drive.SteeringActive(),
          "even an untrackable positive press cancels invalid viewport");
    drive.Press(layout, 6, gasX, gasY);
    drive.Press(layout, 7, center, sy);
    drive.Press({}, -1, gasX, gasY);
    Check(!drive.ThrottleActive() && !drive.SteeringActive(),
          "even an untrackable negative press cancels invalid viewport");

    drive.Press(layout, 8, gasX, gasY);
    drive.Press(layout, 9, center, sy);
    drive.Move({}, -1, gasX, gasY);
    Check(!drive.ThrottleActive() && !drive.SteeringActive(),
          "negative move cannot hide an invalid viewport interruption");

    drive.Move(layout, 1, gasX, gasY);
    drive.Move(layout, 2, center, sy);
    Check(!drive.ThrottleActive() && !drive.SteeringActive(),
          "stale move after viewport cancellation cannot regain ownership");
    drive.Press(layout, -1, gasX, gasY);
    Check(drive.PedalOwner() < 0, "negative finger identifier rejected");

    drive.Press(layout, 12, gasX, gasY);
    drive.Press(layout, 13, center, sy);
    drive.Clear();
    Check(!drive.ThrottleActive() && !drive.BrakeActive() &&
          !drive.SteeringActive() && drive.Steering() == 0.0,
          "pause, menu or app background must neutralize all held controls");
    drive.Move(layout, 12, gasX, gasY);
    drive.Move(layout, 13, center, sy);
    Check(!drive.ThrottleActive() && !drive.SteeringActive(),
          "late move after cancellation cannot re-acquire without press");
    drive.Press(layout, 12, gasX, gasY);
    Check(drive.ThrottleActive(), "new press after foreground can acquire");
    drive.Clear();
    std::cout << "Touch drive state tests passed.\n";
    return 0;
}
