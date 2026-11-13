#include "Track/OriginalArmcoBarrierMesh.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::armco;
static void Require(bool condition,const char* message)
{
    if(!condition)
    {
        std::cerr<<"OriginalArmcoMesh FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static void CompleteOriginalBarriers()
{
    const OriginalArmcoBarrierMeshBuilder builder;
    const auto built=builder.Build();
    const auto& mesh=built.Mesh;
    Require(built.Valid && mesh.Ready &&
            built.Fault==ArmcoFault::None &&
            builder.ValidateMesh(mesh) &&
            mesh.VisualOnly && mesh.NoSeparateCollision &&
            mesh.OriginalRoadCollisionUntouched &&
            mesh.NoOfficialLapAuthority,
            "render-only barriers preserve all four original collisions");
    Require(mesh.Vertices.size()>10000 &&
            mesh.Triangles.size()>3000 &&
            mesh.Vertices.size()<=OriginalArmcoMaxVertices &&
            mesh.Triangles.size()<=OriginalArmcoMaxTriangles,
            "W-profile steel guardrails and support posts have actual geometry");
    int totalRails=0;
    int totalPosts=0;
    for(int i=0;i<4;++i)
    {
        const auto index=static_cast<std::size_t>(i);
        Require(mesh.VerifiedBarriers[index]==1 &&
                mesh.RailSections[index]>50 &&
                mesh.SteelPosts[index]>10,
                "each collision-bearing wall owns visible steel sections");
        totalRails+=mesh.RailSections[index];
        totalPosts+=mesh.SteelPosts[index];
    }
    Require(totalRails>300 &&
            totalPosts>60 &&
            mesh.CorrugationFaces>1000 &&
            mesh.EndCaps>=32 &&
            mesh.SafetyBands==20 &&
            mesh.BoltHeads==totalPosts*3,
            "each barrier has steel wave ribs, posts, bolts and marker patches");
    for(const auto& triangle:mesh.Triangles)
    {
        Require(triangle.PhysicalBarrier>=5 &&
                triangle.PhysicalBarrier<=8,
                "all triangles are owned by a real world-static wall actor");
        for(auto idx:triangle.Indices)
        {
            Require(idx<mesh.Vertices.size(),
                    "mesh index stays inside actual vertex buffer");
            const auto& vertex=mesh.Vertices[idx];
            Require(ArmcoInsidePhysicalBarrier(
                        vertex.Position,triangle.PhysicalBarrier),
                    "authored steel may not exist outside collision-bearing wall");
            Require(std::abs(ArmcoLength(vertex.Normal)-1.0)<1e-7,
                    "all surface normals are normalized");
        }
    }
    Require(mesh.Bounds.Min.Z> -35.0-1e-7 &&
            mesh.Bounds.Max.Z<165.0+1e-7,
            "renderable steel stays inside real wall top/bottom");
}
static void DeterministicPhysicalMesh()
{
    OriginalArmcoBarrierMeshBuilder builder;
    const auto first=builder.Build();
    const auto second=builder.Build();
    Require(first.Valid && second.Valid &&
            first.Mesh.Vertices.size()==second.Mesh.Vertices.size() &&
            first.Mesh.Triangles.size()==second.Mesh.Triangles.size(),
            "same real block plan always regenerates same 3D metal mesh");
    for(std::size_t i=0;i<first.Mesh.Vertices.size();++i)
    {
        const auto& a=first.Mesh.Vertices[i];
        const auto& b=second.Mesh.Vertices[i];
        Require(a.Position.X==b.Position.X &&
                a.Position.Y==b.Position.Y &&
                a.Position.Z==b.Position.Z &&
                a.Normal.X==b.Normal.X &&
                a.Normal.Y==b.Normal.Y &&
                a.Normal.Z==b.Normal.Z,
                "repeated geometric samples are bit-identical");
    }
    for(std::size_t i=0;i<first.Mesh.Triangles.size();++i)
    {
        const auto& a=first.Mesh.Triangles[i];
        const auto& b=second.Mesh.Triangles[i];
        Require(a.Indices==b.Indices &&
                a.PhysicalBarrier==b.PhysicalBarrier &&
                a.Layer==b.Layer,
                "deterministic material and collision-owner assignments");
    }
}
static void FailClosedInvalidInputs()
{
    OriginalArmcoBarrierMeshBuilder valid;
    Require(ArmcoSettingsValid({}),"default original wall profile valid");
    ArmcoSettings settings;
    settings.NeverCreateIndependentCollision=false;
    Require(!OriginalArmcoBarrierMeshBuilder(settings).Build().Valid,
            "preview may not create a second collision body");
    settings={};
    settings.NeverInfluenceRaceClassification=false;
    Require(!OriginalArmcoBarrierMeshBuilder(settings).Build().Valid,
            "metal geometry cannot grant laps or rewards");
    settings={};
    settings.WaveDepthCm=7.0;
    Require(!OriginalArmcoBarrierMeshBuilder(settings).Build().Valid,
            "overdeep corrugation cannot extend beyond real wall");
    settings={};
    settings.RailSamplingLengthCm=std::numeric_limits<double>::quiet_NaN();
    Require(!OriginalArmcoBarrierMeshBuilder(settings).Build().Valid,
            "invalid tessellation cannot create source geometry");
    settings={};
    settings.PostSpacingCm=0.0;
    Require(!OriginalArmcoBarrierMeshBuilder(settings).Build().Valid,
            "zero post spacing must fail before allocating");
    auto mesh=valid.Build().Mesh;
    mesh.Triangles[0].Indices[0]=
        static_cast<std::uint32_t>(mesh.Vertices.size()+2);
    Require(!valid.ValidateMesh(mesh),
            "invalid triangle index rejected");
    mesh=valid.Build().Mesh;
    mesh.Vertices[0].Position={0.0,0.0,0.0};
    Require(!valid.ValidateMesh(mesh),
            "infield steel not in physical wall must reject");
    mesh=valid.Build().Mesh;
    mesh.NoSeparateCollision=false;
    Require(!valid.ValidateMesh(mesh),
            "existing collision can never be replaced silently");
}
static void DensityAndSectionGuards()
{
    ArmcoSettings p;
    p.RailSamplingLengthCm=120.0;
    p.MaximumSectionLengthCm=175.0;
    p.PostSpacingCm=300.0;
    OriginalArmcoBarrierMeshBuilder detailed(p);
    auto output=detailed.Build();
    Require(output.Valid &&
            detailed.ValidateMesh(output.Mesh) &&
            output.Mesh.CorrugationFaces>1500,
            "higher-detail steel remains physically backed");
    p.InnerFaceDepthCm=50.0;
    Require(!OriginalArmcoBarrierMeshBuilder(p).Build().Valid,
            "outer face cannot cross the actual wall collision envelope");
    Require(!ArmcoInsidePhysicalBarrier({0.0,0.0,0.0},5),
            "road interior is not an original barrier");
}
int main()
{
    CompleteOriginalBarriers();
    DeterministicPhysicalMesh();
    FailClosedInvalidInputs();
    DensityAndSectionGuards();
    std::cout<<"Four original Armco physical-wall visual meshes passed geometry and source validation\n";
}
