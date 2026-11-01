#include "UI/TouchPitHold.h"
#include "UI/TouchControlLayout.h"
#include "UI/TouchPressTracker.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* what)
{
    if (!ok)
    {
        std::cerr << "TouchPitHold FAIL: " << what << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}
int main()
{
    using namespace latebrake;
    TouchPitHold hold;
    const auto layout = CalculateTouchControlLayout(1280.0, 720.0);
    const auto portrait = CalculateTouchControlLayout(360.0, 640.0);
    Check(layout.Valid && portrait.Valid, "realistic viewports are valid");
    const auto PitCenter = [](const TouchControlLayout& x)
    {
        return CircuitPoint{ x.PitService.X + x.PitService.Width * 0.5,
                             x.PitService.Y + x.PitService.Height * 0.5 };
    };
    const auto p = PitCenter(layout);
    Check(HitTestTouchControl(layout, p.X, p.Y) ==
          TouchControl::PitService, "PIT pad has its own hit region");
    Check(!hold.IsHeld() && hold.FingerOwner() == -1,
          "default is unheld and unowned");
    Check(!hold.Begin(1, TouchControl::Throttle, true) &&
          !hold.Begin(1, TouchControl::PitService, false),
          "only eligible original PIT press can hold");
    Check(!hold.Begin(-1, TouchControl::PitService, true) &&
          !hold.Begin(TouchPressTracker::MaxFingerCount,
                      TouchControl::PitService, true),
          "invalid touch IDs cannot acquire PIT");
    Check(hold.Begin(2, TouchControl::PitService, true) &&
          hold.IsHeld() && hold.FingerOwner() == 2,
          "qualified independent finger captured");
    Check(!hold.Begin(2, TouchControl::PitService, true) &&
          !hold.Begin(3, TouchControl::PitService, true),
          "duplicate and second finger cannot restart hold");
    hold.Move(3, TouchControl::Throttle, false);
    hold.Release(3);
    Check(hold.FingerOwner() == 2, "other fingers never release current hold");
    hold.Move(2, TouchControl::PitService, true);
    Check(hold.IsHeld(), "staying within PIT continues held service");
    hold.Move(2, TouchControl::None, true);
    Check(!hold.IsHeld(), "moving out immediately cancels service hold");
    hold.Move(2, TouchControl::PitService, true);
    Check(!hold.IsHeld(), "dragging back cannot silently re-arm service");
    Check(hold.Begin(4, TouchControl::PitService, true),
          "a genuinely new eligible finger can hold after cancellation");
    hold.Move(4, TouchControl::PitService, false);
    Check(!hold.IsHeld(), "interrupted eligibility cancels held finger");
    Check(hold.Begin(5, TouchControl::PitService, true),
          "new press can hold after interruption");
    hold.Release(5);
    Check(!hold.IsHeld(), "finger-up releases without scoring service");
    Check(hold.Begin(6, TouchControl::PitService, true),
          "new press after actual release can reacquire");
    hold.Clear();
    Check(!hold.IsHeld() && hold.FingerOwner() == -1,
          "focus loss and viewport resize can neutralize hold");
    hold.Clear();
    hold.Release(6);
    Check(!hold.IsHeld(), "repeated reset and release are idempotent");

    const TouchRect pads[] = {
        layout.Steering, layout.Throttle, layout.Brake, layout.Pause,
        layout.Restart, layout.Recover, layout.GhostView, layout.CareerNext,
        layout.CareerStart, layout.MenuBrowse
    };
    for (const auto& other : pads)
    {
        const bool overlap =
            layout.PitService.X < other.X + other.Width &&
            other.X < layout.PitService.X + layout.PitService.Width &&
            layout.PitService.Y < other.Y + other.Height &&
            other.Y < layout.PitService.Y + layout.PitService.Height;
        Check(!overlap, "PIT zone never overlaps steering, pedals or actions");
    }
    const auto portraitCenter = PitCenter(portrait);
    Check(HitTestTouchControl(portrait, portraitCenter.X,
              portraitCenter.Y) == TouchControl::PitService,
          "portrait PIT pad is independently reachable");
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    Check(HitTestTouchControl(layout, nan, p.Y) == TouchControl::None,
          "invalid position cannot acquire PIT");
    TouchPitHold stress;
    for (int frame = 0; frame < 50000; ++frame)
    {
        const int finger = frame % TouchPressTracker::MaxFingerCount;
        Check(stress.Begin(finger, TouchControl::PitService, true),
              "repeated fresh valid press can start hold");
        stress.Move(finger, TouchControl::PitService, true);
        Check(stress.IsHeld(), "valid frame retains exact original owner");
        stress.Release(finger);
        Check(!stress.IsHeld(), "every release cancels before next race frame");
    }
    std::cout << "TouchPitHold passed\n";
    return EXIT_SUCCESS;
}
