#include "UI/GarageShowroomLane.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "Garage lane: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
using namespace latebrake;

void StyleAdmission(const GarageStyleMouseLayout& style)
{
    Check(style.Valid, "expected valid style geometry");
    for (std::size_t i = 0; i < style.Buttons.size(); ++i)
    {
        const auto& button = style.Buttons[i];
        const auto& r = button.Bounds;
        const double x = r.X + r.Width / 2.0;
        const double y = r.Y + r.Height / 2.0;
        const bool browse = i >= 4;
        for (const bool styleAllowed : {false, true})
            for (const bool browseAllowed : {false, true})
            {
                const auto expected = (browse ? browseAllowed : styleAllowed)
                    ? button.Action : DesktopMenuMouseAction::None;
                Check(HitTestGarageStyleMouse(style, x, y, styleAllowed, browseAllowed)
                    == expected, "style and browsing have independent permissions");
            }
        Check(HitTestGarageStyleMouse(style, r.X + r.Width, y)
            == DesktopMenuMouseAction::None, "right edge is exclusive");
        Check(HitTestGarageStyleMouse(style, x, r.Y + r.Height)
            == DesktopMenuMouseAction::None, "bottom edge is exclusive");

        auto bad = style;
        bad.Buttons[(i + 1) % style.Buttons.size()].Action =
            DesktopMenuMouseAction::Activate;
        Check(HitTestGarageStyleMouse(bad, x, y) == DesktopMenuMouseAction::None,
            "wrong action anywhere rejects whole plan");
        bad = style;
        bad.Buttons[(i + 1) % style.Buttons.size()].Bounds = r;
        Check(HitTestGarageStyleMouse(bad, x, y) == DesktopMenuMouseAction::None,
            "overlap rejects whole plan");
        bad = style;
        bad.Buttons[(i + 1) % style.Buttons.size()].Bounds.Width =
            std::numeric_limits<double>::infinity();
        Check(HitTestGarageStyleMouse(bad, x, y) == DesktopMenuMouseAction::None,
            "nonfinite target rejects whole plan");
    }
}

void DesktopGeometry()
{
    int tested = 0;
    for (const auto& size : {std::array<int, 2>{900,600}, {1280,720},
             {1366,768}, {1920,1080}, {2560,1440}, {3840,2160}, {2560,1080}})
    {
        const double w = size[0], h = size[1];
        const auto hud = CalculateHudLayout(w,h);
        const auto dock = CalculateDesktopMenuMouseLayout(w,h,hud,false,true);
        const auto style = CalculateGarageStyleMouseLayout(w,h,hud,false,true,MenuPage::Garage);
        const auto lane = CalculateGarageShowroomLane(w,h,hud,false,true,MenuPage::Garage);
        Check(dock.Valid && lane.Valid, "desktop showroom has a clear lane");
        StyleAdmission(style);
        const auto panel = CalculateHudMenuReadablePanel(w,h,hud,
            CalculateHudMenuVerticalPlan(w,h,hud,false),lane.TextWidth,true);
        Check(panel.Valid && lane.Aperture.X > panel.X + panel.Width &&
            lane.Aperture.X + lane.Aperture.Width < dock.Left,
            "clear aperture does not obscure details or controls");
        Check(!CalculateGarageShowroomLane(w,h,hud,true,true,MenuPage::Garage).Valid &&
            !CalculateGarageShowroomLane(w,h,hud,false,true,MenuPage::Results).Valid &&
            !CalculateGarageShowroomLane(w,h,hud,false,false,MenuPage::Garage).Valid,
            "only open desktop Garage receives layout");
        const auto buttons = CalculateGarageOrbitButtons(lane,w,h,hud);
        Check(buttons.Valid, "all four angles fit below aperture");
        for (int i = 0; i < GarageOrbitCount; ++i)
        {
            const auto& b = buttons.Buttons[static_cast<std::size_t>(i)];
            const auto& r = b.Bounds;
            const double x = r.X + r.Width / 2.0, y = r.Y + r.Height / 2.0;
            Check(r.Width >= 44.0 && r.Height >= 44.0 &&
                r.Y > lane.Aperture.Y + lane.Aperture.Height,
                "angle targets are readable and separate");
            Check(HitTestGarageOrbitButtons(buttons,x,y,true) == GarageOrbitAction(i) &&
                HitTestGarageOrbitButtons(buttons,x,y,false) == DesktopMenuMouseAction::None,
                "angle action and admission agree");
            auto bad = buttons;
            bad.Buttons[(i + 1) % GarageOrbitCount].Bounds = r;
            Check(HitTestGarageOrbitButtons(bad,x,y,true) == DesktopMenuMouseAction::None,
                "ambiguous angle targets fail closed");
            ++tested;
        }
    }
    Check(tested == 28, "seven viewport layouts and four angle targets");
}

void InvalidGeometry()
{
    const auto hud = CalculateHudLayout(1280,720);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Check(!CalculateGarageShowroomLane(nan,720,hud,false,true,MenuPage::Garage).Valid,
        "nonfinite viewport rejected");
    Check(!CalculateGarageShowroomLane(640,480,CalculateHudLayout(640,480),
        false,true,MenuPage::Garage).Valid, "compact fallback has no desktop lane");
    Check(!CalculateGarageStyleMouseLayout(1280,720,hud,false,true,MenuPage::Results).Valid,
        "cosmetic controls never belong to Results");
    Check(!CalculateGarageStyleMouseLayout(1280,720,hud,true,true,MenuPage::Garage).Valid,
        "cosmetic desktop controls never consume touch");
    Check(HitTestGarageStyleMouse({},0,0) == DesktopMenuMouseAction::None,
        "invalid style plan cannot be activated");
}
}
int main()
{
    DesktopGeometry();
    InvalidGeometry();
    std::cout << "Garage showroom lane and style admission passed.\n";
    return EXIT_SUCCESS;
}
