#include "UI/GarageShowroomSchematic.h"
#include "Vehicles/VehiclePaintRecipe.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>

namespace
{
void Verify(bool yes,const char* reason)
{
    if (!yes)
    {
        std::cerr << "Garage schematic FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Intersects(latebrake::DesktopMenuMouseRect a,
                latebrake::DesktopMenuMouseRect b)
{
    return a.X < b.X+b.Width && a.X+a.Width > b.X &&
           a.Y < b.Y+b.Height && a.Y+a.Height > b.Y;
}

int CountInk(const latebrake::GarageSchematicPlan& p,
             latebrake::GarageSchematicInk ink)
{
    int result = 0;
    for (std::size_t i = 0; i < p.Count; ++i)
        result += p.Pieces[i].Ink == ink;
    return result;
}

void TestViewport(int width,int height)
{
    using namespace latebrake;
    const auto hud = CalculateHudLayout(width,height);
    const auto dock = CalculateGarageStyleMouseLayout(
        width,height,hud,false,true,MenuPage::Garage);
    Verify(dock.Valid,"desktop dock exists");
    for (int vehicle = 0;vehicle < VehicleCount;++vehicle)
    {
        for (int paint = 0;paint < GaragePaintOptions;++paint)
        {
            for (int stripe = 0;stripe < GarageStripeOptions;++stripe)
            {
                for (int rims = 0;rims < GarageWheelOptions;++rims)
                {
                    const GarageLook look{
                        static_cast<std::uint8_t>(paint),
                        static_cast<std::uint8_t>(stripe),
                        static_cast<std::uint8_t>(rims)};
                    const auto p = CalculateGarageSchematic(
                        width,height,hud,dock,vehicle,look);
                    Verify(p.Valid && p.Count <= p.Pieces.size(),
                        "every six-car appearance renders in desktop panel");
                    Verify(p.Panel.Width >= 224.0 &&
                           p.Panel.Height >= 112.0 &&
                           p.Panel.Y + p.Panel.Height <= height,
                        "panel inside viewport");
                    Verify(CountInk(p,GarageSchematicInk::Tire) == 4 &&
                           CountInk(p,GarageSchematicInk::Rim) == 4,
                        "every schematic has four cosmetic wheels and rims");
                    Verify(CountInk(p,GarageSchematicInk::Stripe) == stripe,
                        "clean/center/twin stripe count is genuine");
                    for (const auto& button : dock.Buttons)
                        Verify(!Intersects(p.Panel,button.Bounds),
                            "preview panel cannot cover mouse controls");
                    for (std::size_t i = 0;i < p.Count;++i)
                    {
                        const auto r = p.Pieces[i].Bounds;
                        Verify(r.Width > 0.0 && r.Height > 0.0 &&
                               r.X >= p.Panel.X && r.Y >= p.Panel.Y &&
                               r.X+r.Width <= p.Panel.X+p.Panel.Width &&
                               r.Y+r.Height <= p.Panel.Y+p.Panel.Height,
                            "every glyph inside dedicated showroom viewport");
                    }
                    const auto colors = PaintForGarageLook(vehicle,look);
                    Verify(colors.Name && colors.Name[0] != '\0',
                        "diagram uses the same valid factory/user paint");
                }
            }
        }
    }
}

void TestResponsivenessAndFailureModes()
{
    using namespace latebrake;
    for (const auto& [w,h] :
        {std::pair{900,600},std::pair{960,640},
         std::pair{1024,768},std::pair{1280,720},
         std::pair{1920,1080},std::pair{2560,1440},
         std::pair{3440,1440}})
        TestViewport(w,h);
    const auto hud = CalculateHudLayout(1280,720);
    const auto good = CalculateGarageStyleMouseLayout(
        1280,720,hud,false,true,MenuPage::Garage);
    Verify(good.Valid,"baseline desktop dock");
    for (int invalid : {-200,-1,VehicleCount,99999})
        Verify(!CalculateGarageSchematic(
            1280,720,hud,good,invalid,FactoryGarageLook(0)).Valid,
            "invalid car ID cannot render a mismatched owned style");
    Verify(!CalculateGarageSchematic(
        1280,720,hud,good,0,{255u,0u,0u}).Valid,
        "out-of-bounds paint rejected");
    Verify(!CalculateGarageSchematic(
        1280,720,hud,good,0,{1u,99u,0u}).Valid,
        "out-of-bounds stripe rejected");
    Verify(!CalculateGarageSchematic(
        1280,720,hud,good,0,{1u,1u,3u}).Valid,
        "out-of-bounds rim rejected");
    const auto bad = std::numeric_limits<double>::quiet_NaN();
    Verify(!CalculateGarageSchematic(
        bad,720,hud,good,0,{}).Valid,
        "nonfinite viewport dimensions rejected");
    Verify(!CalculateGarageSchematic(
        1280,720,{bad,0,0,0,false},good,0,{}).Valid,
        "nonfinite UI scale rejected");
    Verify(!CalculateGarageSchematic(
        1280,720,hud,{},0,{}).Valid,
        "closed menu cannot paint a phantom showroom");
    const auto touch = CalculateGarageStyleMouseLayout(
        1280,720,hud,true,true,MenuPage::Garage);
    Verify(!CalculateGarageSchematic(
        1280,720,hud,touch,0,{}).Valid,
        "mobile/touch UI has no overlapping desktop preview");
    for (int width = 900; width < 2500;width+=61)
        for (int height = 600; height < 1300;height+=73)
        {
            const auto layout = CalculateHudLayout(width,height);
            const auto dock = CalculateGarageStyleMouseLayout(
                width,height,layout,false,true,MenuPage::Garage);
            if (!dock.Valid) continue;
            const auto p = CalculateGarageSchematic(
                width,height,layout,dock,2,{7u,2u,2u});
            // If the window is too short to fit an accessible preview,
            // silently omit the diagram rather than invade control bounds.
            if (!p.Valid) continue;
            Verify(p.Panel.Y+p.Panel.Height <= height &&
                   !Intersects(p.Panel,dock.Buttons[5].Bounds),
                "resize sweep remains within the dedicated free lane");
        }
    // Appearance edits should visibly alter ink count/size without
    // introducing component reallocations, saving, or gameplay mutations.
    const auto clean = CalculateGarageSchematic(
        1280,720,hud,good,2,{2u,0u,0u});
    const auto sport = CalculateGarageSchematic(
        1280,720,hud,good,2,{2u,2u,2u});
    Verify(clean.Valid && sport.Valid &&
           sport.Count == clean.Count + 2 &&
           sport.Pieces[2].Bounds.Width > clean.Pieces[2].Bounds.Width,
        "double stripe and track rims produce genuine visible geometry");
    const auto arrow = CalculateGarageSchematic(
        1280,720,hud,good,0,FactoryGarageLook(0));
    const auto nova = CalculateGarageSchematic(
        1280,720,hud,good,5,FactoryGarageLook(5));
    Verify(arrow.Valid && nova.Valid && arrow.Count != nova.Count,
        "garage profiles have discernible silhouette geometry");
}

} // namespace
int main()
{
    TestResponsivenessAndFailureModes();
    std::cout << "Garage showroom schematic validated across source cars, styles and viewports.\n";
    return EXIT_SUCCESS;
}
