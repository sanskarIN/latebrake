#include "Vehicles/OriginalVehicleAnimatedWheels.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace
{
void Require(bool value,const char* why)
{
    if(!value){std::cerr<<"Animated native wheel partition: "<<why<<'\n';
        std::exit(EXIT_FAILURE);}
}
void AllCarsAllRims()
{
    using namespace latebrake::vehiclemesh;
    OriginalVehicleSurfaceComposer composer;
    for(int car=0;car<6;++car)
        for(int rim=0;rim<OriginalVehicleRimStyleCount;++rim)
        {
            const auto original=composer.Build(car,rim);
            Require(original.Valid && original.Mesh.Ready,
                "all 18 existing car/rim variants must build");
            const auto& mesh=original.Mesh;
            const auto staticParts=MakeOriginalVehicleRenderSections(mesh);
            const auto parts=MakeOriginalVehicleAnimatedParts(mesh);
            Require(parts.Valid && staticParts.Valid &&
                parts.TotalTriangles==staticParts.TotalTriangles &&
                parts.TotalTriangles==mesh.Triangles.size(),
                "every original triangle is preserved once");
            std::array<bool,4> observedWheel{};
            std::size_t observed=0;
            for(std::size_t group=0;group<parts.Parts.size();++group)
            {
                const auto& part=parts.Parts[group];
                Require(part.Valid && part.TotalTriangles>0,
                    "body and all four wheels must have visible geometry");
                std::size_t groupCount=0;
                for(std::size_t material=0;
                    material<OriginalVehicleSectionCount;++material)
                {
                    std::uint32_t previous=0;
                    bool first=true;
                    for(auto id:part.TriangleIds[material])
                    {
                        Require(id<mesh.Triangles.size(),
                            "no render section can read outside source mesh");
                        const auto& tri=mesh.Triangles[id];
                        Require((group==0 && tri.WheelSlot==-1) ||
                            (group>0 && tri.WheelSlot==
                                static_cast<int>(group)-1),
                            "wheel ownership is correctly tagged by authoring, not guessed");
                        Require(static_cast<std::size_t>(tri.Material)==material,
                            "original ten-material car palette is unchanged");
                        Require(first || id>previous,
                            "stable original triangle ordering per material");
                        previous=id; first=false;
                        ++observed; ++groupCount;
                        if(group>0)observedWheel[group-1]=true;
                    }
                }
                Require(groupCount==part.TotalTriangles,
                    "no drops or duplicates in one animated component");
            }
            for(bool seen:observedWheel)
                Require(seen,"one of four independently animated wheels missing");
            Require(observed==mesh.Triangles.size(),
                "complete triangle conservation across 5 render components");
            const auto rebuild=MakeOriginalVehicleAnimatedParts(mesh);
            Require(rebuild.Valid && rebuild.TotalTriangles==parts.TotalTriangles,
                "same source grouping has repeatable content");
            for(std::size_t group=0;group<parts.Parts.size();++group)
                for(std::size_t material=0;material<OriginalVehicleSectionCount;++material)
                    Require(parts.Parts[group].TriangleIds[material]==
                        rebuild.Parts[group].TriangleIds[material],
                        "deterministic part and surface material ordering");
        }
}
void RejectBrokenOwnership()
{
    using namespace latebrake::vehiclemesh;
    OriginalVehicleSurfaceComposer composer;
    auto mesh=composer.Build(3,2).Mesh;
    Require(!MakeOriginalVehicleAnimatedParts({}).Valid,
        "cannot render empty source as a valid wheel set");
    mesh.Triangles[0].WheelSlot=8;
    Require(!composer.ValidateMesh(mesh) &&
        !MakeOriginalVehicleAnimatedParts(mesh).Valid,
        "bogus slot rejected before touching UE materials");
    mesh=composer.Build(3,2).Mesh;
    for(auto& triangle:mesh.Triangles)
        if(triangle.WheelSlot==0){triangle.WheelSlot=-1;break;}
    Require(!composer.ValidateMesh(mesh) &&
        !MakeOriginalVehicleAnimatedParts(mesh).Valid,
        "orphan wheel triangle never silently becomes body mesh");
    mesh=composer.Build(3,2).Mesh;
    mesh.ChangesChaosContact=true;
    Require(!MakeOriginalVehicleAnimatedParts(mesh).Valid,
        "render-only animation can never become physics collision");
    mesh=composer.Build(3,2).Mesh;
    mesh.Triangles[0].Material=static_cast<OriginalVehicleMaterial>(255);
    Require(!MakeOriginalVehicleAnimatedParts(mesh).Valid,
        "unknown palette tag cannot enter animated materials");
}
void WheelCentersHaveNoDrift()
{
    using namespace latebrake::vehiclemesh;
    const auto s=OriginalVehicleMeshSettings{};
    for(int slot=0;slot<4;++slot)
    {
        const auto p=OriginalWheelPivot(slot);
        Require(std::isfinite(p.X)&&std::isfinite(p.Y)&&std::isfinite(p.Z),
            "all pivots finite");
        Require(std::abs(p.X)==s.WheelbaseCm/2.0 &&
            std::abs(p.Y)==s.HalfWidthCm &&
            p.Z==s.WheelCenterHeightCm,
            "four original wheels pivot at actual source wheel centers");
    }
    const auto bad=OriginalWheelPivot(-1);
    Require(bad.X==0.0 && bad.Y==0.0 && bad.Z==0.0,
        "invalid wheel index never invents a physical location");
}
}
int main()
{
    AllCarsAllRims();
    RejectBrokenOwnership();
    WheelCentersHaveNoDrift();
    std::cout<<"All 18 original cars/rims and four wheel pivot groups pass.\n";
    return EXIT_SUCCESS;
}
