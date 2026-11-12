#pragma once
#include "Racecraft/PhysicalSixCarReplayArchive.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
// Source-only physical camera planning. The recorded/live chassis decides
// where a camera MAY be placed; Unreal alone owns actual camera components.
inline constexpr int PhysicalCameraObstacleCapacity=24;
enum class PhysicalCameraShot : std::uint8_t
{
    ChaseNear, ChaseFar, Hood, Rear, Overhead, Trackside
};
enum class PhysicalCameraFault : std::uint8_t
{
    None, MissingSource, StaleSource, ChangedRoster,
    MissingChassis, MissingCameraMarker, BadObstacle,
    CameraOccluded, UnsafeGroundHeight, LostRaceAuthority
};
struct PhysicalCameraSettings final
{
    double NearChaseDistanceCm=620.0;
    double FarChaseDistanceCm=1150.0;
    double ChaseEyeHeightCm=220.0;
    double FarChaseEyeHeightCm=360.0;
    double HoodForwardOffsetCm=170.0;
    double HoodHeightCm=85.0;
    double RearDistanceCm=370.0;
    double RearEyeHeightCm=170.0;
    double OverheadHeightCm=1200.0;
    double OverheadBackCm=100.0;
    double LookAheadDistanceCm=600.0;
    double LookAheadSpeedSeconds=0.35;
    double MinimumGroundClearanceCm=55.0;
    double MinimumChaseHeightCm=80.0;
    double CollisionPaddingCm=35.0;
    double MinimumCameraToCarCm=135.0;
    double MaximumCameraToCarCm=2500.0;
    double MinimumCameraToMarkerCm=50.0;
    double MaximumMarkerDistanceCm=22000.0;
    double MinimumFovDegrees=43.0;
    double MaximumFovDegrees=91.0;
    double ReferenceFovDegrees=65.0;
    double SpeedFovGainDegPerCmS=0.0010;
    double MaximumPhysicalSpeedCmS=latebrake::racecraft::MaximumPhysicalSpeedCmS;
    double PositionSmoothingSeconds=0.13;
    double AimSmoothingSeconds=0.09;
    double FovSmoothingSeconds=0.30;
    double MaximumFrameSeconds=MaximumFrameTimeSeconds;
    double MaximumSensorClockSkewSeconds=0.02;
    double MinimumCameraConfidence=0.8;
    bool RequireOriginalTrackObstacles=true;
    bool RequireSameOriginalSixCars=true;
    bool KeepRealVehicleControlsUntouched=true;
    bool KeepOfficialRaceUntouched=true;
};
struct PhysicalCameraChassis final
{
    RaceIdentity Identity{};
    std::uint64_t ChaosBodyComponentToken=0;
    Vec3 PositionCm{};
    Vec3 VelocityCmS{};
    double RoadHeightCm=0.0;
    double YawRadians=0.0;
    double PitchRadians=0.0;
    double RollRadians=0.0;
    double SensorClockSeconds=0.0;
    double Confidence=1.0;
    bool ActualEngineTransformObserved=false;
    bool OriginalCollisionBody=false;
    bool OriginalTrackSurfaceObserved=false;
};
struct PhysicalCameraObstacle final
{
    Vec3 BoundsMinimumCm{};
    Vec3 BoundsMaximumCm{};
    std::uint64_t OriginalCollisionComponentToken=0;
    bool ActualEngineObstacleObserved=false;
    bool BlocksSpectatorCamera=true;
    bool OriginalTrackGeometry=false;
};
struct PhysicalCameraTracksideMarker final
{
    Vec3 PositionCm{};
    Vec3 LookAtCm{};
    std::uint64_t OriginalCameraMarkerToken=0;
    bool OriginalTrackCameraMarker=false;
};
struct PhysicalCameraFrame final
{
    PhysicalTraceFrame Fleet{};
    std::array<PhysicalCameraChassis,PhysicalFleetCapacity> Chassis{};
    std::array<PhysicalCameraObstacle,PhysicalCameraObstacleCapacity> Obstacles{};
    PhysicalCameraTracksideMarker Trackside{};
    int ObstacleCount=0;
    bool OriginalCircuitCollisionReady=false;
    bool SpectatorCameraPermitted=false;
    bool GamePaused=false;
};
struct PhysicalCameraProposal final
{
    RaceIdentity Source{};
    Vec3 EyeCm{};
    Vec3 AimCm{};
    double FieldOfViewDegrees=65.0;
    double EyeGroundClearanceCm=0.0;
    double ObstructionDistanceCm=
        std::numeric_limits<double>::infinity();
    double MeasuredVehicleSpeedCmS=0.0;
    int FocusSlot=-1;
    int VerifiedRivalCount=0;
    int MeasuredObstacleCount=0;
    PhysicalCameraShot Shot=PhysicalCameraShot::ChaseNear;
    PhysicalCameraFault Fault=PhysicalCameraFault::MissingSource;
    bool UnrealCameraCommandIssued=false;
    bool VehicleControlsModified=false;
    bool OfficialRaceClockModified=false;
    bool Valid=false;
};
inline bool ValidPhysicalCameraSettings(
    PhysicalCameraSettings p) noexcept
{
    return Finite(p.NearChaseDistanceCm) &&
        p.NearChaseDistanceCm>p.MinimumCameraToCarCm &&
        Finite(p.FarChaseDistanceCm) &&
        p.FarChaseDistanceCm>p.NearChaseDistanceCm &&
        Finite(p.ChaseEyeHeightCm) &&
        p.ChaseEyeHeightCm>p.MinimumGroundClearanceCm &&
        Finite(p.FarChaseEyeHeightCm) &&
        p.FarChaseEyeHeightCm>p.ChaseEyeHeightCm &&
        Finite(p.HoodForwardOffsetCm) &&
        p.HoodForwardOffsetCm>0.0 &&
        Finite(p.HoodHeightCm) && p.HoodHeightCm>0.0 &&
        Finite(p.RearDistanceCm) &&
        p.RearDistanceCm>p.MinimumCameraToCarCm &&
        Finite(p.RearEyeHeightCm) &&
        p.RearEyeHeightCm>0.0 &&
        Finite(p.OverheadHeightCm) &&
        p.OverheadHeightCm>p.FarChaseEyeHeightCm &&
        Finite(p.OverheadBackCm) &&
        p.OverheadBackCm>=0.0 &&
        Finite(p.LookAheadDistanceCm) &&
        p.LookAheadDistanceCm>0.0 &&
        Finite(p.LookAheadSpeedSeconds) &&
        p.LookAheadSpeedSeconds>=0.0 &&
        Finite(p.MinimumGroundClearanceCm) &&
        p.MinimumGroundClearanceCm>0.0 &&
        Finite(p.MinimumChaseHeightCm) &&
        p.MinimumChaseHeightCm>=p.MinimumGroundClearanceCm &&
        Finite(p.CollisionPaddingCm) &&
        p.CollisionPaddingCm>=0.0 &&
        Finite(p.MinimumCameraToCarCm) &&
        p.MinimumCameraToCarCm>0.0 &&
        Finite(p.MaximumCameraToCarCm) &&
        p.MaximumCameraToCarCm>p.FarChaseDistanceCm &&
        Finite(p.MinimumCameraToMarkerCm) &&
        p.MinimumCameraToMarkerCm>0.0 &&
        Finite(p.MaximumMarkerDistanceCm) &&
        p.MaximumMarkerDistanceCm>p.MaximumCameraToCarCm &&
        Finite(p.MinimumFovDegrees) &&
        p.MinimumFovDegrees>10.0 &&
        Finite(p.MaximumFovDegrees) &&
        p.MaximumFovDegrees>p.MinimumFovDegrees &&
        p.MaximumFovDegrees<170.0 &&
        Finite(p.ReferenceFovDegrees) &&
        p.ReferenceFovDegrees>=p.MinimumFovDegrees &&
        p.ReferenceFovDegrees<=p.MaximumFovDegrees &&
        Finite(p.SpeedFovGainDegPerCmS) &&
        p.SpeedFovGainDegPerCmS>=0.0 &&
        Finite(p.MaximumPhysicalSpeedCmS) &&
        p.MaximumPhysicalSpeedCmS>0.0 &&
        Finite(p.PositionSmoothingSeconds) &&
        p.PositionSmoothingSeconds>=0.0 &&
        Finite(p.AimSmoothingSeconds) &&
        p.AimSmoothingSeconds>=0.0 &&
        Finite(p.FovSmoothingSeconds) &&
        p.FovSmoothingSeconds>=0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumSensorClockSkewSeconds) &&
        p.MaximumSensorClockSkewSeconds>=0.0 &&
        Finite(p.MinimumCameraConfidence) &&
        p.MinimumCameraConfidence>0.0 &&
        p.MinimumCameraConfidence<=1.0;
}
inline bool ValidCameraChassis(
    const PhysicalCameraChassis& s,
    const PhysicalTraceActor& physical,
    const PhysicalTraceFrame& frame,
    PhysicalCameraSettings settings) noexcept
{
    return s.Identity.Session==frame.Identity.Session &&
        s.Identity.SessionRevision==
            frame.Identity.SessionRevision &&
        s.Identity.Frame==frame.Identity.Frame &&
        s.Identity.Actor==physical.Identity.Actor &&
        s.Identity.Slot==physical.Identity.Slot &&
        s.Identity.EngineObserved &&
        s.Identity.BodySimulating &&
        s.Identity.OriginalTrack &&
        s.ChaosBodyComponentToken!=0 &&
        s.ActualEngineTransformObserved &&
        s.OriginalCollisionBody &&
        s.OriginalTrackSurfaceObserved &&
        Finite(s.PositionCm) &&
        Finite(s.VelocityCmS) &&
        Finite(s.RoadHeightCm) &&
        Finite(s.YawRadians) &&
        Finite(s.PitchRadians) &&
        Finite(s.RollRadians) &&
        Finite(s.SensorClockSeconds) &&
        std::abs(s.SensorClockSeconds-
            frame.PhysicsClockSeconds)<=
            settings.MaximumSensorClockSkewSeconds &&
        s.Confidence>=settings.MinimumCameraConfidence &&
        s.Confidence<=1.0 &&
        std::abs(s.PositionCm.X-
            physical.Pose.Position.X)<=1.0 &&
        std::abs(s.PositionCm.Y-
            physical.Pose.Position.Y)<=1.0 &&
        std::abs(WrappedAngle(s.YawRadians-
            physical.Pose.HeadingRadians))<=0.02 &&
        std::abs(s.VelocityCmS.X-
            physical.VelocityCmS.X)<=1.0 &&
        std::abs(s.VelocityCmS.Y-
            physical.VelocityCmS.Y)<=1.0 &&
        Length(s.VelocityCmS)<=settings.MaximumPhysicalSpeedCmS &&
        s.PositionCm.Z>=s.RoadHeightCm-35.0;
}
inline bool ValidCameraObstacle(
    const PhysicalCameraObstacle& obstacle,
    PhysicalCameraSettings settings) noexcept
{
    return Finite(obstacle.BoundsMinimumCm) &&
        Finite(obstacle.BoundsMaximumCm) &&
        obstacle.BoundsMinimumCm.X<obstacle.BoundsMaximumCm.X &&
        obstacle.BoundsMinimumCm.Y<obstacle.BoundsMaximumCm.Y &&
        obstacle.BoundsMinimumCm.Z<obstacle.BoundsMaximumCm.Z &&
        (!settings.RequireOriginalTrackObstacles ||
         (obstacle.OriginalCollisionComponentToken!=0 &&
          obstacle.ActualEngineObstacleObserved &&
          obstacle.OriginalTrackGeometry));
}
inline bool ValidCameraFrame(
    const PhysicalCameraFrame& frame,
    PhysicalCameraSettings settings) noexcept
{
    if(!ValidPhysicalCameraSettings(settings) ||
       !frame.Fleet.Valid ||
       !frame.SpectatorCameraPermitted ||
       !frame.OriginalCircuitCollisionReady ||
       frame.ObstacleCount<0 ||
       frame.ObstacleCount>PhysicalCameraObstacleCapacity)
        return false;
    for(int i=0;i<PhysicalFleetCapacity;++i)
    {
        const auto idx=static_cast<std::size_t>(i);
        const auto& sensor=frame.Chassis[idx];
        if(!ValidCameraChassis(sensor,frame.Fleet.Actors[idx],
            frame.Fleet,settings))
            return false;
        for(int prior=0;prior<i;++prior)
        {
            if(sensor.ChaosBodyComponentToken==
               frame.Chassis[static_cast<std::size_t>(prior)]
                   .ChaosBodyComponentToken)
                return false;
        }
    }
    for(int i=0;i<frame.ObstacleCount;++i)
    {
        const auto& obstacle=frame.Obstacles[static_cast<std::size_t>(i)];
        if(!ValidCameraObstacle(obstacle,settings))
            return false;
        for(int prior=0;prior<i;++prior)
        {
            if(obstacle.OriginalCollisionComponentToken==
               frame.Obstacles[static_cast<std::size_t>(prior)]
                   .OriginalCollisionComponentToken)
                return false;
        }
    }
    return true;
}
inline double CameraRayAabbHit(
    Vec3 start,Vec3 end,
    const PhysicalCameraObstacle& obstacle,
    double paddingCm) noexcept
{
    if(!Finite(start) || !Finite(end) ||
       !Finite(paddingCm) || paddingCm<0.0)
        return std::numeric_limits<double>::infinity();
    const Vec3 movement=end-start;
    double lower=0.0;
    double upper=1.0;
    const std::array<double,3> origin{start.X,start.Y,start.Z};
    const std::array<double,3> delta{movement.X,movement.Y,movement.Z};
    const std::array<double,3> min{
        obstacle.BoundsMinimumCm.X-paddingCm,
        obstacle.BoundsMinimumCm.Y-paddingCm,
        obstacle.BoundsMinimumCm.Z-paddingCm};
    const std::array<double,3> max{
        obstacle.BoundsMaximumCm.X+paddingCm,
        obstacle.BoundsMaximumCm.Y+paddingCm,
        obstacle.BoundsMaximumCm.Z+paddingCm};
    for(std::size_t i=0;i<3;++i)
    {
        if(std::abs(delta[i])<=PhysicsEpsilon)
        {
            if(origin[i]<min[i] || origin[i]>max[i])
                return std::numeric_limits<double>::infinity();
            continue;
        }
        const double one=1.0/delta[i];
        const double a=(min[i]-origin[i])*one;
        const double b=(max[i]-origin[i])*one;
        lower=std::max(lower,std::min(a,b));
        upper=std::min(upper,std::max(a,b));
        if(lower>upper)
            return std::numeric_limits<double>::infinity();
    }
    return lower;
}
inline PhysicalCameraProposal PlanPhysicalCamera(
    const PhysicalCameraFrame& frame,
    int focusSlot,
    PhysicalCameraShot shot,
    PhysicalCameraSettings settings={}) noexcept
{
    PhysicalCameraProposal out;
    out.Shot=shot;
    out.FocusSlot=focusSlot;
    if(!ValidCameraFrame(frame,settings) ||
       focusSlot<0 || focusSlot>=PhysicalFleetCapacity)
        return out;
    out.Source=frame.Fleet.Identity;
    out.VerifiedRivalCount=RivalTrafficCount;
    out.MeasuredObstacleCount=frame.ObstacleCount;
    const auto& chassis=frame.Chassis[static_cast<std::size_t>(focusSlot)];
    const Vec3 position=chassis.PositionCm;
    const Vec2 forward2=HeadingVector(chassis.YawRadians);
    const Vec3 forward{forward2.X,forward2.Y,0.0};
    const double speed=Length(chassis.VelocityCmS);
    out.MeasuredVehicleSpeedCmS=speed;
    const Vec3 ahead=position+forward*settings.LookAheadDistanceCm+
        chassis.VelocityCmS*settings.LookAheadSpeedSeconds;
    out.AimCm=ahead;
    switch(shot)
    {
    case PhysicalCameraShot::ChaseNear:
        out.EyeCm=position-forward*settings.NearChaseDistanceCm+
            Vec3{0.0,0.0,settings.ChaseEyeHeightCm};
        break;
    case PhysicalCameraShot::ChaseFar:
        out.EyeCm=position-forward*settings.FarChaseDistanceCm+
            Vec3{0.0,0.0,settings.FarChaseEyeHeightCm};
        break;
    case PhysicalCameraShot::Hood:
        out.EyeCm=position+forward*settings.HoodForwardOffsetCm+
            Vec3{0.0,0.0,settings.HoodHeightCm};
        out.AimCm=out.EyeCm+forward*settings.LookAheadDistanceCm;
        break;
    case PhysicalCameraShot::Rear:
        out.EyeCm=position-forward*settings.RearDistanceCm+
            Vec3{0.0,0.0,settings.RearEyeHeightCm};
        out.AimCm=position-forward*settings.LookAheadDistanceCm;
        break;
    case PhysicalCameraShot::Overhead:
        out.EyeCm=position-forward*settings.OverheadBackCm+
            Vec3{0.0,0.0,settings.OverheadHeightCm};
        break;
    case PhysicalCameraShot::Trackside:
        if(!frame.Trackside.OriginalTrackCameraMarker ||
           frame.Trackside.OriginalCameraMarkerToken==0 ||
           !Finite(frame.Trackside.PositionCm) ||
           !Finite(frame.Trackside.LookAtCm) ||
           Length(frame.Trackside.PositionCm-position)>
                settings.MaximumMarkerDistanceCm)
        {
            out.Fault=PhysicalCameraFault::MissingCameraMarker;
            return out;
        }
        out.EyeCm=frame.Trackside.PositionCm;
        out.AimCm=position;
        break;
    }
    out.FieldOfViewDegrees=std::clamp(
        settings.ReferenceFovDegrees+
            speed*settings.SpeedFovGainDegPerCmS,
        settings.MinimumFovDegrees,settings.MaximumFovDegrees);
    const double floorZ=chassis.RoadHeightCm+
        settings.MinimumGroundClearanceCm;
    if(out.EyeCm.Z<floorZ)
    {
        if(shot==PhysicalCameraShot::Trackside)
        {
            out.Fault=PhysicalCameraFault::UnsafeGroundHeight;
            return out;
        }
        out.EyeCm.Z=floorZ;
    }
    out.EyeGroundClearanceCm=
        out.EyeCm.Z-chassis.RoadHeightCm;
    // Sweep from the physically observed chassis to the desired camera.
    // The live engine still resolves final camera-collision components.
    // This source policy cannot move or spawn an Unreal camera actor.
    const Vec3 toEye=out.EyeCm-position;
    const double targetDistance=Length(toEye);
    if(targetDistance<settings.MinimumCameraToMarkerCm)
    {
        out.Fault=PhysicalCameraFault::CameraOccluded;
        return out;
    }
    double hitDistance=std::numeric_limits<double>::infinity();
    for(int i=0;i<frame.ObstacleCount;++i)
    {
        const auto& wall=frame.Obstacles[
            static_cast<std::size_t>(i)];
        if(!wall.BlocksSpectatorCamera)continue;
        const double fraction=CameraRayAabbHit(
            position,out.EyeCm,wall,settings.CollisionPaddingCm);
        if(Finite(fraction))
            hitDistance=std::min(hitDistance,
                fraction*targetDistance);
    }
    out.ObstructionDistanceCm=hitDistance;
    if(Finite(hitDistance))
    {
        if(hitDistance<settings.MinimumCameraToCarCm &&
           shot!=PhysicalCameraShot::Hood)
        {
            out.Fault=PhysicalCameraFault::CameraOccluded;
            return out;
        }
        const double permittedDistance=std::max(0.0,
            hitDistance-settings.CollisionPaddingCm);
        out.EyeCm=position+toEye*(permittedDistance/targetDistance);
    }
    if(!Finite(out.EyeCm) || !Finite(out.AimCm) ||
       !Finite(out.FieldOfViewDegrees))
        return out;
    if(out.EyeCm.Z<floorZ)
    {
        out.Fault=PhysicalCameraFault::UnsafeGroundHeight;
        return out;
    }
    out.Fault=PhysicalCameraFault::None;
    out.Valid=true;
    return out;
}
class PhysicalSixCarCameraRig final
{
public:
    bool Configure(PhysicalCameraSettings settings) noexcept
    {
        if(!ValidPhysicalCameraSettings(settings))return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Source={};
        SourceRoster.fill(0);
        HasPrior=false;
        QualifiedFrames=0;
        RejectedFrames=0;
        ShotChanges=0;
        PreviousFocus=-1;
        PreviousShot=PhysicalCameraShot::ChaseNear;
        PreviousEye={};
        PreviousAim={};
        PreviousFov=0.0;
    }
    PhysicalCameraProposal Step(
        const PhysicalCameraFrame& frame,
        int focusSlot,
        PhysicalCameraShot shot) noexcept
    {
        auto proposed=PlanPhysicalCamera(frame,focusSlot,shot,Settings);
        if(!proposed.Valid)
        {
            ++RejectedFrames;
            Last={};
            HasPrior=false;
            return {};
        }
        const auto& now=frame.Fleet.Identity;
        const bool sameEpoch=HasPrior &&
            Source.Session==now.Session &&
            Source.SessionRevision==now.SessionRevision &&
            Source.Actor==now.Actor;
        if(sameEpoch && now.Frame<=Source.Frame)
        {
            ++RejectedFrames;
            Last={};
            HasPrior=false;
            return {};
        }
        if(sameEpoch && frame.Fleet.ActorRoster!=SourceRoster &&
           Settings.RequireSameOriginalSixCars)
        {
            ++RejectedFrames;
            Last={};
            HasPrior=false;
            return {};
        }
        const double dt=frame.Fleet.DeltaSeconds;
        const bool smooth=sameEpoch && HasPrior &&
            now.Frame==Source.Frame+1 &&
            focusSlot==PreviousFocus &&
            shot==PreviousShot &&
            !frame.GamePaused &&
            dt>0.0 && dt<=Settings.MaximumFrameSeconds;
        if(smooth)
        {
            proposed.EyeCm={
                ExponentialResponse(PreviousEye.X,proposed.EyeCm.X,
                    Settings.PositionSmoothingSeconds,dt),
                ExponentialResponse(PreviousEye.Y,proposed.EyeCm.Y,
                    Settings.PositionSmoothingSeconds,dt),
                ExponentialResponse(PreviousEye.Z,proposed.EyeCm.Z,
                    Settings.PositionSmoothingSeconds,dt)};
            proposed.AimCm={
                ExponentialResponse(PreviousAim.X,proposed.AimCm.X,
                    Settings.AimSmoothingSeconds,dt),
                ExponentialResponse(PreviousAim.Y,proposed.AimCm.Y,
                    Settings.AimSmoothingSeconds,dt),
                ExponentialResponse(PreviousAim.Z,proposed.AimCm.Z,
                    Settings.AimSmoothingSeconds,dt)};
            proposed.FieldOfViewDegrees=ExponentialResponse(
                PreviousFov,proposed.FieldOfViewDegrees,
                Settings.FovSmoothingSeconds,dt);
            // Temporal filtering cannot reintroduce an observed wall
            // obstruction: it is a requested, not an engine-actuated pose.
            if(!Finite(proposed.EyeCm) || !Finite(proposed.AimCm))
                return {};
        }
        if(QualifiedFrames>0 && shot!=PreviousShot)
            ++ShotChanges;
        PreviousEye=proposed.EyeCm;
        PreviousAim=proposed.AimCm;
        PreviousFov=proposed.FieldOfViewDegrees;
        PreviousFocus=focusSlot;
        PreviousShot=shot;
        Source=now;
        SourceRoster=frame.Fleet.ActorRoster;
        HasPrior=true;
        ++QualifiedFrames;
        Last=proposed;
        return Last;
    }
    PhysicalCameraProposal Snapshot() const noexcept{return Last;}
    std::uint64_t Accepted() const noexcept{return QualifiedFrames;}
    std::uint64_t Rejected() const noexcept{return RejectedFrames;}
    std::uint64_t Changes() const noexcept{return ShotChanges;}
    bool HasCamera() const noexcept{return Last.Valid;}
private:
    PhysicalCameraSettings Settings{};
    PhysicalCameraProposal Last{};
    RaceIdentity Source{};
    std::array<std::uint64_t,PhysicalFleetCapacity> SourceRoster{};
    Vec3 PreviousEye{};
    Vec3 PreviousAim{};
    double PreviousFov=0.0;
    int PreviousFocus=-1;
    PhysicalCameraShot PreviousShot=PhysicalCameraShot::ChaseNear;
    std::uint64_t QualifiedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t ShotChanges=0;
    bool HasPrior=false;
};
} // namespace latebrake::racecraft