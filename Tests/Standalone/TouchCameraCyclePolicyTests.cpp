#include "Vehicles/TouchCameraCyclePolicy.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool success, const char* why)
{
    if (!success)
    {
        std::cerr << "TouchCameraCyclePolicy FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    CameraTouchRequest ok{
        TouchControl::MenuBrowse, true, true, true, true,
        false, false, false
    };
    Check(CanCycleCameraFromTouch(ok), "fresh CAM in ready race permitted");
    for (auto xy : {240.0, 640.0, 1920.0})
    {
        const auto layout = CalculateTouchControlLayout(xy, xy);
        Check(layout.Valid, "valid compact/tall/square geometry");
        const double x = layout.MenuBrowse.X + layout.MenuBrowse.Width * 0.5;
        const double y = layout.MenuBrowse.Y + layout.MenuBrowse.Height * 0.5;
        Check(HitTestTouchControl(layout, x, y) == TouchControl::MenuBrowse,
              "visible original MenuBrowse pad is exact racing CAM region");
        Check(!layout.Steering.Contains(x, y) &&
              !layout.Throttle.Contains(x, y) &&
              !layout.Brake.Contains(x, y) &&
              !layout.Pause.Contains(x, y) &&
              !layout.Restart.Contains(x, y) &&
              !layout.Recover.Contains(x, y) &&
              !layout.CareerNext.Contains(x, y) &&
              !layout.CareerStart.Contains(x, y),
              "CAM center is not inside driving/critical action pads");
    }
    const auto badLayout = CalculateTouchControlLayout(
        std::numeric_limits<double>::quiet_NaN(), 360.0);
    Check(!badLayout.Valid, "bad geometry rejects all touch regions");

    ok.FreshPress = false;
    Check(!CanCycleCameraFromTouch(ok),
          "touch dragging/repeat is never a camera action");
    ok.FreshPress = true;
    ok.MenuOpen = true;
    Check(!CanCycleCameraFromTouch(ok),
          "menu BROWSE must not change racing camera");
    ok.MenuOpen = false;
    ok.Paused = true;
    Check(!CanCycleCameraFromTouch(ok), "world/manual pause blocks camera");
    ok.Paused = false;
    ok.FocusSuspended = true;
    Check(!CanCycleCameraFromTouch(ok), "OS focus loss blocks camera");
    ok.FocusSuspended = false;
    ok.CircuitReady = false;
    Check(!CanCycleCameraFromTouch(ok),
          "missing qualified circuit blocks action");
    ok.CircuitReady = true;
    ok.PlayerReady = false;
    Check(!CanCycleCameraFromTouch(ok),
          "missing actual player driver blocks action");
    ok.PlayerReady = true;
    ok.HasRaceMode = false;
    Check(!CanCycleCameraFromTouch(ok), "missing mode blocks action");
    ok.HasRaceMode = true;
    for (auto wrong : {TouchControl::Steering, TouchControl::Throttle,
                       TouchControl::Brake, TouchControl::Recover,
                       TouchControl::Restart, TouchControl::Pause,
                       TouchControl::CareerNext, TouchControl::CareerStart})
    {
        ok.Region = wrong;
        Check(!CanCycleCameraFromTouch(ok),
              "unrelated action cannot cycle camera");
    }
    std::cout << "Touch camera command admission passed.\n";
    return EXIT_SUCCESS;
}
