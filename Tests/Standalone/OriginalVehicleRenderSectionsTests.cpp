#include "Vehicles/OriginalVehicleRenderSections.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool yes,const char* why)
{
    if (!yes) { std::cerr<<"Original UE render sections: "<<why<<'\n';
                std::exit(EXIT_FAILURE); }
}
void AllCarSections()
{
    using namespace latebrake::vehiclemesh;
    const OriginalVehicleSurfaceComposer composer;
    for (int car=0;car<6;++car)
    {
        const auto original=composer.Build(car);
        Require(original.Valid && original.Mesh.Ready,
                "original generator creates car before section partition");
        const auto beforeTriangles=original.Mesh.Triangles.size();
        const auto beforeVertices=original.Mesh.Vertices.size();
        const auto sections=MakeOriginalVehicleRenderSections(original.Mesh);
        Require(sections.Valid && sections.TotalTriangles==beforeTriangles,
                "every source triangle survives material section grouping");
        std::size_t observed=0;
        for(std::size_t material=0;material<OriginalVehicleSectionCount;
            ++material)
        {
            std::size_t prior=0;
            bool first=true;
            for(const auto triangleId:sections.TriangleIds[material])
            {
                Require(triangleId<original.Mesh.Triangles.size(),
                        "material section never references missing triangle");
                Require(static_cast<std::size_t>(
                    original.Mesh.Triangles[triangleId].Material)==material,
                    "each material section owns only its source tag");
                Require(first || triangleId>prior,
                    "triangle ordering stays stable within material group");
                prior=triangleId;first=false;
                ++observed;
            }
        }
        Require(observed==beforeTriangles &&
                original.Mesh.Vertices.size()==beforeVertices &&
                original.Mesh.Triangles.size()==beforeTriangles,
                "render partition never drops geometry or edits source mesh");
        // No default active material section can fabricate physics authority.
        Require(original.Mesh.VisualOnly &&
                !original.Mesh.ChangesChaosContact,
                "collision and vehicle grip remain outside renderer authority");
        const auto repeated=MakeOriginalVehicleRenderSections(original.Mesh);
        Require(repeated.Valid && repeated.TotalTriangles==observed,
                "partition produces deterministic count on every run");
        for(std::size_t i=0;i<OriginalVehicleSectionCount;++i)
            Require(repeated.TriangleIds[i]==sections.TriangleIds[i],
                    "every per-material source triangle retains exact order");
    }
}
void InvalidMeshingCannotRender()
{
    using namespace latebrake::vehiclemesh;
    OriginalVehicleSurfaceComposer composer;
    auto mesh=composer.Build(0).Mesh;
    mesh.Ready=false;
    Require(!MakeOriginalVehicleRenderSections(mesh).Valid,
            "unvalidated source model never enters renderer");
    mesh=composer.Build(0).Mesh;
    mesh.ChangesChaosContact=true;
    Require(!MakeOriginalVehicleRenderSections(mesh).Valid,
            "physical-contact changes rejected, even for visually sound mesh");
    mesh=composer.Build(0).Mesh;
    mesh.Triangles[0].Index[0]=std::numeric_limits<std::uint32_t>::max();
    Require(!MakeOriginalVehicleRenderSections(mesh).Valid,
            "out of range vertex indices rejected before any GPU section");
    mesh=composer.Build(0).Mesh;
    mesh.Triangles[0].Material=
        static_cast<OriginalVehicleMaterial>(255u);
    Require(!MakeOriginalVehicleRenderSections(mesh).Valid,
            "unknown material ID rejected rather than miscolored");
    mesh=composer.Build(0).Mesh;
    mesh.Vertices[0].Position.X=
        std::numeric_limits<double>::quiet_NaN();
    Require(!MakeOriginalVehicleRenderSections(mesh).Valid,
            "nonfinite geometry never reaches Unreal render mesh");
    Require(!MakeOriginalVehicleRenderSections({}).Valid,
            "empty source mesh cannot hide starter fallback car");
}
}
int main()
{
    AllCarSections();
    InvalidMeshingCannotRender();
    std::cout<<"Original six-car Unreal render sections passed.\n";
    return EXIT_SUCCESS;
}
