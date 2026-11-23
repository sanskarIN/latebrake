#include "UI/GarageShowroomQuickView.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>

namespace
{
void Verify(bool ok,const char* reason)
{
    if(!ok){std::cerr<<"Unreal quick view: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);}
}
bool Overlaps(latebrake::DesktopMenuMouseRect a,
              latebrake::DesktopMenuMouseRect b)
{
    return a.X<b.X+b.Width && a.X+a.Width>b.X &&
        a.Y<b.Y+b.Height && a.Y+a.Height>b.Y;
}
void AllResolutions()
{
    using namespace latebrake;
    for(const auto& [w,h]:{
        std::pair{900,600},std::pair{960,640},std::pair{1280,720},
        std::pair{1920,1080},std::pair{3440,1440}})
    {
        const auto hud=CalculateHudLayout(w,h);
        const auto style=CalculateGarageStyleMouseLayout(
            w,h,hud,false,true,MenuPage::Garage);
        Verify(style.Valid,"existing style dock exists");
        const auto diagram=CalculateGarageSchematic(
            w,h,hud,style,0,FactoryGarageLook(0));
        Verify(diagram.Valid,"existing preview diagram exists");
        const auto layout=CalculateGarageQuickViewLayout(diagram);
        Verify(layout.Valid,"six chip layout fits viewport");
        for(int id=0;id<VehicleCount;id++)
        {
            const auto& b=layout.Buttons[static_cast<std::size_t>(id)];
            Verify(b.Action==GarageQuickViewAction(id) &&
                GarageQuickViewIndex(b.Action)==id,"car action maps to exact ID");
            const auto r=b.Bounds;
            Verify(r.Width>=25 && r.Height>=20 &&
                r.X>=diagram.Panel.X && r.Y>=diagram.Panel.Y &&
                r.X+r.Width<=diagram.Panel.X+diagram.Panel.Width &&
                r.Y+r.Height<=diagram.Panel.Y+diagram.Panel.Height,
                "chip fully inside schematic panel");
            Verify(HitTestGarageQuickView(layout,r.X+r.Width/2,
                r.Y+r.Height/2,true)==b.Action,
                "click center selects only corresponding viewed car");
            Verify(HitTestGarageQuickView(layout,r.X+r.Width/2,
                r.Y+r.Height/2,false)==DesktopMenuMouseAction::None,
                "closed/disabled gallery cannot be clicked");
            for(const auto& dockButton:style.Buttons)
                Verify(!Overlaps(r,dockButton.Bounds),
                    "view and styling targets do not overlap");
            for(std::size_t j=0;j<diagram.Count;j++)
                Verify(!Overlaps(r,diagram.Pieces[j].Bounds),
                    "view chips do not overlap car silhouette");
            for(int other=0;other<VehicleCount;other++)
                if(other!=id)
                    Verify(!Overlaps(r,layout.Buttons[
                        static_cast<std::size_t>(other)].Bounds),
                        "each car chip has an independent click target");
        }
        Verify(HitTestGarageQuickView(layout,
            diagram.Panel.X+1,diagram.Panel.Y+1,true)==
            DesktopMenuMouseAction::None,"padding never selects a car");
    }
}
void FailClosed()
{
    using namespace latebrake;
    GarageSchematicPlan bad;
    Verify(!CalculateGarageQuickViewLayout(bad).Valid,
        "invalid diagram has no pointer controls");
    bad.Valid=true;bad.Panel={0,0,10,112};
    Verify(!CalculateGarageQuickViewLayout(bad).Valid,
        "tiny panel rejected");
    bad.Panel={0,0,std::numeric_limits<double>::infinity(),200};
    Verify(!CalculateGarageQuickViewLayout(bad).Valid,
        "infinite panel rejected");
    Verify(GarageQuickViewAction(-1)==DesktopMenuMouseAction::None &&
        GarageQuickViewAction(VehicleCount)==DesktopMenuMouseAction::None &&
        GarageQuickViewIndex(DesktopMenuMouseAction::GaragePaint)==-1,
        "invalid vehicle and unrelated pointer action rejected");
}
}
int main()
{
    AllResolutions();
    FailClosed();
    std::cout<<"Production Unreal quick-view showroom layout passed.\n";
    return EXIT_SUCCESS;
}
