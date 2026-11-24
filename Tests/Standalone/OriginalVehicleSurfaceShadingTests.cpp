#include "Vehicles/OriginalVehicleSurfaceShading.h"
#include "Vehicles/OriginalVehicleObjPreview.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

namespace
{
using namespace latebrake::vehiclemesh;
void Check(bool valid,const char* why)
{
    if(!valid){std::cerr<<"OriginalSurfaceShading: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Close(OriginalVehiclePoint a,OriginalVehiclePoint b,double eps=1e-8)
{
    return MeshLength(MeshSubtract(a,b))<=eps;
}
void AppendTriangle(OriginalVehicleMesh& mesh,OriginalVehiclePoint a,
    OriginalVehiclePoint b,OriginalVehiclePoint c,
    OriginalVehicleMaterial m=OriginalVehicleMaterial::BodyPaint,
    int wheel=-1,bool mirrored=false,bool degenerateUv=false)
{
    OriginalVehiclePoint n{};
    Check(SurfaceNormalize(MeshCross(MeshSubtract(b,a),MeshSubtract(c,a)),n),
        "fixture triangle is nondegenerate");
    const auto i=static_cast<std::uint32_t>(mesh.Vertices.size());
    mesh.Vertices.push_back({a,n,{0.0,0.0}});
    mesh.Vertices.push_back({b,n,degenerateUv?OriginalVehicleUv{}:
        OriginalVehicleUv{mirrored?-1.0:1.0,0.0}});
    mesh.Vertices.push_back({c,n,degenerateUv?OriginalVehicleUv{}:
        OriginalVehicleUv{0.0,1.0}});
    mesh.Triangles.push_back({{i,i+1u,i+2u},m,static_cast<std::int8_t>(wheel)});
}
OriginalVehicleMesh Fixture()
{
    auto mesh=OriginalVehicleSurfaceComposer{}.Build(0).Mesh;
    mesh.Vertices.clear();mesh.Triangles.clear();
    // Existing valid metadata and finite bounds are retained. Fixture
    // triangles lie inside them; the shading policy never edits bounds.
    return mesh;
}
OriginalVehicleMesh Fold(double degrees,
    OriginalVehicleMaterial m=OriginalVehicleMaterial::BodyPaint,
    int wheel=-1)
{
    auto mesh=Fixture();
    const double rad=degrees*OriginalVehiclePi/180.0;
    AppendTriangle(mesh,{0,0,30},{10,0,30},{0,10,30},m,wheel);
    AppendTriangle(mesh,{10,0,30},{0,0,30},
        {0,-10*std::cos(rad),30+10*std::sin(rad)},m,wheel);
    return mesh;
}
void SmoothAndHardEdges()
{
    const auto smooth=Fold(30.0);
    const auto p=BuildOriginalSurfaceShading(smooth);
    Check(p.Valid&&p.SmoothEdges==1&&p.BoundaryEdges==4&&
        p.ChangedNormals==4,"only two adjacent corners of each face smooth");
    Check(Close(p.Frames[0].Normal,p.Frames[4].Normal)&&
        Close(p.Frames[1].Normal,p.Frames[3].Normal),
        "welded edge shares angle-weighted normal on both faces");
    Check(Close(p.Frames[2].Normal,smooth.Vertices[2].Normal)&&
        Close(p.Frames[5].Normal,smooth.Vertices[5].Normal),
        "unconnected corners retain original face normal");
    const auto hard=Fold(80.0);
    const auto q=BuildOriginalSurfaceShading(hard);
    Check(q.Valid&&q.HardEdges==1&&q.SmoothEdges==0&&q.ChangedNormals==0,
        "hard crease preserved");
    auto different=Fold(30.0);
    different.Triangles[1].Material=OriginalVehicleMaterial::ClearGlass;
    Check(BuildOriginalSurfaceShading(different).SmoothEdges==0,
        "paint and glass never share a normal group");
    auto wheels=Fold(30.0,OriginalVehicleMaterial::Rubber,0);
    wheels.Triangles[1].WheelSlot=1;
    Check(BuildOriginalSurfaceShading(wheels).SmoothEdges==0,
        "independent animated wheels never share a normal group");
    auto metal=Fold(30.0,OriginalVehicleMaterial::Alloy,0);
    Check(BuildOriginalSurfaceShading(metal).SmoothEdges==0,
        "hard alloy detail preserved");
    OriginalSurfaceShadingSettings settings{};settings.SmoothBody=false;
    Check(BuildOriginalSurfaceShading(smooth,settings).ChangedNormals==0,
        "disabled smoothing leaves authored normal unchanged");
}
void TopologyAndTangents()
{
    auto point=Fixture();
    AppendTriangle(point,{0,0,30},{10,0,30},{0,10,30});
    AppendTriangle(point,{0,0,30},{-10,0,30},{0,-10,35});
    auto p=BuildOriginalSurfaceShading(point);
    Check(p.Valid&&p.SmoothEdges==0&&p.ChangedNormals==0,
        "point-only contact never joins disconnected sheets");
    auto multi=Fold(30.0);
    AppendTriangle(multi,{10,0,30},{0,0,30},{0,-10,25});
    p=BuildOriginalSurfaceShading(multi);
    Check(p.Valid&&p.NonManifoldEdges==1&&p.SmoothEdges==0,
        "three faces on edge do not create an ambiguous smoothing group");
    auto uv=Fixture();
    AppendTriangle(uv,{0,0,30},{10,0,30},{0,10,30});
    AppendTriangle(uv,{20,0,30},{30,0,30},{20,10,30},
        OriginalVehicleMaterial::BodyPaint,-1,true);
    AppendTriangle(uv,{40,0,30},{50,0,30},{40,10,30},
        OriginalVehicleMaterial::BodyPaint,-1,false,true);
    p=BuildOriginalSurfaceShading(uv);
    Check(p.Valid&&p.FallbackTangents==3&&
        !p.Frames[0].FlipTangentY&&p.Frames[3].FlipTangentY,
        "ordinary mirrored and degenerate UV tangent contracts");
    for(const auto& frame:p.Frames)
        Check(std::abs(SurfaceDot(frame.Normal,frame.Tangent))<1e-8&&
            std::abs(MeshLength(frame.Tangent)-1.0)<1e-8,
            "all fallback and UV tangents are unit and orthogonal");
    auto invalid=Fold(30.0);
    invalid.Triangles[1].Index[0]=invalid.Triangles[0].Index[1];
    Check(!BuildOriginalSurfaceShading(invalid).Valid,
        "shared source indices rejected rather than partially rewritten");
    invalid=Fold(30.0);invalid.Vertices[0].Normal={0,0,-1};
    Check(!BuildOriginalSurfaceShading(invalid).Valid,"forged authored normal refused");
    OriginalSurfaceShadingSettings bad{};bad.CreaseDegrees=100.0;
    Check(!BuildOriginalSurfaceShading(uv,bad).Valid,"invalid settings refused");
    bad={};bad.WeldToleranceCm=std::numeric_limits<double>::quiet_NaN();
    Check(!BuildOriginalSurfaceShading(uv,bad).Valid,"nonfinite grid refused");
}
void SixCarsAllRims()
{
    for(int car=0;car<6;++car)
    for(int rim=0;rim<OriginalVehicleRimStyleCount;++rim)
    {
        const auto source=OriginalVehicleSurfaceComposer{}.Build(car,rim);
        Check(source.Valid,"actual six-car source mesh");
        const auto p=BuildOriginalSurfaceShading(source.Mesh);
        const auto q=BuildOriginalSurfaceShading(source.Mesh);
        Check(p.Valid&&q.Valid&&p.Frames.size()==source.Mesh.Vertices.size()&&
            p.SmoothEdges>0&&p.HardEdges>0&&p.ChangedNormals>0&&
            p.ChangedNormals==q.ChangedNormals,"real body and wheel shading changed");
        for(std::size_t i=0;i<p.Frames.size();++i)
        {
            const auto& a=p.Frames[i];const auto& b=q.Frames[i];
            Check(Close(a.Normal,b.Normal)&&Close(a.Tangent,b.Tangent)&&
                a.FlipTangentY==b.FlipTangentY,"deterministic frame generation");
            Check(SurfaceDot(a.Normal,source.Mesh.Vertices[i].Normal)>=0.5-1e-8&&
                std::abs(SurfaceDot(a.Normal,a.Tangent))<1e-8,
                "normal stays in original surface hemisphere");
        }
        std::ostringstream obj;
        Check(WriteOriginalVehicleObj(obj,source.Mesh,"car.mtl"),
            "OBJ consumes the same valid shading frame");
        std::istringstream lines(obj.str());
        std::string line;
        std::size_t index=0;
        while(std::getline(lines,line))
        {
            if(line.rfind("vn ",0)!=0)continue;
            std::istringstream normal(line.substr(3));
            OriginalVehiclePoint decoded{};
            Check(index<p.Frames.size()&&
                static_cast<bool>(normal>>decoded.X>>decoded.Z>>decoded.Y),
                "parse authored OBJ normal");
            decoded.Y=-decoded.Y;
            Check(Close(decoded,p.Frames[index].Normal,2e-6),
                "OBJ normal equals native render frame after coordinate mapping");
            ++index;
        }
        Check(index==p.Frames.size(),"every source corner normal exported");
    }
}
}
int main()
{
    SmoothAndHardEdges();TopologyAndTangents();SixCarsAllRims();
    std::cout<<"Crease-aware shading, UV tangents and 18 original car/rim OBJ pairs passed\n";
    return EXIT_SUCCESS;
}
