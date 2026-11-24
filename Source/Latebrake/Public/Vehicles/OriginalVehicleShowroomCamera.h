#pragma once

#include "Vehicles/OriginalVehicleSurfaceMesh.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake::vehiclemesh
{
// Camera geometry for an isolated garage preview. No physics actor,
// engine camera, visibility query or authoritative race state is touched.
// Source coordinates are centimetres, X forward, Y lateral, Z up.
inline constexpr double ShowroomCameraMinAspect=0.25;
inline constexpr double ShowroomCameraMaxAspect=4.0;
inline constexpr double ShowroomCameraMinFov=20.0;
inline constexpr double ShowroomCameraMaxFov=85.0;
inline constexpr double ShowroomCameraMaxPitch=85.0;
inline constexpr double ShowroomCameraMaxDistanceCm=25000.0;
inline constexpr double ShowroomCameraDegreesToRadians=OriginalVehiclePi/180.0;

inline double ShowroomDot(OriginalVehiclePoint a,OriginalVehiclePoint b) noexcept
{
    return a.X*b.X+a.Y*b.Y+a.Z*b.Z;
}
inline OriginalVehiclePoint ShowroomAdd(
    OriginalVehiclePoint a,OriginalVehiclePoint b) noexcept
{
    return {a.X+b.X,a.Y+b.Y,a.Z+b.Z};
}
inline OriginalVehiclePoint ShowroomScale(
    OriginalVehiclePoint v,double scale) noexcept
{
    return {v.X*scale,v.Y*scale,v.Z*scale};
}
inline bool ShowroomNearPoint(
    OriginalVehiclePoint a,OriginalVehiclePoint b,double tolerance=1e-6) noexcept
{
    return MeshFinitePoint(a)&&MeshFinitePoint(b)&&
        MeshLength(MeshSubtract(a,b))<=tolerance;
}

struct ShowroomOrbitRequest final
{
    double YawDegrees=35.0;
    double PitchDegrees=18.0;
    double VerticalFovDegrees=42.0;
    double AspectRatio=16.0/9.0;
    // Fraction of each half-frustum reserved as padding.
    double Padding=0.12;
    double NearClipCm=10.0;
    // Multiplicative additional distance; 1 gives the tightest padded fit.
    double ZoomOut=1.0;
};

inline bool ValidShowroomOrbitRequest(ShowroomOrbitRequest q) noexcept
{
    return std::isfinite(q.YawDegrees)&&std::abs(q.YawDegrees)<=1e9&&
        std::isfinite(q.PitchDegrees)&&std::abs(q.PitchDegrees)<=ShowroomCameraMaxPitch&&
        std::isfinite(q.VerticalFovDegrees)&&q.VerticalFovDegrees>=ShowroomCameraMinFov&&
        q.VerticalFovDegrees<=ShowroomCameraMaxFov&&
        std::isfinite(q.AspectRatio)&&q.AspectRatio>=ShowroomCameraMinAspect&&
        q.AspectRatio<=ShowroomCameraMaxAspect&&
        std::isfinite(q.Padding)&&q.Padding>=0.0&&q.Padding<=0.45&&
        std::isfinite(q.NearClipCm)&&q.NearClipCm>=1.0&&q.NearClipCm<=100.0&&
        std::isfinite(q.ZoomOut)&&q.ZoomOut>=1.0&&q.ZoomOut<=4.0;
}

struct ShowroomProjectedPoint final
{
    bool Valid=false;
    double NdcX=0.0; // -1 left, +1 right
    double NdcY=0.0; // -1 bottom, +1 top
    double DepthCm=0.0;
    bool InsideViewport=false;
    bool InsidePaddedViewport=false;
};

struct ShowroomCameraPose final
{
    OriginalVehiclePoint Eye{};
    OriginalVehiclePoint Target{};
    OriginalVehiclePoint Right{};
    OriginalVehiclePoint Up{};
    OriginalVehiclePoint Forward{};
    double DistanceCm=0.0;
    double NearClipCm=0.0;
    double FarClipCm=0.0;
    double VerticalFovDegrees=0.0;
    double HorizontalFovDegrees=0.0;
    double AspectRatio=0.0;
    double Padding=0.0;
};

struct ShowroomScreenBounds final
{
    double MinX=0.0,MinY=0.0,MaxX=0.0,MaxY=0.0;
    double MinDepthCm=0.0,MaxDepthCm=0.0;
};

// Own the fitted pose. A caller cannot mutate a cached basis or clip
// plane and then submit it as a valid perspective camera.
class OriginalVehicleShowroomCamera final
{
public:
    bool Ready() const noexcept { return ready_; }
    int VehicleId() const noexcept { return vehicleId_; }
    ShowroomCameraPose Pose() const noexcept { return pose_; }
    ShowroomScreenBounds ScreenBounds() const noexcept { return bounds_; }

    void Reset() noexcept { *this=OriginalVehicleShowroomCamera{}; }

    // Strong replacement: failed request or malformed source leaves
    // the previous valid camera intact, useful during resize/orbit UI.
    bool Fit(const OriginalVehicleMesh& mesh,ShowroomOrbitRequest q={}) noexcept
    {
        if(!ValidShowroomOrbitRequest(q)||!mesh.Ready||
           !OriginalVehicleSurfaceComposer{}.ValidateMesh(mesh))return false;

        // Derive bounds from the actual vertices, not a potentially
        // forged cached AABB. Reject disagreement rather than crop.
        OriginalVehicleBounds actual{};
        for(const auto& vertex:mesh.Vertices)
        {
            const auto p=vertex.Position;
            actual.Minimum.X=std::min(actual.Minimum.X,p.X);
            actual.Minimum.Y=std::min(actual.Minimum.Y,p.Y);
            actual.Minimum.Z=std::min(actual.Minimum.Z,p.Z);
            actual.Maximum.X=std::max(actual.Maximum.X,p.X);
            actual.Maximum.Y=std::max(actual.Maximum.Y,p.Y);
            actual.Maximum.Z=std::max(actual.Maximum.Z,p.Z);
        }
        if(!ShowroomNearPoint(actual.Minimum,mesh.Bounds.Minimum)||
           !ShowroomNearPoint(actual.Maximum,mesh.Bounds.Maximum))return false;

        const auto target=ShowroomScale(
            ShowroomAdd(actual.Minimum,actual.Maximum),0.5);
        const double yaw=std::remainder(q.YawDegrees,360.0)*ShowroomCameraDegreesToRadians;
        const double pitch=q.PitchDegrees*ShowroomCameraDegreesToRadians;
        const double cy=std::cos(yaw),sy=std::sin(yaw);
        const double cp=std::cos(pitch),sp=std::sin(pitch);
        const OriginalVehiclePoint back{cp*cy,cp*sy,sp};
        const OriginalVehiclePoint right{-sy,cy,0.0};
        const OriginalVehiclePoint up{-sp*cy,-sp*sy,cp};
        const auto forward=ShowroomScale(back,-1.0);
        const double tanY=std::tan(q.VerticalFovDegrees*ShowroomCameraDegreesToRadians*0.5);
        const double tanX=tanY*q.AspectRatio;
        const double usable=1.0-q.Padding;
        if(!std::isfinite(tanX)||!std::isfinite(tanY)||tanX<=0.0||tanY<=0.0)
            return false;

        // Each point has camera depth d-dot(offset,back).
        // Solve both perspective constraints analytically for d:
        // |x| <= depth*tanX*usable, |y| <= depth*tanY*usable.
        // Include near clipping in the same maximum. No iterative zoom
        // guesses or bounding-sphere overframing for long/thin vehicles.
        double distance=q.NearClipCm+1.0;
        double backMin=std::numeric_limits<double>::infinity();
        for(const auto& vertex:mesh.Vertices)
        {
            const auto offset=MeshSubtract(vertex.Position,target);
            const double z=ShowroomDot(offset,back);
            const double x=ShowroomDot(offset,right);
            const double y=ShowroomDot(offset,up);
            distance=std::max({distance,z+q.NearClipCm+1.0,
                z+std::abs(x)/(tanX*usable),
                z+std::abs(y)/(tanY*usable)});
            backMin=std::min(backMin,z);
        }
        distance=(distance+1e-5)*q.ZoomOut;
        if(!std::isfinite(distance)||distance<=0.0||
           distance>ShowroomCameraMaxDistanceCm)return false;
        const double farClip=distance-backMin+std::max(10.0,distance*0.02);
        if(!std::isfinite(farClip)||farClip<=q.NearClipCm)return false;

        OriginalVehicleShowroomCamera candidate;
        candidate.pose_={ShowroomAdd(target,ShowroomScale(back,distance)),
            target,right,up,forward,distance,q.NearClipCm,farClip,
            q.VerticalFovDegrees,
            2.0*std::atan(tanX)/ShowroomCameraDegreesToRadians,
            q.AspectRatio,q.Padding};
        candidate.tanX_=tanX;
        candidate.tanY_=tanY;
        candidate.vehicleId_=mesh.VehicleId;
        candidate.ready_=true;
        auto& bounds=candidate.bounds_;
        bounds.MinX=bounds.MinY=bounds.MinDepthCm=
            std::numeric_limits<double>::infinity();
        bounds.MaxX=bounds.MaxY=bounds.MaxDepthCm=
            -std::numeric_limits<double>::infinity();
        for(const auto& vertex:mesh.Vertices)
        {
            const auto p=candidate.Project(vertex.Position);
            if(!p.Valid||!p.InsidePaddedViewport)return false;
            bounds.MinX=std::min(bounds.MinX,p.NdcX);
            bounds.MaxX=std::max(bounds.MaxX,p.NdcX);
            bounds.MinY=std::min(bounds.MinY,p.NdcY);
            bounds.MaxY=std::max(bounds.MaxY,p.NdcY);
            bounds.MinDepthCm=std::min(bounds.MinDepthCm,p.DepthCm);
            bounds.MaxDepthCm=std::max(bounds.MaxDepthCm,p.DepthCm);
        }
        *this=candidate;
        return true;
    }

    ShowroomProjectedPoint Project(OriginalVehiclePoint point) const noexcept
    {
        if(!ready_||!MeshFinitePoint(point)||
           std::max({std::abs(point.X),std::abs(point.Y),std::abs(point.Z)})>1e7)
            return {};
        const auto delta=MeshSubtract(point,pose_.Eye);
        const double depth=ShowroomDot(delta,pose_.Forward);
        if(!std::isfinite(depth)||depth<pose_.NearClipCm||depth>pose_.FarClipCm)
            return {};
        const double x=ShowroomDot(delta,pose_.Right)/(depth*tanX_);
        const double y=ShowroomDot(delta,pose_.Up)/(depth*tanY_);
        if(!std::isfinite(x)||!std::isfinite(y))return {};
        const double maxAbs=std::max(std::abs(x),std::abs(y));
        return {true,x,y,depth,maxAbs<=1.0+1e-9,
            maxAbs<=1.0-pose_.Padding+1e-9};
    }

private:
    bool ready_=false;
    int vehicleId_=-1;
    double tanX_=0.0,tanY_=0.0;
    ShowroomCameraPose pose_{};
    ShowroomScreenBounds bounds_{};
};
} // namespace latebrake::vehiclemesh
