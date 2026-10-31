#include "UI/HudMenuMouseControls.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>

namespace
{
void Check(bool yes, const char* what)
{
    if (!yes)
    {
        std::cerr << "Menu mouse: " << what << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void VerifyViewport(int w, int h)
{
    using namespace latebrake;
    const auto plan = CalculateDesktopMenuMouseLayout(
        w, h, CalculateHudLayout(w, h), false, true);
    Check(plan.Valid, "wide desktop menu dock exists");
    Check(plan.Left > w * 0.58, "menu details have a separate left lane");
    constexpr DesktopMenuMouseAction actions[] = {
        DesktopMenuMouseAction::PreviousPage,
        DesktopMenuMouseAction::NextPage,
        DesktopMenuMouseAction::DetailsUp,
        DesktopMenuMouseAction::DetailsDown,
        DesktopMenuMouseAction::Activate,
        DesktopMenuMouseAction::Close
    };
    for (std::size_t i = 0; i < 6; ++i)
    {
        const auto& button = plan.Buttons[i];
        const auto b = button.Bounds;
        Check(button.Action == actions[i], "button ordering matches");
        Check(b.X >= plan.Left && b.X + b.Width <= w,
            "button stays inside horizontal viewport");
        Check(b.Y >= 0 && b.Y + b.Height < h, "button stays on screen");
        Check(HitTestDesktopMenuMouse(plan, b.X + b.Width / 2.0,
            b.Y + b.Height / 2.0) == actions[i],
            "center hit maps to exactly one action");
        Check(HitTestDesktopMenuMouse(plan, b.X + b.Width, b.Y + b.Height)
            != button.Action, "bottom-right edge exclusive");
    }
    Check(HitTestDesktopMenuMouse(plan, plan.Left - 1.0, h / 2.0)
        == DesktopMenuMouseAction::None, "details never accidentally activate");
    Check(HitTestDesktopMenuMouse(plan, -1.0, -1.0)
        == DesktopMenuMouseAction::None, "negative coordinates rejected");
}
} // namespace

int main()
{
    using namespace latebrake;
    for (const auto [width, height] :
        {std::pair{1280, 720}, std::pair{1920, 1080},
         std::pair{2560, 1440}, std::pair{3440, 1440}})
    {
        VerifyViewport(width, height);
    }
    const auto good = CalculateHudLayout(1280, 720);
    Check(!CalculateDesktopMenuMouseLayout(
        1280, 720, good, true, true).Valid, "never use mouse dock on Android");
    Check(!CalculateDesktopMenuMouseLayout(
        1280, 720, good, false, false).Valid, "closed menu never clickable");
    for (int w : {0, 320, 640, 800, 1099})
    {
        Check(!CalculateDesktopMenuMouseLayout(
            w, 720, CalculateHudLayout(w, 720), false, true).Valid,
            "compact viewport disabled");
    }
    const auto shortLayout = CalculateDesktopMenuMouseLayout(
        1280, 640, CalculateHudLayout(1280, 640), false, true);
    Check(!shortLayout.Valid, "short viewport disabled");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Check(!CalculateDesktopMenuMouseLayout(
        nan, 720, good, false, true).Valid, "invalid width");
    Check(!CalculateDesktopMenuMouseLayout(
        1280, nan, good, false, true).Valid, "invalid height");
    auto plan = CalculateDesktopMenuMouseLayout(
        1280, 720, good, false, true);
    Check(HitTestDesktopMenuMouse(plan, nan, 200) ==
        DesktopMenuMouseAction::None, "NaN click cannot activate");
    Check(HitTestDesktopMenuMouse(plan,
        std::numeric_limits<double>::infinity(), 200) ==
        DesktopMenuMouseAction::None, "infinite click cannot activate");
    plan.Valid = false;
    Check(HitTestDesktopMenuMouse(plan, 1200, 120) ==
        DesktopMenuMouseAction::None, "invalid layout is inert");

    // Broad resize coverage: every admitted dock stays bounded.
    for (int w = 1100; w <= 3600; w += 17)
    {
        for (int h = 680; h <= 1600; h += 19)
        {
            const auto l = CalculateDesktopMenuMouseLayout(
                w, h, CalculateHudLayout(w, h), false, true);
            if (!l.Valid) continue;
            for (const auto& button : l.Buttons)
            {
                Check(button.Bounds.X >= 0 && button.Bounds.Y >= 0 &&
                    button.Bounds.X + button.Bounds.Width <= w &&
                    button.Bounds.Y + button.Bounds.Height < h,
                    "resize sweep stays within screen");
            }
        }
    }
    std::cout << "Desktop menu mouse controls passed.\n";
    return EXIT_SUCCESS;
}
