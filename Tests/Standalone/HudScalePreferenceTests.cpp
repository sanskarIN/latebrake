#include "UI/HudScalePreference.h"
#include "UI/HudInstrumentCluster.h"
#include "UI/HudRaceStatusPanel.h"
#include "UI/HudMenuMouseControls.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr<<"HudScalePreference: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b){return std::abs(a-b)<1e-7;}
bool Same(HudLayout a,HudLayout b)
{
    return a.Scale==b.Scale && a.Left==b.Left && a.Top==b.Top &&
        a.LineHeight==b.LineHeight && a.Compact==b.Compact;
}
void PreferenceCycle()
{
    auto p=HudScalePreference::Automatic;
    for(int i=0;i<10;++i)
    {
        Require(ValidHudScalePreference(p) && std::string_view(HudScalePreferenceName(p))=="AUTO",
            "default mode");
        p=NextHudScalePreference(p);
        Require(p==HudScalePreference::Smaller && std::string_view(HudScalePreferenceName(p))=="SMALLER",
            "smaller mode");
        p=NextHudScalePreference(p);
        Require(p==HudScalePreference::Larger && std::string_view(HudScalePreferenceName(p))=="LARGER",
            "larger mode");
        p=NextHudScalePreference(p);
    }
    const auto invalid=static_cast<HudScalePreference>(255);
    Require(!ValidHudScalePreference(invalid) &&
        NextHudScalePreference(invalid)==HudScalePreference::Automatic,"unknown mode recovers to auto");
}
void RequestedSizeAndLimit()
{
    const auto smaller=ResolveHudScalePreference(1280,720,HudScalePreference::Smaller,false,false);
    const auto larger=ResolveHudScalePreference(1280,720,HudScalePreference::Larger,false,false);
    Require(smaller.Applied && !smaller.Limited && Near(smaller.Layout.Scale,0.85),
        "desktop smaller factor");
    Require(larger.Applied && !larger.Limited && Near(larger.Layout.Scale,1.15),
        "desktop larger factor");
    const auto max=ResolveHudScalePreference(1920,1080,HudScalePreference::Larger,false,false);
    Require(!max.Applied && max.Limited && Near(max.Layout.Scale,1.2),
        "large screen existing geometry upper bound");
    const auto min=ResolveHudScalePreference(640,480,HudScalePreference::Smaller,false,false);
    Require(!min.Applied && min.Limited && Near(min.Layout.Scale,0.6),
        "minimum scale preserves legibility");
    for(const auto& dims:std::array<std::array<double,2>,5>{{
        {0,0},{239,480},{1280,0},{std::numeric_limits<double>::infinity(),720},
        {1280,std::numeric_limits<double>::quiet_NaN()}}})
    {
        const auto result=ResolveHudScalePreference(dims[0],dims[1],HudScalePreference::Larger,false,false);
        Require(!result.Applied && result.Limited &&
            Same(result.Layout,CalculateHudLayout(dims[0],dims[1])),"invalid window fallback");
    }
}
void PreserveMenuGeometry()
{
    for(const auto& dims:std::array<std::array<double,2>,5>{{
        {640,480},{900,600},{1280,720},{1920,1080},{3440,1440}}})
    {
        const auto base=CalculateHudLayout(dims[0],dims[1]);
        const auto original=CalculateDesktopMenuMouseLayout(dims[0],dims[1],base,false,true);
        for(auto mode:{HudScalePreference::Automatic,HudScalePreference::Smaller,HudScalePreference::Larger})
        {
            const auto result=ResolveHudScalePreference(dims[0],dims[1],mode,true,false);
            Require(!result.Applied && !result.Limited && Same(result.Layout,base),
                "menu size immutable across preference");
            const auto dock=CalculateDesktopMenuMouseLayout(dims[0],dims[1],result.Layout,false,true);
            Require(dock.Valid==original.Valid && dock.Left==original.Left,"same dock geometry");
            for(std::size_t i=0;i<dock.Buttons.size();++i)
            {
                const auto& a=dock.Buttons[i];const auto& b=original.Buttons[i];
                Require(a.Action==b.Action && a.Bounds.X==b.Bounds.X && a.Bounds.Y==b.Bounds.Y &&
                    a.Bounds.Width==b.Bounds.Width && a.Bounds.Height==b.Bounds.Height,
                    "purchase and navigation pointer targets unaffected");
            }
        }
    }
}
void CheckLayout(double w,double h,bool touch,HudScalePreference mode)
{
    const auto base=CalculateHudLayout(w,h);
    const auto read=ResolveHudScalePreference(w,h,mode,false,touch);
    const auto& layout=read.Layout;
    const auto oldVertical=CalculateHudVerticalPlan(w,h,base,touch);
    const auto oldHorizontal=CalculateHudTextBounds(w,h,base,touch);
    Require(layout.Scale>=HudPreferenceMinimumScale && layout.Scale<=HudPreferenceMaximumScale &&
        layout.Compact==base.Compact,"same compact class, bounded scale");
    if(mode==HudScalePreference::Automatic || !oldVertical.Valid || !oldHorizontal.Valid)
    {
        Require(Same(layout,base) && !read.Applied,"baseline retained");
        return;
    }
    if(mode==HudScalePreference::Smaller)Require(layout.Scale<=base.Scale+1e-7,"never grow smaller");
    else Require(layout.Scale>=base.Scale-1e-7,"never shrink larger");
    const auto vertical=CalculateHudVerticalPlan(w,h,layout,touch);
    const auto horizontal=CalculateHudTextBounds(w,h,layout,touch);
    Require(vertical.Valid && horizontal.Valid &&
        vertical.MaxTelemetryLines>=std::min(oldVertical.MaxTelemetryLines,HudPreferenceMinimumTelemetryRows),
        "essential rows and safe width retained");
    Require(vertical.SaveWarningY<vertical.RaceStatusY && vertical.RaceStatusY<vertical.FooterY,
        "status and footer distinct");
    if(touch)
    {
        const auto pads=CalculateTouchControlLayout(w,h);
        Require(pads.Valid && horizontal.Right<pads.CareerNext.X &&
            horizontal.Right<pads.MenuBrowse.X && horizontal.Right<pads.Pause.X &&
            vertical.RaceStatusY+layout.LineHeight<=pads.Throttle.Y &&
            vertical.RaceStatusY+layout.LineHeight<=pads.Steering.Y,
            "independent touch targets remain unobstructed");
    }
    else
    {
        const auto instruments=CalculateHudInstrumentCluster(w,h,layout,false,false,false,
            120,0.6,0.2,0.1);
        const auto radar=CalculateHudRaceRadarLayout(w,h,layout,horizontal,instruments,false,false,false);
        double safe=horizontal.MaxWidthPixels;
        if(instruments.Visible)safe=std::min(safe,instruments.Panel.X-horizontal.Left-16*instruments.Scale);
        if(radar.Valid)safe=std::min(safe,radar.Panel.X-horizontal.Left-12*layout.Scale);
        const auto status=CalculateHudRaceStatusLayout(w,h,layout,vertical,horizontal,safe,
            false,false,false,true);
        if(status.Valid)
        {
            Require(status.RemainingRows>=HudRaceStatusMinimumRemainingRows,
                "race overview reserves essential telemetry after scaling");
            if(radar.Valid)Require(!HudRadarOverlaps(status.Panel,radar.Panel),"radar separated");
        }
    }
}
void RandomViewports()
{
    std::mt19937 rng(28416);
    for(int i=0;i<12000;++i)
    {
        const double w=240+rng()%3900,h=240+rng()%2100;
        for(bool touch:{false,true})
            for(auto mode:{HudScalePreference::Automatic,HudScalePreference::Smaller,HudScalePreference::Larger})
                CheckLayout(w,h,touch,mode);
    }
}
} // namespace
int main()
{
    PreferenceCycle();RequestedSizeAndLimit();PreserveMenuGeometry();RandomViewports();
    std::cout<<"HUD sizing preference, menu isolation and viewport safety passed\n";
}
