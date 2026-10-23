#include "UI/TouchPressTracker.h"
#include "UI/TouchPressDispatch.h"

#include <cstdlib>
#include <iostream>
#include <limits>

static void Check(bool ok, const char* description)
{
    if (!ok)
    {
        std::cerr << "Touch press tracker FAIL: " << description << '\n';
        std::exit(1);
    }
}

int main()
{
    latebrake::TouchPressTracker presses;
    Check(presses.Press(0) && presses.IsDown(0), "first press accepted");
    Check(!presses.Press(0), "duplicate press never repeats discrete action");
    Check(presses.Press(1) && presses.IsDown(1), "second finger independent");
    Check(!presses.Press(1), "second finger also single-shot");

    // Leaving a pad or clearing driving controls must not arm another action
    // for an already-held finger. The tracker is not reset on UI actions.
    Check(!presses.Press(0) && !presses.Press(1),
          "moving or changing menus does not produce a second action");
    presses.Release(0);
    Check(!presses.IsDown(0) && presses.IsDown(1),
          "independent release leaves other finger held");
    Check(presses.Press(0), "new press after actual release is permitted");
    presses.Release(0);
    presses.Release(0);
    Check(presses.Press(0), "repeated release is harmless");

    Check(presses.Press(latebrake::TouchPressTracker::MaxFingerCount - 1),
          "last supported touch index accepted");
    Check(!presses.Press(-1) &&
          !presses.Press(latebrake::TouchPressTracker::MaxFingerCount) &&
          !presses.Press(std::numeric_limits<int>::max()),
          "invalid indices cannot trigger actions");
    presses.Release(-1);
    presses.Release(std::numeric_limits<int>::max());
    Check(presses.IsDown(0) && presses.IsDown(1),
          "invalid releases do not affect legitimate touches");

    presses.Reset();
    Check(!presses.IsDown(0) && !presses.IsDown(1) &&
          !presses.IsDown(latebrake::TouchPressTracker::MaxFingerCount - 1),
          "OS interruption clears every touch slot");
    Check(presses.Press(1), "fresh press after focus change accepted");
    // v0.5.23: test the admission API called by the Unreal pawn, not just
    // the lower-level TouchDriveState::Press path. Previously the press
    // tracker discarded duplicate malformed events before safety handling.
    using latebrake::TouchPressAdmission;
    latebrake::TouchPressTracker physical;
    latebrake::TouchDriveState drive;
    const auto layout = latebrake::CalculateTouchControlLayout(1280, 720);
    const double gasX = layout.Throttle.X + 12.0;
    const double gasY = layout.Throttle.Y + 12.0;
    const double steerX = layout.Steering.X + layout.Steering.Width;
    const double steerY = layout.Steering.Y + 12.0;
    Check(layout.Valid, "test layout is usable");

    Check(latebrake::AdmitTouchPress(physical, drive, 2, gasX, gasY) ==
              TouchPressAdmission::NewPress, "first pedal press forwarded");
    drive.Press(layout, 2, gasX, gasY);
    Check(latebrake::AdmitTouchPress(physical, drive, 3, steerX, steerY) ==
              TouchPressAdmission::NewPress, "independent steering press forwarded");
    drive.Press(layout, 3, steerX, steerY);
    Check(drive.ThrottleActive() && drive.Steering() == 1.0,
          "two live controls before duplicate events");

    Check(latebrake::AdmitTouchPress(physical, drive, 2, gasX, gasY) ==
              TouchPressAdmission::Duplicate && drive.ThrottleActive(),
          "valid duplicate suppressed without losing gas");
    Check(latebrake::AdmitTouchPress(
              physical, drive, 2, gasX, std::numeric_limits<double>::quiet_NaN()) ==
              TouchPressAdmission::MalformedDuplicate,
          "malformed duplicate reaches safety cancellation");
    Check(!drive.ThrottleActive() && drive.PedalOwner() == -1 &&
              drive.SteeringOwner() == 3 && drive.Steering() == 1.0,
          "only malformed pedal owner's control canceled");
    Check(physical.IsDown(2) &&
              latebrake::AdmitTouchPress(physical, drive, 2, gasX, gasY) ==
                  TouchPressAdmission::Duplicate,
          "malformed duplicate does not re-arm held finger or action");
    drive.Move(layout, 2, gasX, gasY);
    Check(!drive.ThrottleActive(), "canceled owner cannot reacquire on move");

    Check(latebrake::AdmitTouchPress(
              physical, drive, 3, std::numeric_limits<double>::infinity(), steerY) ==
              TouchPressAdmission::MalformedDuplicate &&
              !drive.SteeringActive() && drive.Steering() == 0.0,
          "malformed steering duplicate neutralized independently");
    Check(physical.IsDown(3), "malformed steering press remains physically held");
    physical.Release(2);
    drive.Release(2);
    Check(latebrake::AdmitTouchPress(physical, drive, 2, gasX, gasY) ==
              TouchPressAdmission::NewPress, "real release permits new gas press");
    drive.Press(layout, 2, gasX, gasY);
    Check(drive.ThrottleActive(), "fresh touch can accelerate again");

    // Malformed FIRST presses still reach ApplyTouch where viewport
    // interruptions and malformed-coordinate handling already occur.
    Check(latebrake::AdmitTouchPress(
              physical, drive, 4, std::numeric_limits<double>::quiet_NaN(), gasY) ==
              TouchPressAdmission::NewPress && physical.IsDown(4),
          "malformed new press forwarded for viewport and geometry checks");
    Check(latebrake::AdmitTouchPress(physical, drive, 4, gasX, gasY) ==
              TouchPressAdmission::Duplicate,
          "valid duplicate cannot turn malformed first press into an action");

    Check(latebrake::AdmitTouchPress(
              physical, drive, 15, std::numeric_limits<double>::infinity(), gasY) ==
              TouchPressAdmission::NewPress, "highest valid finger remains supported");
    Check(latebrake::AdmitTouchPress(
              physical, drive, 15, gasX, std::numeric_limits<double>::infinity()) ==
              TouchPressAdmission::MalformedDuplicate,
          "highest valid held finger accepts fail-safe duplicate handling");
    Check(latebrake::AdmitTouchPress(
              physical, drive, 16, gasX, gasY) ==
              TouchPressAdmission::InvalidFinger &&
              latebrake::AdmitTouchPress(physical, drive, -1, gasX, gasY) ==
              TouchPressAdmission::InvalidFinger &&
              drive.ThrottleActive(),
          "out-of-range fingers cannot disturb a valid captured pedal");

    // A duplicate from a menu/action finger is single-shot even when
    // malformed; canceling an unowned ID leaves independent gas intact.
    Check(latebrake::AdmitTouchPress(physical, drive, 6, 1.0, 1.0) ==
              TouchPressAdmission::NewPress, "action finger is registered");
    Check(latebrake::AdmitTouchPress(
              physical, drive, 6, std::numeric_limits<double>::quiet_NaN(), 1.0) ==
              TouchPressAdmission::MalformedDuplicate &&
              physical.IsDown(6) && drive.ThrottleActive(),
          "malformed action duplicate cannot clear another finger or re-arm");
    physical.Reset();
    drive.Clear();
    Check(latebrake::AdmitTouchPress(physical, drive, 2, gasX, gasY) ==
              TouchPressAdmission::NewPress, "lifecycle reset accepts new input");

    std::cout << "Touch press tracker tests passed.\n";
    return 0;
}
