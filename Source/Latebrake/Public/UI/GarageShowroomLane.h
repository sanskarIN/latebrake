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
} // namespace latebrake
