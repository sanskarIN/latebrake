#include "UI/TouchFingerRange.h"
#include "UI/TouchPressTracker.h"
#include "UI/TouchDriveState.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool accepted, const char* label)
{
    if (!accepted)
    {
        std::cerr << "Finger range FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    static_assert(MaxTrackedTouchFingers == TouchPressTracker::MaxFingerCount);
    static_assert(IsTrackableTouchFinger(0));
    static_assert(IsTrackableTouchFinger(MaxTrackedTouchFingers - 1));
    static_assert(!IsTrackableTouchFinger(-1));
    static_assert(!IsTrackableTouchFinger(MaxTrackedTouchFingers));

    const TouchControlLayout layout = CalculateTouchControlLayout(1280, 720);
    Require(layout.Valid, "valid test viewport");
    TouchPressTracker tracker;
    TouchDriveState drive;
    const double steerX = layout.Steering.X + layout.Steering.Width / 2.0;
    const double steerY = layout.Steering.Y + layout.Steering.Height / 2.0;
    const double throttleX = layout.Throttle.X + 10.0;
    const double throttleY = layout.Throttle.Y + 10.0;

    for (const int finger : {-1, MaxTrackedTouchFingers,
                             std::numeric_limits<int>::max()})
    {
        Require(!tracker.Press(finger), "tracker rejects invalid press");
        drive.Press(layout, finger, throttleX, throttleY);
        drive.Press(layout, finger, steerX, steerY);
        Require(!drive.ThrottleActive() && !drive.SteeringActive(),
                "driver rejects invalid press independently");
        tracker.Release(finger);
        drive.Move(layout, finger, throttleX, throttleY);
        Require(!drive.ThrottleActive() && !drive.SteeringActive(),
                "stray move cannot claim drive input");
    }

    const int last = MaxTrackedTouchFingers - 1;
    Require(tracker.Press(last), "last valid finger starts physical session");
    drive.Press(layout, last, throttleX, throttleY);
    Require(drive.PedalOwner() == last && drive.ThrottleActive(),
            "last valid finger owns pedal in both policies");
    tracker.Release(last);
    drive.Release(last);
    Require(!drive.ThrottleActive() && !drive.BrakeActive(),
            "last finger release cancels engine throttle");
    Require(tracker.Press(last), "released last finger may press again");
    drive.Press(layout, last, throttleX, throttleY);
    Require(drive.ThrottleActive(), "last tracked finger may reacquire");
    drive.Move({}, MaxTrackedTouchFingers, throttleX, throttleY);
    Require(!drive.ThrottleActive() && drive.PedalOwner() < 0,
            "invalid viewport clears all drive even with stray positive index");
    tracker.Reset();
    drive.Clear();

    // Invalid viewport is a global input interruption, independent of
    // whether the event itself has a valid finger index.
    for (const int invalid : {-1, MaxTrackedTouchFingers,
                              std::numeric_limits<int>::max()})
    {
        drive.Press(layout, last, throttleX, throttleY);
        Require(drive.ThrottleActive(), "establish gas before invalid press");
        drive.Press({}, invalid, throttleX, throttleY);
        Require(!drive.ThrottleActive() && drive.PedalOwner() < 0,
                "invalid layout press cancels prior owner for any finger");
    }
    drive.Press(layout, last, throttleX, throttleY);
    drive.Move({}, -1, throttleX, throttleY);
    Require(!drive.ThrottleActive() && drive.PedalOwner() < 0,
            "negative move also observes global viewport cancellation");

    // Malformed duplicate from the maximum VALID finger neutralizes
    // only its own pedal, not another finger's steering.
    drive.Press(layout, last, throttleX, throttleY);
    drive.Press(layout, 0, steerX, steerY);
    Require(drive.ThrottleActive() && drive.SteeringOwner() == 0,
            "two owners before malformed maximum-index press");
    drive.Press(layout, last, std::numeric_limits<double>::quiet_NaN(),
                throttleY);
    Require(!drive.ThrottleActive() && drive.PedalOwner() < 0 &&
            drive.SteeringOwner() == 0,
            "malformed max-index duplicate releases pedal only");
    drive.Press(layout, MaxTrackedTouchFingers,
                std::numeric_limits<double>::infinity(), steerY);
    Require(drive.SteeringOwner() == 0,
            "untrackable malformed finger cannot clear another owner");
    drive.Clear();

    std::cout << "Shared 16-finger touch range passed.\n";
    return EXIT_SUCCESS;
}
