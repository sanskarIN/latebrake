#include "UI/GhostTouchPolicy.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool good, const char* why)
{
    if (!good)
    {
        std::cerr << "GhostTouchPolicy FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    GhostTouchRequest valid{
        TouchControl::GhostView, true, true, true, true,
        true, false, false, false, false
    };
    Check(CanToggleGhostFromTouch(valid),
          "new ghost press in green qualified race is allowed");

    for (double width : {240.0, 360.0, 720.0, 1280.0, 1920.0})
    {
        for (double height : {240.0, 640.0, 720.0, 1080.0})
        {
            const auto layout = CalculateTouchControlLayout(width, height);
            Check(layout.Valid, "supported screen has valid layout");
            const double x = layout.GhostView.X + layout.GhostView.Width / 2;
            const double y = layout.GhostView.Y + layout.GhostView.Height / 2;
            Check(HitTestTouchControl(layout, x, y) == TouchControl::GhostView,
                  "ghost pad center maps exclusively to GhostView");
            Check(layout.Recover.Y + layout.Recover.Height < layout.GhostView.Y,
                  "ghost button is below RECOVER with a real neutral gap");
            Check(layout.GhostView.Y + layout.GhostView.Height < layout.Throttle.Y,
                  "ghost button is above GAS with a real neutral gap");
            Check(layout.GhostView.X == layout.Recover.X &&
                  layout.GhostView.Width == layout.Recover.Width,
                  "new pad aligns with right-hand action buttons");
            Check(!layout.Steering.Contains(x, y) &&
                  !layout.Throttle.Contains(x, y) &&
                  !layout.Brake.Contains(x, y) &&
                  !layout.Pause.Contains(x, y) &&
                  !layout.Restart.Contains(x, y) &&
                  !layout.Recover.Contains(x, y) &&
                  !layout.CareerNext.Contains(x, y) &&
                  !layout.CareerStart.Contains(x, y) &&
                  !layout.MenuBrowse.Contains(x, y),
                  "ghost hit zone does not overlap any other control");
            Check(HitTestTouchControl(layout, layout.Throttle.X + 1,
                  layout.Throttle.Y + 1) == TouchControl::Throttle,
                  "GHOST never steals throttle input");
        }
    }

    const auto bad = CalculateTouchControlLayout(
        std::numeric_limits<double>::infinity(), 640.0);
    Check(!bad.Valid && HitTestTouchControl(bad, 0.0, 0.0) == TouchControl::None,
          "corrupt viewport cannot trigger action");

    GhostTouchRequest blocked = valid;
    blocked.FreshPress = false;
    Check(!CanToggleGhostFromTouch(blocked), "drag/held press cannot toggle");
    blocked = valid;
    blocked.HasRaceMode = false;
    Check(!CanToggleGhostFromTouch(blocked), "missing mode blocks touch");
    blocked = valid;
    blocked.CircuitReady = false;
    Check(!CanToggleGhostFromTouch(blocked), "unsafe track blocks touch");
    blocked = valid;
    blocked.PlayerReady = false;
    Check(!CanToggleGhostFromTouch(blocked), "missing driver blocks touch");
    blocked = valid;
    blocked.RaceGreen = false;
    Check(!CanToggleGhostFromTouch(blocked), "countdown blocks touch");
    blocked = valid;
    blocked.MenuOpen = true;
    Check(!CanToggleGhostFromTouch(blocked), "menu owns touch");
    blocked = valid;
    blocked.Paused = true;
    Check(!CanToggleGhostFromTouch(blocked), "paused race blocks touch");
    blocked = valid;
    blocked.FocusSuspended = true;
    Check(!CanToggleGhostFromTouch(blocked), "lost OS focus blocks touch");
    blocked = valid;
    blocked.RaceFinished = true;
    Check(!CanToggleGhostFromTouch(blocked), "results screen blocks touch");

    for (TouchControl other : {TouchControl::None, TouchControl::Steering,
             TouchControl::Throttle, TouchControl::Brake,
             TouchControl::Pause, TouchControl::Restart, TouchControl::Recover,
             TouchControl::CareerNext, TouchControl::CareerStart,
             TouchControl::MenuBrowse})
    {
        blocked = valid;
        blocked.Region = other;
        Check(!CanToggleGhostFromTouch(blocked),
              "all unrelated pads are denied");
    }
    std::cout << "Ghost touch policy and layout passed.\n";
    return EXIT_SUCCESS;
}
