#include "UI/GarageAppearanceGlyph.h"
#include "UI/GarageAppearancePalette.h"
#include "Vehicles/OriginalVehicleSurfaceMesh.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Check(bool pass,const char* label)
{
    if(!pass)
    {
        std::cerr<<label<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Equal(VehiclePaintRgb a,VehiclePaintRgb b)
{
    return a.R==b.R&&a.G==b.G&&a.B==b.B;
}
void VerifyBounds(const GarageAppearanceGlyph& g)
{
    Check(g.Valid&&g.RectCount<=g.Rects.size()&&g.LineCount<=g.Lines.size(),
        "valid bounded drawing plan");
    const auto& b=g.Bounds;
    for(std::size_t i=0;i<g.RectCount;++i)
    {
        const auto& r=g.Rects[i].Bounds;
        Check(r.X>=b.X&&r.Y>=b.Y&&r.Width>0&&r.Height>0&&
            r.X+r.Width<=b.X+b.Width&&r.Y+r.Height<=b.Y+b.Height,
            "every rect remains inside icon");
    }
    for(std::size_t i=0;i<g.LineCount;++i)
    {
        const auto& l=g.Lines[i];
        const double m=l.Thickness*0.5;
        for(const auto p:{l.A,l.B})
            Check(std::isfinite(p.X)&&std::isfinite(p.Y)&&
                p.X-m>=b.X&&p.X+m<=b.X+b.Width&&
                p.Y-m>=b.Y&&p.Y+m<=b.Y+b.Height,
                "stroke extent remains inside icon");
    }
}
void AllValidLooks()
{
    for(int id=0;id<VehicleCount;++id)
        for(int p=0;p<GaragePaintOptions;++p)
            for(int s=0;s<GarageStripeOptions;++s)
                for(int w=0;w<GarageWheelOptions;++w)
                {
                    const GarageLook look{static_cast<std::uint8_t>(p),
                        static_cast<std::uint8_t>(s),static_cast<std::uint8_t>(w)};
                    for(const auto size:{20.0,28.8,34.0,48.0,57.6,96.0})
                    {
                        const DesktopMenuMouseRect b{17.5,30.25,size,size};
                        const auto stripe=CalculateGarageAppearanceGlyph(
                            b,id,look,GarageLookLayer::Stripe);
                        VerifyBounds(stripe);
                        Check(stripe.StripeCount==s&&stripe.RectCount==static_cast<std::size_t>(3+s),
                            "clean center twin have exactly zero one two stripes");
                        Check(stripe.LineCount==0,"stripe icon uses simple rects");
                        std::size_t stripes=0;
                        for(std::size_t i=0;i<stripe.RectCount;++i)
                            if(stripe.Rects[i].Ink==GarageGlyphInk::Stripe)++stripes;
                        Check(stripes==static_cast<std::size_t>(s),"stripe rect count matches choice");
                        Check(Equal(GarageGlyphColor(GarageGlyphInk::Body,id,look),
                            PaintForGarageLook(id,look).Body),"body color matches real palette");
                        Check(Equal(GarageGlyphColor(GarageGlyphInk::Stripe,id,look),
                            StripeTintForGarageLook(look)),"stripe color matches real palette");
                        const auto wheel=CalculateGarageAppearanceGlyph(
                            b,id,look,GarageLookLayer::Wheels);
                        VerifyBounds(wheel);
                        Check(wheel.SpokeCount==vehiclemesh::OriginalVehicleSpokesForStyle(w),
                            "glyph and original geometry spoke count agree");
                        Check(wheel.LineCount==static_cast<std::size_t>(72+4*wheel.SpokeCount),
                            "each spoke emits a complete four-edge outline");
                        Check(Equal(GarageGlyphColor(GarageGlyphInk::Alloy,id,look),
                            RimTintForGarageLook(look)),"alloy tint matches native material");
                        Check(wheel.RectCount==0,"wheel outline has no rectangle over spokes");
                        const auto paint=CalculateGarageAppearanceGlyph(
                            b,id,look,GarageLookLayer::Paint);
                        VerifyBounds(paint);
                        Check(paint.RectCount==3&&paint.StripeCount==0&&paint.LineCount==0,
                            "paint swatch is independent of stripe geometry");
                    }
                }
}
void AuthoredGeometryAgreement()
{
    using namespace vehiclemesh;
    for(int style=0;style<OriginalVehicleRimStyleCount;++style)
    {
        const auto recipe=OriginalRimRecipeForStyle(style);
        Check(recipe.Spokes==std::array{7,5,12}[static_cast<std::size_t>(style)],
            "original factory sport track identities");
        const auto glyph=CalculateGarageAppearanceGlyph(
            {0,0,64,64},0,{0,0,static_cast<std::uint8_t>(style)},GarageLookLayer::Wheels);
        VerifyBounds(glyph);
        const double ring=30.0*0.79;
        for(int i=0;i<recipe.Spokes;++i)
        {
            const auto j=static_cast<std::size_t>(72+i*4);
            const auto& a=glyph.Lines[j];
            const auto& b=glyph.Lines[j+1];
            const auto& c=glyph.Lines[j+2];
            const auto& d=glyph.Lines[j+3];
            const auto distance=[](GarageGlyphPoint p)
            {return std::hypot(p.X-32.0,p.Y-32.0);};
            Check(std::abs(distance(a.A)-ring*recipe.InnerFraction)<1e-8&&
                std::abs(distance(a.B)-ring*recipe.OuterFraction)<1e-8,
                "spoke inner and outer radii match native parameters");
            Check(a.B.X==b.A.X&&a.B.Y==b.A.Y&&b.B.X==c.A.X&&b.B.Y==c.A.Y&&
                c.B.X==d.A.X&&c.B.Y==d.A.Y&&d.B.X==a.A.X&&d.B.Y==a.A.Y,
                "spoke face is a closed outline");
        }
        for(int id=0;id<VehicleCount;++id)
        {
            const auto built=OriginalVehicleSurfaceComposer{}.Build(id,style);
            Check(built.Valid&&built.Mesh.RimStyle==style&&
                built.Mesh.RimSpokeQuads==glyph.SpokeCount*4,
                "all four native wheels share schematic rim identity");
        }
    }
}
void InvalidInput()
{
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    for(auto b:{DesktopMenuMouseRect{0,0,19,30},{0,0,30,0},{nan,0,30,30},
        {0,0,inf,30},{-1,0,30,30},{1e308,0,32,32},{0,0,4097,30}})
        Check(!CalculateGarageAppearanceGlyph(b,0,{},GarageLookLayer::Wheels).Valid,
            "malformed bounds fail closed");
    Check(!CalculateGarageAppearanceGlyph({0,0,40,40},-1,{},GarageLookLayer::Wheels).Valid,
        "invalid vehicle rejected");
    Check(!CalculateGarageAppearanceGlyph({0,0,40,40},0,{0,0,255},GarageLookLayer::Wheels).Valid,
        "corrupt appearance rejected");
    Check(!CalculateGarageAppearanceGlyph({0,0,40,40},0,{},static_cast<GarageLookLayer>(255)).Valid,
        "unknown layer rejected");
    Check(OriginalRimRecipeForStyle(-1).Spokes==0&&OriginalRimRecipeForStyle(3).Spokes==0,
        "unknown rim style has no geometry");
}
}
int main()
{
    AllValidLooks();
    AuthoredGeometryAgreement();
    InvalidInput();
    std::cout<<"Garage appearance glyph agreement passed.\n";
    return EXIT_SUCCESS;
}
