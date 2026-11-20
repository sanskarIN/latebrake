#pragma once

#include "Track/WorldScaleTracksideScenePlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Pure C++20 rendering ADVISORY for the original six planned road layouts.
// It consumes future source-camera observations and authored trackside props.
// It cannot alter the Unreal viewport, render graph, gameplay actors,
// collision, race clock, camera occlusion, or authoritative race results.
inline constexpr std::size_t TracksideDrawMaximumObjects=
    TracksideSceneObjectCount;
inline constexpr double TracksideDrawPi=
    3.1415926535897932384626433832795;

enum class TracksideDetailLevel : std::uint8_t
{
    Omitted,
    Landmark,
    Reduced,
    Full
};

struct TracksideDrawSettings final
{
    double NearDistanceCm=8000.0;
    double MiddleDistanceCm=25000.0;
    double AbsoluteMaximumDistanceCm=65000.0;
    double MaximumViewClockSeconds=86400.0;
    double MinimumHorizontalFovDegrees=30.0;
    double MaximumHorizontalFovDegrees=130.0;
    double MinimumVerticalFovDegrees=20.0;
    double MaximumVerticalFovDegrees=95.0;
    double MinimumCameraHeightCm=-10000.0;
    double MaximumCameraHeightCm=10000.0;
    double MaximumCameraPlanarMagnitudeCm=1000000.0;
    std::size_t MaximumVisibleObjects=40u;
    std::size_t MaximumEstimatedTriangles=440u;
    std::size_t MaximumFullObjects=12u;
    std::size_t MaximumReducedObjects=22u;
    bool NoNativeRendererMutation=true;
    bool RequireSourceCameraObservation=true;
    bool NeverDeclareOcclusionVisible=true;
    bool NeverSpawnUnrealMesh=true;
};

struct TracksideCameraWitness final
{
    std::uint64_t SourceSession=0u;
    std::uint64_t NativeCameraToken=0u;
    std::uint64_t SourceFrame=0u;
    double ClockSeconds=0.0;
    double Xcm=0.0;
    double Ycm=0.0;
    double Zcm=0.0;
    double YawRadians=0.0;
    double PitchRadians=0.0;
    double HorizontalFovDegrees=80.0;
    double VerticalFovDegrees=50.0;
    bool ActualCameraTransformObserved=false;
    bool OriginalRaceCameraAuthorityUnchanged=true;
};

struct TracksideDrawObject final
{
    std::uint64_t DeterministicId=0u;
    TracksideSceneKind Kind=TracksideSceneKind::Floodlight;
    TracksideDetailLevel Detail=TracksideDetailLevel::Omitted;
    double DistanceToBoundsCm=0.0;
    double AngularOffsetRadians=0.0;
    double ClockSeconds=0.0;
    std::size_t EstimatedTriangles=0u;
    unsigned MaterialIndex=0u;
    bool SourceFootprintVerified=false;
    bool NoRealOcclusionTest=true;
    bool RenderSubmissionOccurred=false;
};

struct TracksideDrawFrame final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceFrame=0u;
    double ClockSeconds=0.0;
    std::array<TracksideDrawObject,TracksideDrawMaximumObjects> Objects{};
    std::size_t TotalCandidateObjects=0u;
    std::size_t FacingViewObjects=0u;
    std::size_t VisibleObjectCount=0u;
    std::size_t EstimatedTriangles=0u;
    std::size_t FullDetailCount=0u;
    std::size_t ReducedDetailCount=0u;
    std::size_t LandmarkCount=0u;
    std::size_t RejectedByDistance=0u;
    std::size_t RejectedByFrustum=0u;
    std::size_t RejectedByBudget=0u;
    bool NoEngineRendererMutated=true;
    bool RealOcclusionUnverified=true;
    bool OriginalRaceUntouched=true;
};

struct TracksideDrawCandidate final
{
    std::size_t SourceIndex=0u;
    std::uint64_t DeterministicId=0u;
    double DistanceToBoundsCm=0.0;
    double AngularOffsetRadians=0.0;
    TracksideDetailLevel RequestedDetail=TracksideDetailLevel::Omitted;
    unsigned KindPriority=0u;
};

inline bool ValidTracksideDrawSettings(
    TracksideDrawSettings p) noexcept
{
    const auto finite=[](double x) noexcept {
        return std::isfinite(x);
    };
    return finite(p.NearDistanceCm) &&
        p.NearDistanceCm>=1000.0 &&
        p.NearDistanceCm<=20000.0 &&
        finite(p.MiddleDistanceCm) &&
        p.MiddleDistanceCm>p.NearDistanceCm+1000.0 &&
        p.MiddleDistanceCm<=40000.0 &&
        finite(p.AbsoluteMaximumDistanceCm) &&
        p.AbsoluteMaximumDistanceCm>p.MiddleDistanceCm &&
        p.AbsoluteMaximumDistanceCm<=100000.0 &&
        finite(p.MaximumViewClockSeconds) &&
        p.MaximumViewClockSeconds>=60.0 &&
        p.MaximumViewClockSeconds<=86400.0 &&
        finite(p.MinimumHorizontalFovDegrees) &&
        p.MinimumHorizontalFovDegrees>=20.0 &&
        p.MinimumHorizontalFovDegrees<=70.0 &&
        finite(p.MaximumHorizontalFovDegrees) &&
        p.MaximumHorizontalFovDegrees>
            p.MinimumHorizontalFovDegrees &&
        p.MaximumHorizontalFovDegrees<=150.0 &&
        finite(p.MinimumVerticalFovDegrees) &&
        p.MinimumVerticalFovDegrees>=10.0 &&
        p.MinimumVerticalFovDegrees<=60.0 &&
        finite(p.MaximumVerticalFovDegrees) &&
        p.MaximumVerticalFovDegrees>
            p.MinimumVerticalFovDegrees &&
        p.MaximumVerticalFovDegrees<=110.0 &&
        finite(p.MinimumCameraHeightCm) &&
        finite(p.MaximumCameraHeightCm) &&
        p.MinimumCameraHeightCm>=-20000.0 &&
        p.MaximumCameraHeightCm<=20000.0 &&
        p.MaximumCameraHeightCm>p.MinimumCameraHeightCm &&
        finite(p.MaximumCameraPlanarMagnitudeCm) &&
        p.MaximumCameraPlanarMagnitudeCm>50000.0 &&
        p.MaximumCameraPlanarMagnitudeCm<=1000000.0 &&
        p.MaximumVisibleObjects>=1u &&
        p.MaximumVisibleObjects<=TracksideDrawMaximumObjects &&
        p.MaximumEstimatedTriangles>=12u &&
        p.MaximumEstimatedTriangles<=10000u &&
        p.MaximumFullObjects<=p.MaximumVisibleObjects &&
        p.MaximumReducedObjects<=p.MaximumVisibleObjects &&
        p.NoNativeRendererMutation &&
        p.RequireSourceCameraObservation &&
        p.NeverDeclareOcclusionVisible &&
        p.NeverSpawnUnrealMesh;
}

inline bool ValidTracksideCameraWitness(
    TracksideCameraWitness camera,
    TracksideDrawSettings settings) noexcept
{
    if(!ValidTracksideDrawSettings(settings) ||
       camera.SourceSession==0u ||
       camera.NativeCameraToken==0u ||
       camera.SourceFrame==0u ||
       !camera.ActualCameraTransformObserved ||
       !camera.OriginalRaceCameraAuthorityUnchanged)
        return false;
    const auto finite=[](double x) noexcept {
        return std::isfinite(x);
    };
    return finite(camera.ClockSeconds) &&
        camera.ClockSeconds>0.0 &&
        camera.ClockSeconds<=settings.MaximumViewClockSeconds &&
        finite(camera.Xcm) && finite(camera.Ycm) &&
        finite(camera.Zcm) &&
        std::abs(camera.Xcm)<=settings.MaximumCameraPlanarMagnitudeCm &&
        std::abs(camera.Ycm)<=settings.MaximumCameraPlanarMagnitudeCm &&
        camera.Zcm>=settings.MinimumCameraHeightCm &&
        camera.Zcm<=settings.MaximumCameraHeightCm &&
        finite(camera.YawRadians) &&
        std::abs(camera.YawRadians)<=1000.0 &&
        finite(camera.PitchRadians) &&
        std::abs(camera.PitchRadians)<=TracksideDrawPi*0.5 &&
        finite(camera.HorizontalFovDegrees) &&
        camera.HorizontalFovDegrees>=
            settings.MinimumHorizontalFovDegrees &&
        camera.HorizontalFovDegrees<=
            settings.MaximumHorizontalFovDegrees &&
        finite(camera.VerticalFovDegrees) &&
        camera.VerticalFovDegrees>=
            settings.MinimumVerticalFovDegrees &&
        camera.VerticalFovDegrees<=
            settings.MaximumVerticalFovDegrees;
}

inline unsigned TracksideDrawPriority(
    TracksideSceneKind kind) noexcept
{
    switch(kind)
    {
    case TracksideSceneKind::RescueAccess:return 7u;
    case TracksideSceneKind::MarshalPost:return 6u;
    case TracksideSceneKind::WayfindingSign:return 5u;
    case TracksideSceneKind::Floodlight:return 4u;
    case TracksideSceneKind::CameraTower:return 3u;
    case TracksideSceneKind::Grandstand:return 2u;
    case TracksideSceneKind::WoodlandCanopy:return 1u;
    }
    return 0u;
}

inline std::size_t TracksideEstimatedTriangles(
    TracksideSceneKind kind,
    TracksideDetailLevel level) noexcept
{
    if(level==TracksideDetailLevel::Omitted)return 0u;
    if(level==TracksideDetailLevel::Landmark)return 2u;
    if(level==TracksideDetailLevel::Reduced)return 12u;
    switch(kind)
    {
    case TracksideSceneKind::Grandstand:return 36u;
    case TracksideSceneKind::RescueAccess:
    case TracksideSceneKind::Floodlight:
    case TracksideSceneKind::MarshalPost:
    case TracksideSceneKind::CameraTower:
    case TracksideSceneKind::WayfindingSign:
    case TracksideSceneKind::WoodlandCanopy:return 24u;
    }
    return 0u;
}

inline bool ValidTracksideSceneObjectForDraw(
    const TracksideSceneObject& object,
    WorldScaleVariantId circuit) noexcept
{
    return object.Valid &&
        object.Circuit==circuit &&
        object.DeterministicId!=0u &&
        object.EveryBaseCornerOutsideRoad &&
        !object.UnrealActorSpawned &&
        !object.DrivableCollisionAdded &&
        std::isfinite(object.Xcm) &&
        std::isfinite(object.Ycm) &&
        std::isfinite(object.Zcm) &&
        std::isfinite(object.HalfWidthCm) &&
        std::isfinite(object.HalfLengthCm) &&
        std::isfinite(object.HeightCm) &&
        std::isfinite(object.MaximumDrawDistanceCm) &&
        object.HalfWidthCm>0.0 &&
        object.HalfLengthCm>0.0 &&
        object.HeightCm>0.0 &&
        object.MaximumDrawDistanceCm>=0.0 &&
        object.MaterialIndex>=1u &&
        object.MaterialIndex<=7u &&
        TracksideDrawPriority(object.Kind)>0u;
}

// Broad-phase sphere bounds are intentionally generous: unlike point
// culling, a tall floodlight or wide grandstand can remain partially in
// view while its center is outside the camera frustum.
inline bool TracksideFrustumSphere(
    TracksideCameraWitness camera,
    const TracksideSceneObject& object,
    double& distanceToBoundsCm,
    double& angularOffsetRadians) noexcept
{
    distanceToBoundsCm=0.0;
    angularOffsetRadians=0.0;
    const double cx=object.Xcm-camera.Xcm;
    const double cy=object.Ycm-camera.Ycm;
    const double cz=object.Zcm+object.HeightCm*0.5-camera.Zcm;
    const double radius=std::hypot(
        std::hypot(object.HalfWidthCm,object.HalfLengthCm),
        object.HeightCm*0.5);
    const double length=std::hypot(std::hypot(cx,cy),cz);
    if(!std::isfinite(radius) || !std::isfinite(length))
        return false;
    distanceToBoundsCm=std::max(0.0,length-radius);
    const double cp=std::cos(camera.PitchRadians);
    const double sp=std::sin(camera.PitchRadians);
    const double cyaw=std::cos(camera.YawRadians);
    const double syaw=std::sin(camera.YawRadians);
    const double forwardX=cp*cyaw;
    const double forwardY=cp*syaw;
    const double forwardZ=sp;
    const double rightX=-syaw;
    const double rightY=cyaw;
    const double upX=-sp*cyaw;
    const double upY=-sp*syaw;
    const double upZ=cp;
    const double front=cx*forwardX+
        cy*forwardY+cz*forwardZ;
    if(front < -radius)return false;
    const double horizontal=cx*rightX+cy*rightY;
    const double vertical=cx*upX+cy*upY+cz*upZ;
    const double tangentH=std::tan(
        camera.HorizontalFovDegrees*TracksideDrawPi/360.0);
    const double tangentV=std::tan(
        camera.VerticalFovDegrees*TracksideDrawPi/360.0);
    if(!std::isfinite(front) ||
       !std::isfinite(horizontal) ||
       !std::isfinite(vertical) ||
       !std::isfinite(tangentH) ||
       !std::isfinite(tangentV) ||
       std::abs(horizontal)>std::max(0.0,front)*tangentH+radius ||
       std::abs(vertical)>std::max(0.0,front)*tangentV+radius)
        return false;
    const double transverse=std::hypot(horizontal,vertical);
    angularOffsetRadians=std::atan2(
        transverse,std::max(0.0,front));
    return std::isfinite(angularOffsetRadians);
}

inline TracksideDetailLevel ChooseTracksideDetail(
    double distanceCm,TracksideDrawSettings p) noexcept
{
    if(!std::isfinite(distanceCm) || distanceCm<0.0 ||
       distanceCm>p.AbsoluteMaximumDistanceCm)
        return TracksideDetailLevel::Omitted;
    if(distanceCm<=p.NearDistanceCm)
        return TracksideDetailLevel::Full;
    if(distanceCm<=p.MiddleDistanceCm)
        return TracksideDetailLevel::Reduced;
    return TracksideDetailLevel::Landmark;
}

// A strict draw BUDGET is applied after culling, not before. Candidate
// order is deterministic for a given source view, with the nearest
// visibility-bound objects first and stable IDs breaking exact ties.
inline TracksideDrawFrame BuildTracksideDrawFrame(
    const TracksideScenePlan& scene,
    TracksideCameraWitness camera,
    TracksideDrawSettings settings={}) noexcept
{
    TracksideDrawFrame out;
    if(!scene.Valid || !scene.SourceOnly ||
       !scene.OriginalCollisionUnchanged ||
       scene.QualifiedObjects!=TracksideSceneObjectCount ||
       !ValidTracksideCameraWitness(camera,settings))
        return out;
    out.Circuit=scene.Circuit;
    out.SourceSession=camera.SourceSession;
    out.SourceFrame=camera.SourceFrame;
    out.ClockSeconds=camera.ClockSeconds;
    std::array<TracksideDrawCandidate,TracksideDrawMaximumObjects>
        candidates{};
    std::size_t count=0u;
    for(std::size_t i=0;i<scene.Objects.size();++i)
    {
        const auto& object=scene.Objects[i];
        if(!ValidTracksideSceneObjectForDraw(object,scene.Circuit))
            return {};
        for(std::size_t prev=0;prev<i;++prev)
            if(object.DeterministicId==
                   scene.Objects[prev].DeterministicId)
                return {};
        ++out.TotalCandidateObjects;
        double distance=0.0;
        double angle=0.0;
        if(!TracksideFrustumSphere(camera,object,distance,angle))
        {
            ++out.RejectedByFrustum;
            continue;
        }
        ++out.FacingViewObjects;
        if(distance>object.MaximumDrawDistanceCm ||
           distance>settings.AbsoluteMaximumDistanceCm)
        {
            ++out.RejectedByDistance;
            continue;
        }
        const auto detail=ChooseTracksideDetail(distance,settings);
        if(detail==TracksideDetailLevel::Omitted)
        {
            ++out.RejectedByDistance;
            continue;
        }
        candidates[count++]={i,object.DeterministicId,
            distance,angle,detail,TracksideDrawPriority(object.Kind)};
    }
    std::sort(candidates.begin(),candidates.begin()+count,
        [](const TracksideDrawCandidate& a,
           const TracksideDrawCandidate& b) noexcept {
            if(a.DistanceToBoundsCm!=b.DistanceToBoundsCm)
                return a.DistanceToBoundsCm<b.DistanceToBoundsCm;
            if(a.KindPriority!=b.KindPriority)
                return a.KindPriority>b.KindPriority;
            return a.DeterministicId<b.DeterministicId;
        });
    for(std::size_t i=0;i<count;++i)
    {
        const auto& candidate=candidates[i];
        const auto& object=scene.Objects[candidate.SourceIndex];
        if(out.VisibleObjectCount>=settings.MaximumVisibleObjects)
        {
            ++out.RejectedByBudget;
            continue;
        }
        auto detail=candidate.RequestedDetail;
        // Degrade expensive close-up geometry before dropping a
        // verified visible object. This is an explicit predictable
        // triangle-budget response, not real GPU timing or occlusion.
        if(detail==TracksideDetailLevel::Full &&
           out.FullDetailCount>=settings.MaximumFullObjects)
            detail=TracksideDetailLevel::Reduced;
        if(detail==TracksideDetailLevel::Reduced &&
           out.ReducedDetailCount>=settings.MaximumReducedObjects)
            detail=TracksideDetailLevel::Landmark;
        auto cost=TracksideEstimatedTriangles(object.Kind,detail);
        while(detail!=TracksideDetailLevel::Omitted &&
              out.EstimatedTriangles+cost>
                  settings.MaximumEstimatedTriangles)
        {
            if(detail==TracksideDetailLevel::Full)
                detail=TracksideDetailLevel::Reduced;
            else if(detail==TracksideDetailLevel::Reduced)
                detail=TracksideDetailLevel::Landmark;
            else
                detail=TracksideDetailLevel::Omitted;
            cost=TracksideEstimatedTriangles(object.Kind,detail);
        }
        if(detail==TracksideDetailLevel::Omitted)
        {
            ++out.RejectedByBudget;
            continue;
        }
        TracksideDrawObject entry;
        entry.DeterministicId=candidate.DeterministicId;
        entry.Kind=object.Kind;
        entry.Detail=detail;
        entry.DistanceToBoundsCm=candidate.DistanceToBoundsCm;
        entry.AngularOffsetRadians=candidate.AngularOffsetRadians;
        entry.ClockSeconds=camera.ClockSeconds;
        entry.EstimatedTriangles=cost;
        entry.MaterialIndex=object.MaterialIndex;
        entry.SourceFootprintVerified=true;
        out.Objects[out.VisibleObjectCount++]=entry;
        out.EstimatedTriangles+=cost;
        if(detail==TracksideDetailLevel::Full)
            ++out.FullDetailCount;
        if(detail==TracksideDetailLevel::Reduced)
            ++out.ReducedDetailCount;
        if(detail==TracksideDetailLevel::Landmark)
            ++out.LandmarkCount;
    }
    out.Valid=out.TotalCandidateObjects==TracksideSceneObjectCount &&
        out.VisibleObjectCount<=settings.MaximumVisibleObjects &&
        out.EstimatedTriangles<=settings.MaximumEstimatedTriangles &&
        out.VisibleObjectCount==
            out.FullDetailCount+out.ReducedDetailCount+
                out.LandmarkCount;
    return out;
}
} // namespace latebrake
