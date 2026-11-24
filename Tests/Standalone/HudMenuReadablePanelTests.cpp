
#include "UI/HudMenuReadablePanel.h"
#include "UI/HudMenuMouseControls.h"
#include "UI/HudTextBounds.h"
#include "UI/TouchControlLayout.h"
#include <cstdlib>
#include <iostream>
#include <utility>

namespace
{
void Check(bool okay,const char* why)
{
    if(!okay){std::cerr<<"Menu backplate failed: "<<why<<'\n';
               std::exit(EXIT_FAILURE);}
}
void CheckView(double w,double h,bool touch)
{
    using namespace latebrake;
    const auto l=CalculateHudLayout(w,h);
    const auto v=CalculateHudMenuVerticalPlan(w,h,l,touch);
    const auto t=CalculateHudTextBounds(w,h,l,touch);
    const auto dock=CalculateDesktopMenuMouseLayout(w,h,l,touch,true);
    const double safe=dock.Valid?
        std::min(t.MaxWidthPixels,dock.Left-t.Left-12.0*l.Scale):
        t.MaxWidthPixels;
    const auto p=CalculateHudMenuReadablePanel(w,h,l,v,safe,true);
    if(!v.Valid||!t.Valid||safe<=0.0)
    {
        Check(!p.Valid,"invalid viewport must not draw");
        return;
    }
    Check(p.Valid && p.Width>0.0 && p.Height>0.0,
          "valid menu has a contrast layer");
    Check(p.X>=0.0 && p.Y>=0.0 &&
          p.X+p.Width<=w && p.Y+p.Height<=h &&
          p.HeaderHeight>0.0 && p.HeaderHeight<=p.Height,
          "panel wholly inside viewport");
    Check(p.X<=l.Left &&
          p.X+p.Width<=l.Left+safe+0.001,
          "panel never exceeds measured text lane");
    Check(v.SaveWarningY>=p.Y &&
          v.SaveWarningY<=p.Y+p.Height,
          "save-error baseline has a readable backdrop");
    if(dock.Valid)
        Check(p.X+p.Width<=dock.Left,
              "mouse dock is outside the menu backplate");
    if(touch)
    {
        const auto pad=CalculateTouchControlLayout(w,h);
        if(pad.Valid)
            Check(p.Y+p.Height<pad.Steering.Y,
                  "touch steering strip remains untouched");
    }
    Check(!CalculateHudMenuReadablePanel(w,h,l,v,safe,false).Valid,
          "closed menu never dims active play");
}
void Screens()
{
    for(const auto size:{
        std::pair<int,int>{900,600},{1280,720},{1920,1080},
        {640,360},{540,960},{1080,2400},{2560,1440},
        {240,240},{3840,2160}
    })
    {
        CheckView(size.first,size.second,false);
        CheckView(size.first,size.second,true);
    }
}
void Stress()
{
    unsigned int seed=0x2aa9417u;
    for(int i=0;i<100000;++i)
    {
        seed=1664525u*seed+1013904223u;
        const int w=240+static_cast<int>(seed%3600u);
        seed=1664525u*seed+1013904223u;
        const int h=240+static_cast<int>(seed%1800u);
        CheckView(w,h,(seed&8u)!=0u);
    }
}
void Invalid()
{
    using namespace latebrake;
    const auto l=CalculateHudLayout(1280,720);
    const auto v=CalculateHudMenuVerticalPlan(1280,720,l,false);
    Check(!CalculateHudMenuReadablePanel(1280,720,l,v,0,true).Valid,
          "missing text width");
    Check(!CalculateHudMenuReadablePanel(1280,720,l,{},700,true).Valid,
          "missing vertical plan");
    Check(!CalculateHudMenuReadablePanel(0,720,l,v,700,true).Valid,
          "minimized canvas");
    auto wrong=l;wrong.Top=10000.0;
    Check(!CalculateHudMenuReadablePanel(1280,720,wrong,v,700,true).Valid,
          "invalid origin");
}
}
int main()
{
    Screens();Invalid();Stress();
    std::cout<<"Unreal Canvas readable menu panel passed.\n";
    return EXIT_SUCCESS;
}
