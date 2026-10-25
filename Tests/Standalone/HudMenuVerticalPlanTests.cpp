#include "UI/HudMenuVerticalPlan.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "HUD menu vertical FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
void CheckViewport(double width, double height, bool mobile)
{
    using namespace latebrake;
    const HudLayout layout = CalculateHudLayout(width, height);
    const auto plan =
        CalculateHudMenuVerticalPlan(width, height, layout, mobile);
    Check(plan.Valid && plan.MaxDetailsRows >= 1 &&
          plan.MaxDetailsRows <= 90,
          "supported viewport has at least one menu detail row");
    const double step = 28.0 * layout.Scale;
    const double lastBaseline = layout.Top +
        static_cast<double>(plan.MaxDetailsRows - 1) * step;
    Check(lastBaseline + 1.25 * step <=
          plan.OverflowNoticeY + 1e-8,
          "ordinary menu details clear reserved overflow notice");
    Check(plan.OverflowNoticeY + 1.25 * step <=
          plan.SaveWarningY + 1e-8,
          "overflow and SAVE FAILED notices have distinct baselines");
    if (mobile)
    {
        const auto touch = CalculateTouchControlLayout(width, height);
        Check(touch.Valid &&
              plan.SaveWarningY + step <= touch.Steering.Y + 1e-8,
              "mobile menu text clears actual physical steering zone");
    }
    else
    {
        Check(plan.SaveWarningY + step <= height + 1e-8,
              "desktop menu SAVE banner stays in viewport");
    }
}
}

int main()
{
    using namespace latebrake;
    for (auto [width, height] : {
        std::pair{240.0, 240.0},
        std::pair{360.0, 640.0},
        std::pair{640.0, 360.0},
        std::pair{800.0, 480.0},
        std::pair{1280.0, 720.0},
        std::pair{1920.0, 1080.0}
    })
    {
        CheckViewport(width, height, false);
        CheckViewport(width, height, true);
    }
    const auto landscape = CalculateHudMenuVerticalPlan(
        640.0, 360.0, CalculateHudLayout(640.0, 360.0), true);
    const auto desktop = CalculateHudMenuVerticalPlan(
        640.0, 360.0, CalculateHudLayout(640.0, 360.0), false);
    Check(desktop.MaxDetailsRows > landscape.MaxDetailsRows,
          "real steering pad leaves fewer rows on Android");
    for (double bad : {
        0.0, 239.0, -1.0,
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()
    })
    {
        Check(!CalculateHudMenuVerticalPlan(
            bad, 720.0, CalculateHudLayout(bad, 720.0), false).Valid,
            "invalid width fails closed");
        Check(!CalculateHudMenuVerticalPlan(
            1280.0, bad, CalculateHudLayout(1280.0, bad), true).Valid,
            "invalid height fails closed");
    }
    auto layout = CalculateHudLayout(1280.0, 720.0);
    layout.Scale = 0.0;
    Check(!CalculateHudMenuVerticalPlan(1280.0, 720.0, layout, false).Valid,
          "invalid font scaling fails closed");
    layout = CalculateHudLayout(1280.0, 720.0);
    layout.Top = std::numeric_limits<double>::quiet_NaN();
    Check(!CalculateHudMenuVerticalPlan(1280.0, 720.0, layout, false).Valid,
          "invalid menu top fails closed");
    std::cout << "HUD menu vertical tests passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
