#include "Vehicles/OriginalVehicleShowroomCamera.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake::vehiclemesh;
void Check(bool ok,const char* why)
{
    if(!ok){std::cerr<<"ShowroomCamera: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-8)
{
    return std::abs(a-b)<=tolerance;
}
void VerifyProjection(const OriginalVehicleMesh& mesh,ShowroomOrbitRequest q)
{
    OriginalVehicleShowroomCamera camera;
    Check(camera.Fit(mesh,q)&&camera.Ready(),"fit actual original mesh");
    const auto pose=camera.Pose();
    Check(camera.VehicleId()==mesh.VehicleId&&pose.DistanceCm>0.0&&
        Near(MeshLength(pose.Forward),1.0)&&Near(MeshLength(pose.Up),1.0)&&
        Near(MeshLength(pose.Right),1.0)&&
        Near(ShowroomDot(pose.Forward,pose.Right),0.0)&&
        Near(ShowroomDot(pose.Forward,pose.Up),0.0)&&
        Near(ShowroomDot(pose.Right,pose.Up),0.0)&&
        ShowroomNearPoint(MeshCross(pose.Forward,pose.Right),pose.Up),
        "orthonormal Unreal-compatible camera basis");
    const auto center=camera.Project(pose.Target);
    Check(center.Valid&&Near(center.NdcX,0.0)&&Near(center.NdcY,0.0)&&
        Near(center.DepthCm,pose.DistanceCm),"target centered at fitted depth");
    const auto right=ShowroomAdd(pose.Target,pose.Right);
    const auto up=ShowroomAdd(pose.Target,pose.Up);
    Check(camera.Project(right).NdcX>0.0&&camera.Project(up).NdcY>0.0,
        "camera right/up project with correct sign");
    const auto b=camera.ScreenBounds();
    for(const auto& v:mesh.Vertices)
    {
        const auto p=camera.Project(v.Position);
        Check(p.Valid&&p.InsideViewport&&p.InsidePaddedViewport&&
            p.DepthCm>=pose.NearClipCm&&p.DepthCm<=pose.FarClipCm&&
            p.NdcX>=b.MinX&&p.NdcX<=b.MaxX&&p.NdcY>=b.MinY&&p.NdcY<=b.MaxY&&
            p.DepthCm>=b.MinDepthCm&&p.DepthCm<=b.MaxDepthCm,
            "all actual vertices inside padded viewport and clip range");
    }
    if(q.ZoomOut==1.0)
    {
        const double maxAbs=std::max({std::abs(b.MinX),std::abs(b.MaxX),
            std::abs(b.MinY),std::abs(b.MaxY)});
        Check(Near(maxAbs,1.0-q.Padding,1e-5)||
            Near(b.MinDepthCm,q.NearClipCm+1.0,1e-4),
            "tight fit contacts padding or near clearance instead of excessive zoom");
    }
    Check(!camera.Project(pose.Eye).Valid,"reject zero depth");
    Check(!camera.Project(ShowroomAdd(pose.Eye,
        ShowroomScale(pose.Forward,-100.0))).Valid,"reject behind camera");
    Check(!camera.Project(ShowroomAdd(pose.Eye,
        ShowroomScale(pose.Forward,pose.FarClipCm+1.0))).Valid,"reject beyond far clip");
}
void AllVehicles()
{
    for(int id=0;id<6;++id)
    {
        const auto built=OriginalVehicleSurfaceComposer{}.Build(id);
        Check(built.Valid,"source car exists");
        for(double aspect:{0.25,0.5625,1.0,16.0/9.0,4.0})
        for(double pitch:{-85.0,-20.0,0.0,18.0,45.0,85.0})
        for(double yaw:{-180.0,-90.0,0.0,35.0,90.0,180.0,395.0})
        {
            ShowroomOrbitRequest q{};
            q.AspectRatio=aspect;q.PitchDegrees=pitch;q.YawDegrees=yaw;
            VerifyProjection(built.Mesh,q);
        }
        ShowroomOrbitRequest wide{};
        wide.VerticalFovDegrees=85.0;
        wide.ZoomOut=3.0;
        VerifyProjection(built.Mesh,wide);
        ShowroomOrbitRequest narrow{};
        narrow.VerticalFovDegrees=20.0;
        narrow.Padding=0.45;
        narrow.AspectRatio=0.25;
        VerifyProjection(built.Mesh,narrow);
    }
}
void CachedGeometryParity()
{
    for(int id=0;id<6;++id)
    {
        auto mesh=OriginalVehicleSurfaceComposer{}.Build(id).Mesh;
        OriginalVehicleShowroomGeometry geometry;
        Check(geometry.Build(mesh)&&geometry.Ready()&&geometry.VehicleId()==id&&
            geometry.Points().size()==mesh.Vertices.size(),
            "one-time validated geometry snapshot");
        OriginalVehicleShowroomCamera direct,cached;
        for(double yaw:{0.0,35.0,90.0,180.0,270.0})
        {
            ShowroomOrbitRequest q{};
            q.YawDegrees=yaw;
            q.AspectRatio=0.5625;
            Check(direct.Fit(mesh,q)&&cached.FitPrepared(geometry,q)&&
                ShowroomNearPoint(direct.Pose().Eye,cached.Pose().Eye)&&
                Near(direct.Pose().DistanceCm,cached.Pose().DistanceCm),
                "cached and direct fit are equivalent");
        }
        const auto prior=cached.Pose();
        mesh.Bounds.Maximum.X+=50.0;
        Check(!geometry.Build(mesh)&&geometry.Ready()&&geometry.VehicleId()==id,
            "failed prepared geometry replacement retains snapshot");
        ShowroomOrbitRequest q{};q.YawDegrees=270.0;q.AspectRatio=0.5625;
        Check(cached.FitPrepared(geometry,q)&&
            ShowroomNearPoint(prior.Eye,cached.Pose().Eye),
            "cached geometry independent of caller mutation");
        geometry.Reset();
        Check(!geometry.Ready()&&!cached.FitPrepared(geometry,q)&&
            cached.Ready(),"reset geometry rejects fit without erasing valid camera");
    }
}
void InvalidAndAtomic()
{
    OriginalVehicleShowroomCamera camera;
    auto built=OriginalVehicleSurfaceComposer{}.Build(2);
    Check(!camera.Ready()&&!camera.Project({}).Valid&&
        !camera.Fit({}),"uninitialized camera invalid");
    Check(camera.Fit(built.Mesh),"valid baseline");
    const auto prior=camera.Pose();
    const double nan=std::numeric_limits<double>::quiet_NaN();
    for(double bad:{0.0,-1.0,0.1,5.0,nan})
    {
        ShowroomOrbitRequest q{};q.AspectRatio=bad;
        Check(!camera.Fit(built.Mesh,q),"invalid aspect rejected");
    }
    ShowroomOrbitRequest q{};q.PitchDegrees=90.0;
    Check(!camera.Fit(built.Mesh,q),"vertical singularity rejected");
    q={};q.VerticalFovDegrees=0.0;
    Check(!camera.Fit(built.Mesh,q),"invalid FOV");
    q={};q.Padding=1.0;
    Check(!camera.Fit(built.Mesh,q),"invalid padding");
    q={};q.ZoomOut=0.5;
    Check(!camera.Fit(built.Mesh,q),"zoom cannot violate fit");
    q={};q.YawDegrees=nan;
    Check(!camera.Fit(built.Mesh,q),"invalid yaw");
    auto corrupt=built.Mesh;
    corrupt.Bounds.Maximum.X+=10.0;
    Check(!camera.Fit(corrupt),"forged cached AABB rejected");
    corrupt=built.Mesh;corrupt.Vertices[0].Position.X=nan;
    Check(!camera.Fit(corrupt),"nonfinite source geometry");
    Check(camera.Ready()&&Near(camera.Pose().DistanceCm,prior.DistanceCm)&&
        ShowroomNearPoint(camera.Pose().Eye,prior.Eye),"failed fit preserves prior camera");
    Check(!camera.Project({nan,0.0,0.0}).Valid,"invalid projection point");
    camera.Reset();
    Check(!camera.Ready()&&camera.VehicleId()==-1&&!camera.Project({}).Valid,
        "reset invalidates pose");
}
}
int main()
{
    AllVehicles();CachedGeometryParity();InvalidAndAtomic();
    std::cout<<"Six original cars, 1260 orbit/viewport fits and invalid input regression PASS\n";
    return EXIT_SUCCESS;
}
