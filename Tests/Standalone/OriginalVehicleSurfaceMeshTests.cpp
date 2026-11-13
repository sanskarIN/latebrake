#include "Vehicles/OriginalVehicleSurfaceMesh.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::vehiclemesh;
static void Check(bool okay,const char* why)
{
    if(!okay)
    {
        std::cerr<<"Original vehicle mesh FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static std::array<std::size_t,6> AllSixOriginalCars()
{
    OriginalVehicleSurfaceComposer build;
    std::array<std::size_t,6> triangles{};
    for(int car=0;car<6;++car)
    {
        const auto result=build.Build(car);
        const auto& mesh=result.Mesh;
        Check(result.Valid && result.Fault==OriginalVehicleMeshFault::None &&
              mesh.Ready && mesh.VehicleId==car &&
              mesh.OriginalGeometry && mesh.VisualOnly &&
              !mesh.ChangesChaosContact,
              "six fictional models are original cosmetic surfaces only");
        Check(build.ValidateMesh(mesh) &&
              mesh.Vertices.size()>1000 &&
              mesh.Triangles.size()>400 &&
              mesh.Vertices.size()<=OriginalVehicleMaxVertices &&
              mesh.Triangles.size()<=OriginalVehicleMaxTriangles,
              "every car has nondegenerate bounded body and 4 wheel geometry");
        Check(mesh.WheelTireQuads==OriginalVehicleWheelSides*4*3 &&
              mesh.HeadlampQuads==2 &&
              mesh.TaillampQuads==2 &&
              mesh.WindowQuads>=6 &&
              mesh.BodyQuads>200 &&
              mesh.DetailQuads>=24,
              "four tires, lighting, glazing and body panels are all present");
        Check(mesh.Bounds.Minimum.X < -200.0 &&
              mesh.Bounds.Maximum.X > 200.0 &&
              mesh.Bounds.Minimum.Y < -90.0 &&
              mesh.Bounds.Maximum.Y > 90.0 &&
              mesh.Bounds.Maximum.Z > 150.0,
              "car proportions span a credible four-wheel body envelope");
        const auto rebuilt=build.Build(car);
        Check(rebuilt.Valid &&
              rebuilt.Mesh.Vertices.size()==mesh.Vertices.size() &&
              rebuilt.Mesh.Triangles.size()==mesh.Triangles.size(),
              "same original model regenerates deterministically");
        for(std::size_t i=0;i<mesh.Vertices.size();++i)
        {
            const auto& a=mesh.Vertices[i];
            const auto& b=rebuilt.Mesh.Vertices[i];
            Check(a.Position.X==b.Position.X &&
                  a.Position.Y==b.Position.Y &&
                  a.Position.Z==b.Position.Z &&
                  a.Normal.X==b.Normal.X &&
                  a.Normal.Y==b.Normal.Y &&
                  a.Normal.Z==b.Normal.Z,
                  "repeated geometry is bit-for-bit stable in source order");
        }
        triangles[static_cast<std::size_t>(car)]=mesh.Triangles.size();
    }
    Check(triangles[0]!=triangles[1] ||
          triangles[2]!=triangles[3],
          "garage body and aero variants have different real surface topology");
    return triangles;
}
static void InvalidBoundsAndIDsFailClosed()
{
    OriginalVehicleSurfaceComposer build;
    Check(!build.Build(-1).Valid &&
          build.Build(-1).Fault==OriginalVehicleMeshFault::InvalidVehicle &&
          !build.Build(6).Valid,
          "nonexistent garage entries never receive a fake vehicle mesh");
    OriginalVehicleMeshSettings config;
    config.LengthCm=std::numeric_limits<double>::quiet_NaN();
    Check(!OriginalVehicleSurfaceComposer(config).Build(0).Valid,
          "nan body length rejected");
    config={};
    config.WheelRadiusCm=config.RimRadiusCm;
    Check(!OriginalVehicleSurfaceComposer(config).Build(0).Valid,
          "impossible tire/rim spacing rejected");
    config={};
    config.LongitudinalSegments=10000;
    Check(!OriginalVehicleSurfaceComposer(config).Build(0).Valid,
          "unbounded detail count rejected");
    config={};
    config.NeverChangeCollision=false;
    Check(!OriginalVehicleSurfaceComposer(config).Build(0).Valid,
          "visual geometry cannot silently replace Chaos collision");
    config={};
    config.WheelOpeningClearanceCm=-1.0;
    Check(!OriginalVehicleSurfaceComposer(config).Build(0).Valid,
          "negative wheel-clearance cannot invert the body surface");
}
static void MaterialAndGeometryIntegrity()
{
    OriginalVehicleSurfaceComposer build;
    auto mesh=build.Build(3).Mesh;
    std::array<std::size_t,
        static_cast<std::size_t>(OriginalVehicleMaterial::Count)> counts{};
    for(const auto& triangle:mesh.Triangles)
        ++counts[static_cast<std::size_t>(triangle.Material)];
    for(const auto material:{
        OriginalVehicleMaterial::BodyPaint,
        OriginalVehicleMaterial::Accent,
        OriginalVehicleMaterial::ClearGlass,
        OriginalVehicleMaterial::Rubber,
        OriginalVehicleMaterial::Alloy,
        OriginalVehicleMaterial::BrakeRotor,
        OriginalVehicleMaterial::Headlamp,
        OriginalVehicleMaterial::Taillamp,
        OriginalVehicleMaterial::Intake,
        OriginalVehicleMaterial::CarbonTrim})
    {
        Check(counts[static_cast<std::size_t>(material)]>0,
              "original track variant includes every physical decoration layer");
    }
    Check(build.ValidateMesh(mesh),
          "baseline mesh validation succeeds");
    mesh.Triangles[0].Index[2]=
        static_cast<std::uint32_t>(mesh.Vertices.size()+1);
    Check(!build.ValidateMesh(mesh),
          "bad index cannot reach renderer");
    mesh=build.Build(3).Mesh;
    mesh.Vertices[0].Normal.Z=
        std::numeric_limits<double>::quiet_NaN();
    Check(!build.ValidateMesh(mesh),
          "nan normal cannot reach shader");
    mesh=build.Build(3).Mesh;
    mesh.ChangesChaosContact=true;
    Check(!build.ValidateMesh(mesh),
          "render-only model cannot claim to change contact physics");
}
static void ConfigurableGeometryLimits()
{
    OriginalVehicleMeshSettings p;
    p.WheelRadialSegments=32;
    p.LongitudinalSegments=40;
    p.CabinRoofHeightCm=190.0;
    p.WheelOpeningClearanceCm=7.0;
    auto build=OriginalVehicleSurfaceComposer(p);
    const auto mesh=build.Build(5);
    Check(mesh.Valid &&
          mesh.Mesh.WheelTireQuads==32*4*3 &&
          mesh.Mesh.BodyQuads>=40*7 &&
          mesh.Mesh.Bounds.Maximum.Z>170.0 &&
          build.ValidateMesh(mesh.Mesh),
          "configurable quality and roof envelope remain deterministic");
    p.CabinRoofHeightCm=600.0;
    Check(!OriginalVehicleSurfaceComposer(p).Build(5).Valid,
          "implausible body envelope rejected");
}
int main()
{
    const auto triangleCounts=AllSixOriginalCars();
    InvalidBoundsAndIDsFailClosed();
    MaterialAndGeometryIntegrity();
    ConfigurableGeometryLimits();
    std::cout<<"Original six-car visual surfaces validated: "<<
        triangleCounts[0]<<" triangles in baseline model\n";
}
