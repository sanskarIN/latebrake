#include "UI/TouchPressDispatch.h"
#include "UI/TouchPressTracker.h"
#include "UI/TouchDriveState.h"
#include "UI/TouchControlLayout.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "Paused touch release FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    const TouchControlLayout layout = CalculateTouchControlLayout(1280, 720);
    Require(layout.Valid, "landscape touch layout is valid");

    TouchPressTracker presses;
    TouchDriveState drive;
    const double gasX = layout.Throttle.X + 12.0;
    const double gasY = layout.Throttle.Y + 12.0;
    const double steerX = layout.Steering.X + layout.Steering.Width;
    const double steerY = layout.Steering.Y + 12.0;
    const double pauseX = layout.Pause.X + 12.0;
    const double pauseY = layout.Pause.Y + 12.0;

    // First two touches are driving fingers, followed by a pause action
    // finger. A menu toggle must clear driving ownership but keep the
    // *physical* press latch so a held finger cannot trigger another PAUSE.
    Require(AdmitTouchPress(presses, drive, 0, gasX, gasY) ==
                TouchPressAdmission::NewPress,
            "gas receives its first press");
    drive.Press(layout, 0, gasX, gasY);
    Require(AdmitTouchPress(presses, drive, 1, steerX, steerY) ==
                TouchPressAdmission::NewPress,
            "steering receives its first press");
    drive.Press(layout, 1, steerX, steerY);
    Require(drive.ThrottleActive() && drive.SteeringActive(),
            "independent fingers can simultaneously drive");

    Require(AdmitTouchPress(presses, drive, 2, pauseX, pauseY) ==
                TouchPressAdmission::NewPress,
            "PAUSE action is admitted once");
    drive.Clear(); // The actual GameMode neutralizes controls during pause.
    Require(presses.IsDown(0) && presses.IsDown(1) && presses.IsDown(2),
            "pausing does not rearm physically down fingers");
    Require(AdmitTouchPress(presses, drive, 2, pauseX, pauseY) ==
                TouchPressAdmission::Duplicate,
            "held PAUSE finger cannot toggle menu repeatedly");

    // The actual Unreal TouchReleased callback must run WHILE paused. If
    // that binding lacks bExecuteWhenPaused, these physical releases never
    // reach the tracker and subsequent touches cannot reacquire controls.
    presses.Release(0);
    drive.Release(0);
    presses.Release(1);
    drive.Release(1);
    Require(!presses.IsDown(0) && !presses.IsDown(1) &&
            presses.IsDown(2),
            "paused driving-finger lifts free their slots independently");
    Require(AdmitTouchPress(presses, drive, 2, pauseX, pauseY) ==
                TouchPressAdmission::Duplicate,
            "a still-held PAUSE finger remains single-shot");

    presses.Release(2);
    drive.Release(2);
    Require(!presses.IsDown(2),
            "PAUSE finger release is processed before a resume tap");
    Require(AdmitTouchPress(presses, drive, 2, pauseX, pauseY) ==
                TouchPressAdmission::NewPress,
            "after paused release, new PAUSE finger press may resume");
    // Resume clears driving state, but a moved finger may not steal a pad.
    drive.Clear();
    drive.Move(layout, 0, gasX, gasY);
    Require(!drive.ThrottleActive() && !drive.BrakeActive() &&
            !drive.SteeringActive(),
            "late moves from released pre-pause touches never reacquire gas");

    Require(AdmitTouchPress(presses, drive, 0, gasX, gasY) ==
                TouchPressAdmission::NewPress,
            "new gas press following paused lift is no longer stuck");
    drive.Press(layout, 0, gasX, gasY);
    Require(drive.ThrottleActive(), "fresh physical gas press works after resume");
    Require(AdmitTouchPress(presses, drive, 1, steerX, steerY) ==
                TouchPressAdmission::NewPress,
            "new steering press following paused lift is accepted");
    drive.Press(layout, 1, steerX, steerY);
    Require(drive.SteeringActive(), "fresh steering press works after resume");

    // Menu navigation fingers and simultaneous drive fingers are independent.
    // Releasing one must not silently clear the other or duplicate actions.
    presses.Release(2);
    drive.Release(2);
    Require(presses.IsDown(0) && presses.IsDown(1) &&
            drive.ThrottleActive() && drive.SteeringActive(),
            "menu finger release cannot cancel active gas or steering");
    presses.Release(0);
    drive.Release(0);
    Require(!drive.ThrottleActive() && drive.SteeringActive(),
            "gas release does not drop steering ownership");
    presses.Release(1);
    drive.Release(1);
    Require(!drive.SteeringActive() && !presses.IsDown(1),
            "steering release clears last owner");
    presses.Release(1); // Android platforms may deliver duplicate lifts.
    Require(AdmitTouchPress(presses, drive, 1, steerX, steerY) ==
                TouchPressAdmission::NewPress,
            "duplicate release is safe and index can be reused");

    std::cout << "Paused touch release lifecycle tests passed (portable only).\n";
    return EXIT_SUCCESS;
}
