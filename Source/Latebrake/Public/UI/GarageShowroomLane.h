#pragma once

#include "UI/HudMenuMouseControls.h"
#include "UI/HudMenuReadablePanel.h"
#include <algorithm>
#include <cmath>

namespace latebrake
{
// One geometry contract for the desktop Canvas text lane, wheel admission,
// and the actual 3D view. It does not relocate any button or change authority.
struct GarageShowroomLane final
{
    bool Valid=false;
    double TextWidth=0.0;
    DesktopMenuMouseRect Aperture{};
    double CenterX=0.0;
    double CenterY=0.0;
    double HalfWidth=1.0;
    double HalfHeight=1.0;
};

inline GarageShowroomLane CalculateGarageShowroomLane(
    double width,double height,HudLayout hud,bool touchUI,
    bool menuOpen,MenuPage page) noexcept
{
    if(page!=MenuPage::Garage||!menuOpen||touchUI||hud.Compact||
       !std::isfinite(width)||!std::isfinite(height)||
       width<900.0||height<600.0)return {};
    const auto dock=CalculateDesktopMenuMouseLayout(width,height,hud,false,true);
    const auto style=CalculateGarageStyleMouseLayout(width,height,hud,false,true,page);
    const auto text=CalculateHudTextBounds(width,height,hud,false);
    const auto vertical=CalculateHudMenuVerticalPlan(width,height,hud,false);
    if(!dock.Valid||!style.Valid||!text.Valid||!vertical.Valid)return {};
    const double textRight=std::min(width*0.40,dock.Left-200.0);
    const double textWidth=textRight-text.Left;
    const auto panel=CalculateHudMenuReadablePanel(
        width,height,hud,vertical,textWidth,true);
    if(!panel.Valid||textWidth<240.0)return {};
    const double gap=std::max(12.0,18.0*hud.Scale);
    const double left=panel.X+panel.Width+gap;
    const double right=dock.Left-gap;
    const double top=height*0.15;
    const double bottom=height*0.85;
    if(right-left<160.0||bottom-top<180.0)return {};
    const DesktopMenuMouseRect aperture{left,top,right-left,bottom-top};
    // Pixel coordinates are top-left; camera NDC has positive Y upward.
    const double cx=(left+right)/width-1.0;
    const double cy=1.0-(top+bottom)/height;
    const double hx=aperture.Width/width;
    const double hy=aperture.Height/height;
    if(!std::isfinite(cx)||!std::isfinite(cy)||
       !std::isfinite(hx)||!std::isfinite(hy)||
       std::abs(cx)+hx>1.0||std::abs(cy)+hy>1.0||
       hx<0.10||hy<0.10)return {};
    return {true,textWidth,aperture,cx,cy,hx,hy};
}

inline constexpr int GarageOrbitCount=4;
inline constexpr DesktopMenuMouseAction GarageOrbitAction(int index) noexcept
{
    return index>=0&&index<GarageOrbitCount
        ? static_cast<DesktopMenuMouseAction>(
            static_cast<int>(DesktopMenuMouseAction::GarageOrbit0)+index)
        : DesktopMenuMouseAction::None;
}
inline constexpr int GarageOrbitIndex(DesktopMenuMouseAction action) noexcept
{
    constexpr int first=static_cast<int>(DesktopMenuMouseAction::GarageOrbit0);
    const int value=static_cast<int>(action);
    return value>=first&&value<first+GarageOrbitCount?value-first:-1;
}
inline constexpr const char* GarageOrbitCaption(int index) noexcept
{
    constexpr const char* captions[]={"FRONT","SIDE","REAR","OTHER"};
    return index>=0&&index<GarageOrbitCount?captions[index]:"";
}
static_assert(GarageOrbitIndex(GarageOrbitAction(3))==3);
static_assert(GarageQuickViewIndex(GarageOrbitAction(0))==-1);

struct GarageOrbitButtons final
{
    bool Valid=false;
    std::array<DesktopMenuMouseButton,GarageOrbitCount> Buttons{};
};

// Keep all four pointer targets below the projected geometry, within the
// same horizontal lane. A small or malformed lane never creates an
// invisible click target. Existing right dock and left details are intact.
inline GarageOrbitButtons CalculateGarageOrbitButtons(
    const GarageShowroomLane& lane,double width,double height,
    HudLayout hud) noexcept
{
    GarageOrbitButtons result{};
    const auto& a=lane.Aperture;
    if(!lane.Valid||!std::isfinite(width)||!std::isfinite(height)||
       !std::isfinite(hud.Scale)||hud.Scale<0.6||hud.Scale>1.2||
       !std::isfinite(a.X)||!std::isfinite(a.Y)||
       !std::isfinite(a.Width)||!std::isfinite(a.Height)||
       a.X<0.0||a.Y<0.0||a.Width<=0.0||a.Height<=0.0||
       a.X+a.Width>width||a.Y+a.Height>height)return {};
    const double gap=std::max(6.0,8.0*hud.Scale);
    const double bh=std::max(44.0,44.0*hud.Scale);
    const double bw=std::min(100.0*hud.Scale,
        (a.Width-(GarageOrbitCount-1)*gap)/GarageOrbitCount);
    const double y=a.Y+a.Height+gap;
    if(!std::isfinite(bw)||bw<44.0||y+bh>height-gap)return {};
    const double total=GarageOrbitCount*bw+(GarageOrbitCount-1)*gap;
    const double x=a.X+(a.Width-total)*0.5;
    if(!std::isfinite(x)||x<a.X||x+total>a.X+a.Width+1e-7)return {};
    for(int i=0;i<GarageOrbitCount;++i)
    {
        result.Buttons[static_cast<std::size_t>(i)]={
            GarageOrbitAction(i),{x+i*(bw+gap),y,bw,bh}};
    }
    result.Valid=true;
    return result;
}

inline DesktopMenuMouseAction HitTestGarageOrbitButtons(
    const GarageOrbitButtons& buttons,double x,double y,bool enabled) noexcept
{
    if(!buttons.Valid||!enabled||!std::isfinite(x)||!std::isfinite(y))
        return DesktopMenuMouseAction::None;
    DesktopMenuMouseAction found=DesktopMenuMouseAction::None;
    for(int i=0;i<GarageOrbitCount;++i)
    {
        const auto& b=buttons.Buttons[static_cast<std::size_t>(i)];
        const auto& r=b.Bounds;
        if(b.Action!=GarageOrbitAction(i)||!std::isfinite(r.X)||
           !std::isfinite(r.Y)||!std::isfinite(r.Width)||
           !std::isfinite(r.Height)||!std::isfinite(r.X+r.Width)||
           !std::isfinite(r.Y+r.Height)||r.X<0.0||r.Y<0.0||
           r.Width<44.0||r.Height<44.0)
            return DesktopMenuMouseAction::None;
        if(!r.Contains(x,y))continue;
        if(found!=DesktopMenuMouseAction::None)return DesktopMenuMouseAction::None;
        found=b.Action;
    }
    return found;
}
} // namespace latebrake
