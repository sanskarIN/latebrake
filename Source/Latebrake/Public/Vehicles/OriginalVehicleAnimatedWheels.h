#pragma once
#include "Vehicles/OriginalVehicleRenderSections.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>

namespace latebrake::vehiclemesh
{
// The source wheel and nonwheel triangles stay in ONE validated original
// procedural model. Unlike static material-only sections, these partition
// into 5 different Unreal scene components so all four wheels can animate
// without re-uploading the entire triangle buffer on every physics frame.
inline constexpr std::size_t OriginalVehicleAnimatedPartCount=5u;
inline constexpr std::size_t OriginalVehicleAnimatedWheelCount=4u;

struct OriginalVehicleAnimatedParts final
{
    // Part 0 = body, parts 1-4 = front L, front R, rear L, rear R.
    std::array<OriginalVehicleRenderSections,
        OriginalVehicleAnimatedPartCount> Parts{};
    std::size_t TotalTriangles=0;
    bool Valid=false;
};

inline constexpr OriginalVehiclePoint OriginalWheelPivot(
    int wheelSlot, OriginalVehicleMeshSettings settings={}) noexcept
{
    if(wheelSlot<0 || wheelSlot>=4) return {};
    return {wheelSlot<2 ? settings.WheelbaseCm*0.5
                        : -settings.WheelbaseCm*0.5,
            (wheelSlot%2==0 ? -1.0 : 1.0)*settings.HalfWidthCm,
            settings.WheelCenterHeightCm};
}

// A part's tag alone cannot determine where the actor will rotate it:
// reject re-tagged triangles from the opposite axle/side, not just bad
// material IDs or empty wheel groups. This is evaluated once per rebuild,
// never in the physics/render frame update loop.
inline bool IsSourceWheelTriangleNearPivot(
    const OriginalVehicleMesh& mesh,
    const OriginalVehicleTriangle& triangle) noexcept
{
    if(triangle.WheelSlot<0 || triangle.WheelSlot>=4)
        return false;
    constexpr OriginalVehicleMeshSettings settings{};
    const auto pivot=OriginalWheelPivot(triangle.WheelSlot,settings);
    const double radial=settings.WheelRadiusCm+1.0;
    const double lateral=settings.TireWidthCm*0.5+2.0;
    for(auto index:triangle.Index)
    {
        if(index>=mesh.Vertices.size()) return false;
        const auto& point=mesh.Vertices[index].Position;
        if(!std::isfinite(point.X) || !std::isfinite(point.Y) ||
           !std::isfinite(point.Z) ||
           std::abs(point.X-pivot.X)>radial ||
           std::abs(point.Y-pivot.Y)>lateral ||
           std::abs(point.Z-pivot.Z)>radial)
            return false;
    }
    return true;
}

inline OriginalVehicleAnimatedParts MakeOriginalVehicleAnimatedParts(
    const OriginalVehicleMesh& mesh)
{
    OriginalVehicleAnimatedParts out{};
    if(!mesh.Ready || !mesh.VisualOnly || mesh.ChangesChaosContact ||
       !mesh.OriginalGeometry ||
       !OriginalVehicleSurfaceComposer{}.ValidateMesh(mesh))
        return out;

    for(std::size_t i=0;i<mesh.Triangles.size();++i)
    {
        const auto& tri=mesh.Triangles[i];
        if(tri.WheelSlot<-1 || tri.WheelSlot>=4)
            return {};
        const std::size_t material=static_cast<std::size_t>(tri.Material);
        if(material>=OriginalVehicleSectionCount)
            return {};
        const bool wheelMaterial =
            tri.Material==OriginalVehicleMaterial::Rubber ||
            tri.Material==OriginalVehicleMaterial::Alloy ||
            tri.Material==OriginalVehicleMaterial::BrakeRotor;
        if((tri.WheelSlot>=0)!=wheelMaterial ||
           (wheelMaterial &&
            !IsSourceWheelTriangleNearPivot(mesh,tri)))
            return {};
        auto& part=out.Parts[static_cast<std::size_t>(tri.WheelSlot+1)];
        part.TriangleIds[material].push_back(static_cast<std::uint32_t>(i));
        ++part.TotalTriangles;
        ++out.TotalTriangles;
    }
    if(out.TotalTriangles!=mesh.Triangles.size() ||
       out.Parts[0].TriangleIds[
           static_cast<std::size_t>(OriginalVehicleMaterial::BodyPaint)]
           .empty())
        return {};

    const auto rubber=static_cast<std::size_t>(OriginalVehicleMaterial::Rubber);
    const auto alloy=static_cast<std::size_t>(OriginalVehicleMaterial::Alloy);
    const auto rotor=static_cast<std::size_t>(OriginalVehicleMaterial::BrakeRotor);
    for(std::size_t w=1;w<OriginalVehicleAnimatedPartCount;++w)
    {
        const auto& p=out.Parts[w];
        if(p.TriangleIds[rubber].empty() || p.TriangleIds[alloy].empty() ||
           p.TriangleIds[rotor].empty())
            return {};
        // Each authored wheel has equal body-independent rubber segments;
        // a missing/duplicate tire wheel can never appear "complete."
        if(p.TriangleIds[rubber].size() !=
            out.Parts[1].TriangleIds[rubber].size() ||
           p.TriangleIds[rotor].size() !=
            out.Parts[1].TriangleIds[rotor].size())
            return {};
    }
    for(auto& part:out.Parts)
        part.Valid=part.TotalTriangles>0;
    out.Valid=true;
    return out;
}
static_assert(OriginalVehicleAnimatedWheelCount==4u);
static_assert(OriginalWheelPivot(0).X>0.0 &&
              OriginalWheelPivot(0).Y<0.0 &&
              OriginalWheelPivot(1).X>0.0 &&
              OriginalWheelPivot(1).Y>0.0 &&
              OriginalWheelPivot(2).X<0.0 &&
              OriginalWheelPivot(3).X<0.0);
} // namespace latebrake::vehiclemesh
