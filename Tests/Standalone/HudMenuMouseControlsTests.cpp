#include "UI/HudMenuMouseControls.h"

#include <cstdlib>
#include <cstring>
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
void TestGarageStyleIndicators()
{
    using namespace latebrake;
    constexpr DesktopMenuMouseAction styleActions[] = {
        DesktopMenuMouseAction::GaragePaint,
        DesktopMenuMouseAction::GarageStripe,
        DesktopMenuMouseAction::GarageWheels,
        DesktopMenuMouseAction::GarageReset,
        DesktopMenuMouseAction::GarageBrowsePrevious,
        DesktopMenuMouseAction::GarageBrowseNext
    };
    for (int vehicle = 0; vehicle < VehicleCount; ++vehicle)
    {
        const auto factory = FactoryGarageLook(vehicle);
        for (int paint = 0; paint < GaragePaintOptions; ++paint)
        for (int stripe = 0; stripe < GarageStripeOptions; ++stripe)
        for (int wheels = 0; wheels < GarageWheelOptions; ++wheels)
        {
            const GarageLook look{
                static_cast<std::uint8_t>(paint),
                static_cast<std::uint8_t>(stripe),
                static_cast<std::uint8_t>(wheels)};
            const bool isFactory = SameGarageLook(look, factory);
            for (auto action : styleActions)
            {
                const auto info =
                    DescribeGarageStyleButton(action, vehicle, look);
                Check(info.Valid && info.Caption && info.Caption[0] != '\0',
                    "every valid showroom choice has a readable button");
                Check(info.Factory == isFactory,
                    "factory status follows viewed car, not equipped car");
                if (action == DesktopMenuMouseAction::GaragePaint)
                    Check(info.Ordinal == paint+1 &&
                          info.Total == GaragePaintOptions,
                          "paint button indicates actual current shade");
                else if (action == DesktopMenuMouseAction::GarageStripe)
                    Check(info.Ordinal == stripe+1 &&
                          info.Total == GarageStripeOptions,
                          "stripe button indicates exact pattern index");
                else if (action == DesktopMenuMouseAction::GarageWheels)
                    Check(info.Ordinal == wheels+1 &&
                          info.Total == GarageWheelOptions,
                          "rim button indicates exact visible trim index");
                else if (action == DesktopMenuMouseAction::GarageReset)
                    Check(info.Total == 0 &&
                          std::strcmp(info.Caption,
                              isFactory ? "FACTORY OK" : "RESET LOOK") == 0,
                          "factory button explains whether a reset is needed");
                else
                    Check(info.Total == 0 && info.Ordinal == 0,
                          "browse action never borrows styling option count");
            }
        }
    }
    const auto factory = FactoryGarageLook(0);
    for (int badId : {-1, VehicleCount, 99999})
        Check(!DescribeGarageStyleButton(
            DesktopMenuMouseAction::GaragePaint, badId, factory).Valid,
            "future/invalid car ID cannot supply a fake style caption");
    for (GarageLook bad : {GarageLook{255u,0u,0u},
                          GarageLook{0u,255u,0u},
                          GarageLook{0u,0u,255u}})
        Check(!DescribeGarageStyleButton(
            DesktopMenuMouseAction::GarageReset, 0, bad).Valid,
            "corrupt cosmetic record has no reset-button presentation");
    Check(!DescribeGarageStyleButton(
        DesktopMenuMouseAction::None, 0, factory).Valid,
        "unrecognized control gets no fabricated garage label");
}

void TestGarageCosmeticDock()
{
    using namespace latebrake;
    for (const auto& [width, height] :
        {std::pair{900, 600}, std::pair{960, 640},
         std::pair{1280, 720}, std::pair{1920, 1080},
         std::pair{2560, 1440}})
    {
        const auto geometry = CalculateHudLayout(width, height);
        const auto garage = CalculateGarageStyleMouseLayout(
            width, height, geometry, false, true, MenuPage::Garage);
        Check(garage.Valid,
            "desktop garage provides four style and two browse buttons");
        const auto dock = CalculateDesktopMenuMouseLayout(
            width, height, geometry, false, true);
        const auto& last = dock.Buttons[5].Bounds;
        const DesktopMenuMouseAction expected[] = {
            DesktopMenuMouseAction::GaragePaint,
            DesktopMenuMouseAction::GarageStripe,
            DesktopMenuMouseAction::GarageWheels,
            DesktopMenuMouseAction::GarageReset,
            DesktopMenuMouseAction::GarageBrowsePrevious,
            DesktopMenuMouseAction::GarageBrowseNext
        };
        for (int i = 0; i < 6; ++i)
        {
            const auto button = garage.Buttons[static_cast<std::size_t>(i)];
            Check(button.Action == expected[i],
                  "fixed style action order");
            const auto b = button.Bounds;
            Check(b.Width >= 112.0 && b.Height >= 44.0 &&
                  b.X + b.Width <= width && b.Y + b.Height < height &&
                  b.Y >= last.Y + last.Height,
                  "style buttons fit below older menu controls");
            Check(HitTestGarageStyleMouse(garage,
                b.X+b.Width*0.5, b.Y+b.Height*0.5) == expected[i],
                "exact drawn center selects intended style");
            Check(HitTestDesktopMenuMouse(dock,
                b.X+b.Width*0.5, b.Y+b.Height*0.5) ==
                  DesktopMenuMouseAction::None,
                "old menu buttons cannot capture style clicks");
        }
        Check(!CalculateGarageStyleMouseLayout(
            width, height, geometry, false, true, MenuPage::Results).Valid,
            "Results menu does not allow cosmetic inputs");
        Check(!CalculateGarageStyleMouseLayout(
            width, height, geometry, false, false, MenuPage::Garage).Valid,
            "closed garage is not clickable");
        Check(!CalculateGarageStyleMouseLayout(
            width, height, geometry, true, true, MenuPage::Garage).Valid,
            "desktop pointer controls do not overlay touch HUD");
        Check(HitTestGarageStyleMouse(garage, -1.0, -1.0) ==
              DesktopMenuMouseAction::None,
              "invalid garage pointer is inert");
        const auto blockedButton = garage.Buttons[0].Bounds;
        Check(HitTestGarageStyleMouse(garage,
            blockedButton.X+blockedButton.Width*0.5,
            blockedButton.Y+blockedButton.Height*0.5,
            false) == DesktopMenuMouseAction::None,
            "uneditable active career cannot select a style with mouse");
        const auto browseRect = garage.Buttons[4].Bounds;
        const double browseX = browseRect.X + browseRect.Width*0.5;
        const double browseY = browseRect.Y + browseRect.Height*0.5;
        Check(HitTestGarageStyleMouse(garage, browseX, browseY,
                  false, true) ==
                  DesktopMenuMouseAction::GarageBrowsePrevious,
              "locked career still permits read-only browsing");
        Check(HitTestGarageStyleMouse(garage, browseX, browseY,
                  true, false) == DesktopMenuMouseAction::None,
              "closed or unauthorized gallery cannot browse");
        Check(HitTestGarageStyleMouse(garage,
                  blockedButton.X+blockedButton.Width*0.5,
                  blockedButton.Y+blockedButton.Height*0.5,
                  false, true) == DesktopMenuMouseAction::None,
              "disabled styling remains blocked while browsing is allowed");
        auto damaged = garage;
        damaged.Buttons[0].Bounds.Width =
            std::numeric_limits<double>::infinity();
        const auto b = garage.Buttons[0].Bounds;
        Check(HitTestGarageStyleMouse(damaged,
            b.X+b.Width*0.5, b.Y+b.Height*0.5) ==
              DesktopMenuMouseAction::None,
            "nonfinite garage rectangle never captures clicks");
    }
}
} // namespace

int main()
{
    TestGarageCosmeticDock();
    TestGarageStyleIndicators();
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
    const double max = std::numeric_limits<double>::max();
    broken = plan;
    broken.Buttons[0].Bounds.X = max;
    broken.Buttons[0].Bounds.Width = max;
    Check(HitTestDesktopMenuMouse(broken, max,
        plan.Buttons[0].Bounds.Y + 1.0) ==
        DesktopMenuMouseAction::None,
        "finite origin and width with overflowing right edge rejected");
    broken = plan;
    broken.Buttons[0].Bounds.Y = max;
    broken.Buttons[0].Bounds.Height = max;
    Check(HitTestDesktopMenuMouse(broken,
        plan.Buttons[0].Bounds.X + 1.0, max) ==
        DesktopMenuMouseAction::None,
        "finite origin and height with overflowing bottom edge rejected");
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
    // Wheel events must stay inside the actual left details lane, away
    // from menu actions and the persistent SAVE FAILED/footer baseline.
    for (const auto& [width, height] :
        {std::pair{240, 240}, std::pair{640, 360},
         std::pair{900, 600}, std::pair{1280, 720},
         std::pair{1920, 1080}, std::pair{3440, 1440}})
    {
        const auto l = CalculateHudLayout(width, height);
        const auto zone = CalculateDesktopMenuWheelZone(
            width, height, l, false, true);
        Check(zone.Width > 0.0 && zone.Height > 0.0,
              "desktop details wheel lane exists for supported viewports");
        const double x = zone.X + zone.Width * 0.5;
        const double y = zone.Y + zone.Height * 0.5;
        Check(AdmitDesktopMenuWheel(zone, x, y, -1, {true, true}) &&
              AdmitDesktopMenuWheel(zone, x, y, 1, {true, true}),
              "both directions scroll when real neighboring pages exist");
        Check(!AdmitDesktopMenuWheel(zone, x, y, 0, {true, true}) &&
              !AdmitDesktopMenuWheel(zone, x, y, -2, {true, true}) &&
              !AdmitDesktopMenuWheel(zone, x, y, 2, {true, true}),
              "unknown wheel directions never scroll");
        Check(!AdmitDesktopMenuWheel(zone, x, y, -1, {false, true}) &&
              !AdmitDesktopMenuWheel(zone, x, y, 1, {true, false}) &&
              !AdmitDesktopMenuWheel(zone, x, y, 1, {}),
              "first, last and empty detail pages never scroll beyond bounds");
        Check(!AdmitDesktopMenuWheel(zone, zone.X - 1.0, y,
                  1, {true, true}) &&
              !AdmitDesktopMenuWheel(zone, x, zone.Y - 1.0,
                  1, {true, true}),
              "wheel outside the left details area is inert");
        Check(!AdmitDesktopMenuWheel(zone,
              std::numeric_limits<double>::quiet_NaN(),
              y, 1, {true, true}),
              "invalid mouse wheel coordinates are inert");
        Check(!CalculateDesktopMenuWheelZone(
              width, height, l, true, true).Contains(x, y) &&
              !CalculateDesktopMenuWheelZone(
              width, height, l, false, false).Contains(x, y),
              "touch UI and closed menus never admit desktop wheel");
        const auto dock = CalculateDesktopMenuMouseLayout(
            width, height, l, false, true);
        if (dock.Valid)
        {
            for (const auto& button : dock.Buttons)
            {
                const auto b = button.Bounds;
                Check(!AdmitDesktopMenuWheel(zone,
                    b.X + b.Width * 0.5, b.Y + b.Height * 0.5,
                    1, {true, true}),
                    "wheel above menu action buttons cannot scroll details");
            }
        }
    }
    Check(!CalculateDesktopMenuWheelZone(0, 720, good, false, true)
              .Contains(100, 100),
          "invalid viewport never has an active wheel zone");
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
