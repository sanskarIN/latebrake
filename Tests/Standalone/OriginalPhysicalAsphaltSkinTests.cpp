#include "Track/OriginalPhysicalAsphaltSkin.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::trackskin;
static void Require(bool pass,const char* why)
{
    if(!pass)
    {
        std::cerr<<"OriginalAsphaltSkin FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static void PhysicalOnlyGeometry()
{
    const OriginalPhysicalAsphaltSkin generator;
    const auto result=generator.Build();
    const auto& mesh=result.Mesh;
    Require(result.Valid && mesh.Ready &&
            result.Fault==AsphaltSkinFault::None &&
            mesh.VisualOnly &&
            !mesh.ChangesRaceTiming &&
            !mesh.HasIndependentCollision,
            "authored asphalt skin is read-only and cannot replace Chaos");
    Require(generator.ValidateMesh(mesh) &&
            mesh.BaseQuads>400 &&
            mesh.RubberPatches>0 &&
            mesh.SurfaceRepairs>0 &&
            mesh.JointSeams>0 &&
            mesh.Triangles.size()==mesh.LayerQuads[0]*2U+
                mesh.LayerQuads[1]*2U+
                mesh.LayerQuads[2]*2U+
                mesh.LayerQuads[3]*2U+
                mesh.LayerQuads[4]*2U+
                mesh.LayerQuads[5]*2U,
            "base, joints, wear and repairs have reconciled geometry");
    std::array<int,4> slabCounts{};
    for(const auto& tri:mesh.Triangles)
    {
        Require(tri.SourceSlab>=1 && tri.SourceSlab<=4,
                "all panels belong to one of four physical road slabs");
        ++slabCounts[static_cast<std::size_t>(tri.SourceSlab-1)];
        for(auto idx:tri.Indices)
        {
            Require(idx<mesh.Vertices.size(),
                    "triangulation index is within the authored mesh");
            const auto& p=mesh.Vertices[idx].Position;
            Require(OriginalSkinPhysicalAsphalt(p.X,p.Y),
                    "every rendered triangle corner lies on physical road");
            Require(p.Z>-1.0 && p.Z<-0.5,
                    "non-colliding skin floats a fraction above actual asphalt");
        }
    }
    for(int count:slabCounts)
        Require(count>0,"all four collision slabs have visible skin");
    Require(mesh.IgnoredOverlappingSections>0,
            "one physical slab owns each intersection; no overlapping skins");
    Require(mesh.Vertices.size()<=MaximumSkinVertices &&
            mesh.Triangles.size()<=MaximumSkinTriangles,
            "bounded raster topology avoids unbounded runtime allocation");
}
static void OriginalGeometryDeterminism()
{
    const OriginalPhysicalAsphaltSkin generator;
    const auto first=generator.Build();
    const auto second=generator.Build();
    Require(first.Valid && second.Valid &&
            first.Mesh.BaseQuads==second.Mesh.BaseQuads &&
            first.Mesh.RubberPatches==second.Mesh.RubberPatches &&
            first.Mesh.SurfaceRepairs==second.Mesh.SurfaceRepairs &&
            first.Mesh.JointSeams==second.Mesh.JointSeams &&
            first.Mesh.Triangles.size()==second.Mesh.Triangles.size(),
            "identical physical circuit always builds identical topology");
    for(std::size_t i=0;i<first.Mesh.Vertices.size();++i)
    {
        const auto& a=first.Mesh.Vertices[i];
        const auto& b=second.Mesh.Vertices[i];
        Require(a.Position.X==b.Position.X &&
                a.Position.Y==b.Position.Y &&
                a.Position.Z==b.Position.Z &&
                a.Normal.Z==b.Normal.Z &&
                a.Uv.U==b.Uv.U &&
                a.Uv.V==b.Uv.V,
                "geometry stable across repeated builds without random state");
    }
    for(std::size_t i=0;i<first.Mesh.Triangles.size();++i)
    {
        const auto& a=first.Mesh.Triangles[i];
        const auto& b=second.Mesh.Triangles[i];
        Require(a.Indices==b.Indices &&
                a.Layer==b.Layer &&
                a.SourceSlab==b.SourceSlab,
                "material and physical slab provenance are deterministic");
    }
}
static void ConfigurationGuards()
{
    AsphaltSkinSettings config;
    Require(ValidAsphaltSkinSettings(config),
            "defaults satisfy all source safety requirements");
    config.DoNotModifyCollision=false;
    Require(!OriginalPhysicalAsphaltSkin(config).Build().Valid,
            "no profile may enable authored road collision");
    config={};
    config.DoNotAwardRaceCredits=false;
    Require(!OriginalPhysicalAsphaltSkin(config).Build().Valid,
            "no visual road mesh may change race rewards");
    config={};
    config.TargetCellCm=std::numeric_limits<double>::infinity();
    Require(!OriginalPhysicalAsphaltSkin(config).Build().Valid,
            "nonfinite tiling rejected before allocation");
    config={};
    config.SeamEveryTiles=0;
    Require(!OriginalPhysicalAsphaltSkin(config).Build().Valid,
            "zero seam interval cannot cause modulo by zero");
    config={};
    config.RubberEveryTiles=-1;
    Require(!OriginalPhysicalAsphaltSkin(config).Build().Valid,
            "negative wear repetition cannot trigger undefined behavior");
    config={};
    config.RoadTopClearanceCm=-0.5;
    Require(!OriginalPhysicalAsphaltSkin(config).Build().Valid,
            "no skin may be buried inside collision");
    config={};
    config.MaximumWorldCoordinateCm=100.0;
    Require(!OriginalPhysicalAsphaltSkin(config).Build().Valid,
            "world bounds must contain original physical track");
}
static void OriginalRoadMembership()
{
    Require(OriginalSkinPhysicalAsphalt(
                latebrake::PlannedCollisionBlock(1).Center.X,
                latebrake::PlannedCollisionBlock(1).Center.Y),
            "south straight remains physical asphalt");
    Require(OriginalSkinPhysicalAsphalt(
                latebrake::PlannedCollisionBlock(3).Center.X,
                latebrake::PlannedCollisionBlock(3).Center.Y),
            "east straight remains physical asphalt");
    Require(!OriginalSkinPhysicalAsphalt(0.0,0.0),
            "infield grass never becomes a fake road");
    Require(!OriginalSkinPhysicalAsphalt(12000.0,0.0),
            "outside world is not a valid road");
    Require(!OriginalSkinPhysicalAsphalt(
                std::numeric_limits<double>::quiet_NaN(),0.0),
            "nonfinite location cannot be painted as asphalt");
    Require(std::abs(SkinSlabTop(
                latebrake::PlannedCollisionBlock(1))+1.0)<1e-9,
            "visual height anchored to actual -1cm top of road");
}
static void SafeCustomization()
{
    AsphaltSkinSettings config;
    config.TargetCellCm=180.0;
    config.MaximumCellCm=250.0;
    config.MinimumCellCm=70.0;
    config.RepairEveryTiles=17;
    config.SeamEveryTiles=12;
    config.RubberEveryTiles=11;
    const OriginalPhysicalAsphaltSkin generator(config);
    const auto output=generator.Build();
    Require(output.Valid && generator.ValidateMesh(output.Mesh),
            "denser source-validated tiling remains original and bounded");
    Require(output.Mesh.BaseQuads>600 &&
            output.Mesh.RubberPatches>0,
            "visual detail scales with genuine original road area");
    auto invalid=output.Mesh;
    invalid.Vertices[0].Position.X=0.0;
    invalid.Vertices[0].Position.Y=0.0;
    Require(!generator.ValidateMesh(invalid),
            "source validator rejects a surface moved onto grass");
    invalid=output.Mesh;
    invalid.Triangles[0].Indices[0]=
        static_cast<std::uint32_t>(invalid.Vertices.size()+1);
    Require(!generator.ValidateMesh(invalid),
            "source validator rejects a broken mesh index");
    invalid=output.Mesh;
    invalid.HasIndependentCollision=true;
    Require(!generator.ValidateMesh(invalid),
            "source validator rejects unauthorized visual physics");
}
int main()
{
    PhysicalOnlyGeometry();
    OriginalGeometryDeterminism();
    ConfigurationGuards();
    OriginalRoadMembership();
    SafeCustomization();
    std::cout<<"Original 4-slab physical asphalt skin: bounded, collision-free and deterministic\n";
}
