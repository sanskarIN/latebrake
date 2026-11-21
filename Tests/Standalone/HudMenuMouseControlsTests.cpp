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
    Check(plan.Left >= 0.0, "responsive menu never crosses the text lane");
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
        Check(b.Width >= 112.0 && b.Height >= 44.0,
              "all desktop actions retain accessible pointer targets");
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
    for (const auto& [width, height] :
        {std::pair{900, 600}, std::pair{960, 640},
         std::pair{1024, 768}, std::pair{1100, 680},
         std::pair{1280, 720}, std::pair{1920, 1080},
         std::pair{2560, 1440}, std::pair{3440, 1440}})
    {
        VerifyViewport(width, height);
    }
    const auto good = CalculateHudLayout(1280, 720);
    Check(!CalculateDesktopMenuMouseLayout(
        1280, 720, good, true, true).Valid, "never use mouse dock on Android");
    Check(!CalculateDesktopMenuMouseLayout(
        1280, 720, good, false, false).Valid, "closed menu never clickable");
    for (int w : {0, 320, 640, 800, 899})
    {
        Check(!CalculateDesktopMenuMouseLayout(
            w, 720, CalculateHudLayout(w, 720), false, true).Valid,
            "compact viewport disabled");
    }
    const auto shortLayout = CalculateDesktopMenuMouseLayout(
        1280, 599, CalculateHudLayout(1280, 599), false, true);
    Check(!shortLayout.Valid, "viewport below minimum height disabled");
    Check(CalculateDesktopMenuMouseLayout(
        1280, 640, CalculateHudLayout(1280, 640), false, true).Valid,
        "short but usable landscape desktop keeps keyboard and mouse dock");
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
    // Malformed rectangles must not accept clicks even if the enclosing
    // layout is accidentally marked valid by an external caller.
    auto broken = plan;
    broken.Buttons[0].Bounds.Width = 0.0;
    Check(HitTestDesktopMenuMouse(broken,
        plan.Buttons[0].Bounds.X + 1.0,
        plan.Buttons[0].Bounds.Y + 1.0) ==
        DesktopMenuMouseAction::None, "zero-width click target rejected");
    broken = plan;
    broken.Buttons[0].Bounds.Height =
        std::numeric_limits<double>::infinity();
    Check(HitTestDesktopMenuMouse(broken,
        plan.Buttons[0].Bounds.X + 1.0,
        plan.Buttons[0].Bounds.Y + 1.0) ==
        DesktopMenuMouseAction::None, "nonfinite click target rejected");
    broken = plan;
    broken.Buttons[0].Bounds.X = nan;
    Check(HitTestDesktopMenuMouse(broken, 100, 100) ==
        DesktopMenuMouseAction::None, "nonfinite button origin rejected");
    // Real pager lifecycle governs BOTH whether detail buttons look
    // enabled and whether a pointer click can request page changes.
    MenuDetailPager pager;
    const auto disabled = AvailableDesktopMenuDetails(MenuPage::Garage,
        pager);
    Check(!disabled.DetailsUp && !disabled.DetailsDown,
        "unsynchronized menu cannot expose stale detail arrows");
    const auto detailUp = plan.Buttons[2].Bounds;
    const auto detailDown = plan.Buttons[3].Bounds;
    const auto click = [&](DesktopMenuMouseRect button,
                           DesktopMenuMouseAvailability availability)
    {
        return HitTestDesktopMenuMouse(plan, button.X + button.Width * 0.5,
            button.Y + button.Height * 0.5, availability);
    };
    Check(click(detailUp, disabled) == DesktopMenuMouseAction::None &&
          click(detailDown, disabled) == DesktopMenuMouseAction::None,
          "disabled detail controls are not clickable");
    Check(click(plan.Buttons[0].Bounds, disabled) ==
              DesktopMenuMouseAction::PreviousPage &&
          click(plan.Buttons[4].Bounds, disabled) ==
              DesktopMenuMouseAction::Activate &&
          click(plan.Buttons[5].Bounds, disabled) ==
              DesktopMenuMouseAction::Close,
          "page, selection and close actions remain usable without details");
    pager.Synchronize(MenuPage::Garage, 11, 3);
    auto allowed = AvailableDesktopMenuDetails(MenuPage::Garage, pager);
    Check(!allowed.DetailsUp && allowed.DetailsDown &&
          click(detailUp, allowed) == DesktopMenuMouseAction::None &&
          click(detailDown, allowed) ==
              DesktopMenuMouseAction::DetailsDown,
          "first page exposes only next-detail action");
    pager.Next();
    allowed = AvailableDesktopMenuDetails(MenuPage::Garage, pager);
    Check(allowed.DetailsUp && allowed.DetailsDown &&
          click(detailUp, allowed) == DesktopMenuMouseAction::DetailsUp &&
          click(detailDown, allowed) ==
              DesktopMenuMouseAction::DetailsDown,
          "middle detail page enables both directions");
    pager.Next();
    pager.Next();
    allowed = AvailableDesktopMenuDetails(MenuPage::Garage, pager);
    Check(allowed.DetailsUp && !allowed.DetailsDown &&
          click(detailUp, allowed) == DesktopMenuMouseAction::DetailsUp &&
          click(detailDown, allowed) == DesktopMenuMouseAction::None,
          "last detail page disables next-detail action");
    allowed = AvailableDesktopMenuDetails(MenuPage::Results, pager);
    Check(!allowed.DetailsUp && !allowed.DetailsDown,
          "switching menu categories invalidates old pager authority");
    pager.Synchronize(MenuPage::Results, 1, 4);
    allowed = AvailableDesktopMenuDetails(MenuPage::Results, pager);
    Check(!allowed.DetailsUp && !allowed.DetailsDown,
          "single-page results never claim scrollable details");
    pager.Synchronize(MenuPage::Results, 11, 0);
    allowed = AvailableDesktopMenuDetails(MenuPage::Results, pager);
    Check(!allowed.DetailsUp && !allowed.DetailsDown,
          "zero visible rows cannot create clickable ghost arrows");
    pager.Synchronize(MenuPage::Closed, 11, 3);
    allowed = AvailableDesktopMenuDetails(MenuPage::Closed, pager);
    Check(!allowed.DetailsUp && !allowed.DetailsDown &&
          click(detailUp, allowed) == DesktopMenuMouseAction::None,
          "closing clears paging controls");
    Check(!DesktopMenuMouseActionEnabled(
              DesktopMenuMouseAction::None, {true, true}),
          "None is never a clickable action");
    plan.Valid = false;
    Check(HitTestDesktopMenuMouse(plan, 1200, 120) ==
        DesktopMenuMouseAction::None, "invalid layout is inert");

    // Broad resize coverage: every admitted dock stays bounded.
    for (int w = 900; w <= 3600; w += 17)
    {
        for (int h = 600; h <= 1600; h += 19)
        {
            const auto l = CalculateDesktopMenuMouseLayout(
                w, h, CalculateHudLayout(w, h), false, true);
            if (!l.Valid) continue;
            for (const auto& button : l.Buttons)
            {
                Check(button.Bounds.X >= 0 && button.Bounds.Y >= 0 &&
                    button.Bounds.X + button.Bounds.Width <= w &&
                    button.Bounds.Y + button.Bounds.Height < h &&
                    button.Bounds.Width >= 112.0 &&
                    button.Bounds.Height >= 44.0,
                    "resize sweep stays on screen with usable click targets");
            }
        }
    }
    std::cout << "Desktop menu mouse controls passed.\n";
    return EXIT_SUCCESS;
}
