#include "UI/HudButtonCaptionFit.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <array>

using namespace latebrake;
namespace
{
void Check(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "HudButtonCaptionFit: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void FullAndShort()
{
    const DesktopMenuMouseRect button{20, 40, 120, 44};
    auto fit = FitHudButtonCaption(button, 1, 6, 3, 0, true,
        [](bool shortText, double scale)
        {return HudCaptionExtent{(shortText?30.0:90.0)*scale, 20*scale};});
    Check(fit.Valid && !fit.ShortCaption && fit.Scale==1,
        "prefer full caption at preferred scale");
    Check(fit.X==35 && fit.Y==52, "center within both axes");
    fit = FitHudButtonCaption(button, 1, 6, 3, 25, true,
        [](bool shortText, double scale)
        {return HudCaptionExtent{(shortText?30.0:90.0)*scale, 20*scale};});
    Check(fit.Valid && fit.ShortCaption && fit.Scale==1,
        "paint swatch must reserve space before fitting text");
    Check(fit.X+30<=109, "short label cannot cover accessory");
    fit = FitHudButtonCaption(button, 1, 6, 3, 0, false,
        [](bool, double scale){return HudCaptionExtent{120*scale,20*scale};});
    Check(fit.Valid && fit.Scale==0.84, "bounded reduction when full label needs room");
    fit = FitHudButtonCaption(button, 1, 6, 3, 0, true,
        [](bool, double){return HudCaptionExtent{900,900};});
    Check(!fit.Valid, "unfittable text is never painted outside");
}

void InvalidGeometry()
{
    constexpr DesktopMenuMouseRect bounds{0,0,120,44};
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    int calls=0;
    const auto measure=[&](bool,double){++calls;return HudCaptionExtent{20,15};};
    for(const double bad : {nan, inf, -1.0, 0.0})
        Check(!FitHudButtonCaption(bounds,bad,4,4,0,false,measure).Valid,
            "invalid scale rejected");
    for(const double bad : {nan,inf,-1.0,120.0})
        Check(!FitHudButtonCaption(bounds,1,4,4,bad,false,measure).Valid,
            "invalid accessory rejected");
    for(const auto rect : {DesktopMenuMouseRect{nan,0,120,44},
        DesktopMenuMouseRect{0,0,0,44}, DesktopMenuMouseRect{-1,0,120,44},
        DesktopMenuMouseRect{1e308,0,1e308,44}})
        Check(!FitHudButtonCaption(rect,1,4,4,0,false,measure).Valid,
            "invalid rectangle rejected");
    Check(calls==0,"never measure invalid geometry");
    for(const double bad : {nan,inf,-1.0,0.0})
        Check(!FitHudButtonCaption(bounds,1,4,4,0,true,
            [bad](bool,double){return HudCaptionExtent{bad,15};}).Valid,
            "invalid font measurement rejected");
}

void LayoutMatrix()
{
    const std::array widths{25.0,44.0,100.0,112.0,150.0,260.0};
    const std::array heights{20.0,44.0,52.8,80.0};
    for(double w:widths) for(double h:heights)
        for(double scale:{0.5,0.7,0.9,1.2})
            for(double accessory:{0.0,14.0,25.0})
                for(double glyphWidth:{12.0,55.0,130.0,400.0})
                {
                    const DesktopMenuMouseRect button{17,23,w,h};
                    auto measure=[&](bool brief,double s)
                    {
                        return HudCaptionExtent{(brief?glyphWidth/2:glyphWidth)*s,
                            18*s};
                    };
                    const auto fit=FitHudButtonCaption(button,scale,6,3,
                        accessory,true,measure);
                    if(!fit.Valid) continue;
                    const auto extent=measure(fit.ShortCaption,fit.Scale);
                    Check(fit.X>=23 && fit.Y>=26 &&
                        fit.X+extent.Width<=17+w-6-accessory &&
                        fit.Y+extent.Height<=23+h-3,
                        "every measured placement stays in protected lane");
                    Check(fit.Scale<=scale && fit.Scale>=scale*0.76,
                        "caption does not grow or shrink below bounded fallback");
                }
}
}
int main()
{
    FullAndShort();
    InvalidGeometry();
    LayoutMatrix();
    std::cout << "HUD button caption layout tests passed.\n";
    return EXIT_SUCCESS;
}
