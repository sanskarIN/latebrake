#pragma once

#include "UI/HudMenuMouseControls.h"
#include "Vehicles/VehiclePaintRecipe.h"
#include "Vehicles/OriginalVehicleRimRecipe.h"
#include <array>
#include <cmath>

namespace latebrake
{
// Bounded, allocation-free schematic preview. No render capture or
// claim of native pixels: coordinates derive from the real look recipe.
struct GarageGlyphPoint final { double X=0.0,Y=0.0; };
enum class GarageGlyphInk { Body, Accent, Stripe, Tire, Alloy, Hub };
struct GarageGlyphLine final
{
    GarageGlyphPoint A{},B{};
    GarageGlyphInk Ink=GarageGlyphInk::Body;
    double Thickness=1.0;
};
struct GarageGlyphRect final
{
    DesktopMenuMouseRect Bounds{};
    GarageGlyphInk Ink=GarageGlyphInk::Body;
};
struct GarageAppearanceGlyph final
{
    bool Valid=false;
    DesktopMenuMouseRect Bounds{};
    std::array<GarageGlyphRect,8> Rects{};
    std::array<GarageGlyphLine,128> Lines{};
    std::size_t RectCount=0,LineCount=0;
    int SpokeCount=0;
    int StripeCount=0;
    GarageLook Look{};
    int VehicleId=-1;
};

inline bool ValidGarageGlyphBounds(DesktopMenuMouseRect b) noexcept
{
    return std::isfinite(b.X)&&std::isfinite(b.Y)&&
        std::isfinite(b.Width)&&std::isfinite(b.Height)&&
        std::isfinite(b.X+b.Width)&&std::isfinite(b.Y+b.Height)&&
        b.X>=0.0&&b.Y>=0.0&&b.Width>=20.0&&b.Height>=20.0&&
        b.Width<=4096.0&&b.Height<=4096.0&&
        b.X+b.Width>b.X&&b.Y+b.Height>b.Y;
}
inline VehiclePaintRgb GarageGlyphColor(GarageGlyphInk ink,int vehicle,GarageLook look) noexcept
{
    const auto paint=PaintForGarageLook(vehicle,look);
    switch(ink)
    {
    case GarageGlyphInk::Body: return paint.Body;
    case GarageGlyphInk::Accent: return paint.Accent;
    case GarageGlyphInk::Stripe: return StripeTintForGarageLook(look);
    case GarageGlyphInk::Tire: return {0.030,0.033,0.038};
    case GarageGlyphInk::Alloy: return RimTintForGarageLook(look);
    case GarageGlyphInk::Hub: return {0.30,0.32,0.36};
    }
    return {};
}

inline GarageAppearanceGlyph CalculateGarageAppearanceGlyph(
    DesktopMenuMouseRect bounds,int vehicle,GarageLook look,GarageLookLayer layer) noexcept
{
    if(!ValidGarageGlyphBounds(bounds)||!GarageState::IsVehicle(vehicle)||
        !ValidGarageLook(look)||GarageLookChoiceCount(layer)<=0)return {};
    GarageAppearanceGlyph out;
    out.Bounds=bounds;out.Look=look;out.VehicleId=vehicle;
    bool ok=true;
    const auto pointInside=[&](GarageGlyphPoint p,double margin)
    {
        return std::isfinite(p.X)&&std::isfinite(p.Y)&&
            p.X-margin>=bounds.X&&p.X+margin<=bounds.X+bounds.Width&&
            p.Y-margin>=bounds.Y&&p.Y+margin<=bounds.Y+bounds.Height;
    };
    const auto line=[&](GarageGlyphPoint a,GarageGlyphPoint b,
        GarageGlyphInk ink,double thickness)
    {
        if(out.LineCount>=out.Lines.size()||!std::isfinite(thickness)||
            thickness<=0.0||!pointInside(a,thickness*0.5)||
            !pointInside(b,thickness*0.5))
        {ok=false;return;}
        out.Lines[out.LineCount++]={a,b,ink,thickness};
    };
    const auto rect=[&](double x,double y,double w,double h,GarageGlyphInk ink)
    {
        if(out.RectCount>=out.Rects.size()||!std::isfinite(w)||!std::isfinite(h)||
            w<=0.0||h<=0.0||!pointInside({x,y},0.0)||
            !pointInside({x+w,y+h},0.0))
        {ok=false;return;}
        out.Rects[out.RectCount++]={{x,y,w,h},ink};
    };
    const double cx=bounds.X+bounds.Width*0.5;
    const double cy=bounds.Y+bounds.Height*0.5;
    if(layer==GarageLookLayer::Wheels)
    {
        const auto recipe=vehiclemesh::OriginalRimRecipeForStyle(look.Wheels);
        if(recipe.Spokes<=0)return {};
        const double radius=0.5*std::min(bounds.Width,bounds.Height)-2.0;
        const double ring=radius*0.79;
        const auto polar=[&](double r,double a)
        {return GarageGlyphPoint{cx+r*std::cos(a),cy+r*std::sin(a)};};
        constexpr int segments=24;
        constexpr double tau=2.0*vehiclemesh::OriginalVehiclePi;
        for(int i=0;i<segments;++i)
        {
            const double a=tau*i/segments,b=tau*(i+1)/segments;
            line(polar(radius,a),polar(radius,b),GarageGlyphInk::Tire,3.0);
            line(polar(ring,a),polar(ring,b),GarageGlyphInk::Alloy,1.5);
            line(polar(ring*0.19,a),polar(ring*0.19,b),GarageGlyphInk::Hub,1.0);
        }
        // Exactly the authored mesh's spoke endpoints and angular widths.
        // A four-edge outline represents each alloy face at UI scale.
        for(int i=0;i<recipe.Spokes;++i)
        {
            const double a=tau*i/recipe.Spokes;
            const double d=recipe.HalfAngle;
            const auto p0=polar(ring*recipe.InnerFraction,a-d);
            const auto p1=polar(ring*recipe.OuterFraction,a-d*0.25);
            const auto p2=polar(ring*recipe.OuterFraction,a+d*0.25);
            const auto p3=polar(ring*recipe.InnerFraction,a+d);
            line(p0,p1,GarageGlyphInk::Alloy,1.0);
            line(p1,p2,GarageGlyphInk::Alloy,1.0);
            line(p2,p3,GarageGlyphInk::Alloy,1.0);
            line(p3,p0,GarageGlyphInk::Alloy,1.0);
        }
        out.SpokeCount=recipe.Spokes;
    }
    else
    {
        // Bounded roof swatch, not a full-car top-view approximation.
        const double w=bounds.Width*0.84,h=bounds.Height*0.68;
        const double x=cx-w*0.5,y=cy-h*0.5;
        rect(x,y,w,h,GarageGlyphInk::Body);
        rect(x,y,w,h*0.10,GarageGlyphInk::Accent);
        rect(x,y+h*0.90,w,h*0.10,GarageGlyphInk::Accent);
        if(layer==GarageLookLayer::Stripe || layer==GarageLookLayer::Preset)
        {
            if(look.Stripe==1u)
                rect(x+w*0.08,cy-h*0.085,w*0.84,h*0.17,GarageGlyphInk::Stripe);
            else if(look.Stripe==2u)
            {
                rect(x+w*0.08,cy-h*0.23,w*0.84,h*0.11,GarageGlyphInk::Stripe);
                rect(x+w*0.08,cy+h*0.12,w*0.84,h*0.11,GarageGlyphInk::Stripe);
            }
            out.StripeCount=look.Stripe;
        }
    }
    if(!ok||(out.RectCount==0&&out.LineCount==0))return {};
    out.Valid=true;return out;
}
} // namespace latebrake
