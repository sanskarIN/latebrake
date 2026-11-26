#pragma once

#include "Vehicles/OriginalVehicleSurfaceMesh.h"
#include <array>

namespace latebrake::vehiclemesh
{
// Three prepared, visual-only bands. Clean shows none, center shows
// band zero, twin shows bands one and two. Styling changes visibility
// and tint, not topology, wheel contact or the authoritative chassis.
inline constexpr std::size_t OriginalRoofStripeBandCount=3;
inline constexpr std::size_t OriginalRoofStripeMaxQuads=66;
inline constexpr double OriginalRoofStripeClearanceCm=0.65;
inline constexpr double OriginalRoofStripeHalfWidthCm=2.4;
inline constexpr std::array<double,OriginalRoofStripeBandCount> OriginalRoofStripeOffsets{
    0.0,-21.0,21.0};

constexpr bool OriginalRoofStripeVisible(int choice,std::size_t band) noexcept
{
    return band<OriginalRoofStripeBandCount&&
        ((choice==1&&band==0)||(choice==2&&(band==1||band==2)));
}

struct OriginalRoofStripeQuad final
{
    std::array<OriginalVehiclePoint,4> Points{};
    OriginalVehiclePoint Normal{};
    OriginalVehiclePoint Tangent{};
};
struct OriginalRoofStripeBand final
{
    std::array<OriginalRoofStripeQuad,OriginalRoofStripeMaxQuads> Quads{};
    std::size_t Count=0;
};
struct OriginalRoofStripePlan final
{
    bool Valid=false;
    int VehicleId=-1;
    std::array<OriginalRoofStripeBand,OriginalRoofStripeBandCount> Bands{};
};

inline OriginalRoofStripePlan BuildOriginalRoofStripePlan(
    int id,OriginalVehicleMeshSettings settings={}) noexcept
{
    if(!GarageState::IsVehicle(id)||!ValidOriginalVehicleSettings(settings))return {};
    const OriginalVehicleSurfaceComposer composer(settings);
    // Stay over the central body top rather than the separate front/rear
    // aero details. Each segment is split on the actual authoring grid.
    const double start=-70.0,end=70.0;
    const double half=settings.LengthCm*0.5;
    const double step=settings.LengthCm/settings.LongitudinalSegments;
    OriginalRoofStripePlan result;
    result.VehicleId=id;
    for(int i=0;i<settings.LongitudinalSegments;++i)
    {
        const double gridA=-half+step*i;
        const double gridB=-half+step*(i+1);
        const double xa=std::max(start,gridA),xb=std::min(end,gridB);
        if(xb-xa<=1e-8)continue;
        const auto a=composer.SampleRoof(id,xa);
        const auto b=composer.SampleRoof(id,xb);
        if(!a.Valid||!b.Valid)return {};
        const double slope=(b.Z-a.Z)/(xb-xa);
        const double length=std::hypot(1.0,slope);
        if(!std::isfinite(length)||length<=0.0)return {};
        const OriginalVehiclePoint normal{-slope/length,0.0,1.0/length};
        const OriginalVehiclePoint tangent{1.0/length,0.0,slope/length};
        for(std::size_t band=0;band<result.Bands.size();++band)
        {
            const double y=OriginalRoofStripeOffsets[band];
            const double lo=y-OriginalRoofStripeHalfWidthCm;
            const double hi=y+OriginalRoofStripeHalfWidthCm;
            // Require the complete stripe to lie on the roof surface.
            if(std::max(std::abs(lo),std::abs(hi))>=std::min(a.HalfWidth,b.HalfWidth))
                return {};
            auto& output=result.Bands[band];
            if(output.Count>=output.Quads.size())return {};
            const double za=a.Z+OriginalRoofStripeClearanceCm;
            const double zb=b.Z+OriginalRoofStripeClearanceCm;
            output.Quads[output.Count++]={{
                OriginalVehiclePoint{xa,lo,za},OriginalVehiclePoint{xb,lo,zb},
                OriginalVehiclePoint{xb,hi,zb},OriginalVehiclePoint{xa,hi,za}},
                normal,tangent};
        }
    }
    for(const auto& band:result.Bands)
        if(band.Count==0)return {};
    result.Valid=true;
    return result;
}
} // namespace latebrake::vehiclemesh
