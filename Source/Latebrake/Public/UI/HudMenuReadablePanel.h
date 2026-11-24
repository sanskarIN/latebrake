
#pragma once
#include "UI/HudLayout.h"
#include "UI/HudMenuVerticalPlan.h"
#include <algorithm>
#include <cmath>

namespace latebrake
{
// Visual-only safe Canvas backplate; never changes pointer, touch or input.
struct HudMenuReadablePanel final
{
    bool Valid=false;
    double X=0.0,Y=0.0,Width=0.0,Height=0.0,HeaderHeight=0.0;
};

inline HudMenuReadablePanel CalculateHudMenuReadablePanel(
    double w,double h,HudLayout layout,HudMenuVerticalPlan vertical,
    double safeTextWidth,bool menuOpen) noexcept
{
    if(!menuOpen || !vertical.Valid ||
       !std::isfinite(w) || !std::isfinite(h) ||
       w<240.0 || h<240.0 ||
       !std::isfinite(layout.Left) || layout.Left<0.0 ||
       !std::isfinite(layout.Top) || layout.Top<0.0 ||
       !std::isfinite(layout.Scale) || layout.Scale<0.25 ||
       layout.Scale>4.0 ||
       !std::isfinite(safeTextWidth) || safeTextWidth<=0.0 ||
       !std::isfinite(vertical.SaveWarningY))
        return {};

    const double s=layout.Scale;
    const double left=std::max(0.0,layout.Left-12.0*s);
    const double top=std::max(0.0,layout.Top-10.0*s);
    // The right edge uses the EXACT protected font-measured text lane.
    // Do not darken the real desktop menu dock or phone action column.
    const double right=std::min(w-2.0*s,layout.Left+safeTextWidth);
    const double bottom=std::min(h-2.0*s,vertical.SaveWarningY+22.0*s);
    const double width=right-left,height=bottom-top;
    const double header=std::min(height,46.0*s);
    if(!std::isfinite(left)||!std::isfinite(top)||
       !std::isfinite(width)||!std::isfinite(height)||
       width<140.0*s||height<70.0*s||
       right>w||bottom>h||header<14.0*s||header>height)
        return {};
    return {true,left,top,width,height,header};
}
} // namespace latebrake
