#include "UI/TouchPressTracker.h"

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
    std::cout << "Touch press tracker tests passed.\n";
    return 0;
}
