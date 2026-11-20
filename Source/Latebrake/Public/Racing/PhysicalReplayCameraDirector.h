#pragma once

#include "Racing/PhysicalSixCarReplayTimeline.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Portable, read-only proposed replay camera framing. No Unreal camera
// component, camera collision sweep, trackside mesh or spectator pawn is
// created here; a trusted future native adapter must qualify occlusion.
inline constexpr double PhysicalReplayCameraPi=
    3.1415926535897932384626433832795;

enum class PhysicalReplayShot : std::uint8_t
{
    Chase,
    Bonnet,
    RightSide,
    LeftSide,
    HighAerial,
    RearFacing,
    Orbit
};

struct PhysicalReplayCameraSettings final
{
    double ChaseDistanceCm=720.0;
    double ChaseHeightCm=280.0;
    double AerialDistanceCm=1700.0;
    double AerialHeightCm=1950.0;
    double SideCameraOffsetCm=700.0;
    double SideCameraHeightCm=310.0;
    double OrbitRadiusCm=850.0;
    double OrbitHeightCm=390.0;
    double TargetLeadSeconds=0.16;
    double MaximumTargetLeadCm=900.0;
    double MinimumCameraFovDegrees=50.0;
    double MaximumCameraFovDegrees=95.0;
    double BaseCameraFovDegrees=66.0;
    double SpeedFovGainDegrees=22.0;
    double MaximumCameraFrameSeconds=0.10;
    double PositionSmoothingRate=6.5;
    double RotationSmoothingRate=8.0;
    double FovSmoothingRate=4.5;
    bool CameraIsVisualOnly=true;
    bool NeverObscureOfficialHud=true;
};

struct PhysicalReplayCameraInput final
{
    const PhysicalReplayPlaybackFrame* Playback=nullptr;
    std::uint64_t FocusActor=0;
    double ActiveClockSeconds=0.0;
    double FrameDeltaSeconds=0.0;
    PhysicalReplayShot Shot=PhysicalReplayShot::Chase;
    bool EngineReplayAuthorized=false;
    bool UserCameraEnabled=false;
};

struct PhysicalReplayCameraPose final
{
    bool Valid=false;
    std::uint64_t FocusActor=0;
    PhysicalReplayShot Shot=PhysicalReplayShot::Chase;
    double CameraXcm=0.0;
    double CameraYcm=0.0;
    double CameraZcm=0.0;
    double LookXcm=0.0;
    double LookYcm=0.0;
    double LookZcm=0.0;
    double YawRadians=0.0;
    double PitchRadians=0.0;
    double RollRadians=0.0;
    double HorizontalFovDegrees=0.0;
    double CameraTargetDistanceCm=0.0;
    bool HasValidReplayActor=false;
    bool OcclusionVerified=false;
    bool SceneMutationIssued=false;
    bool NativeCameraInstalled=false;
};

inline bool ValidPhysicalReplayCameraSettings(
    PhysicalReplayCameraSettings s) noexcept
{
    const auto finite=[](double x) noexcept {
        return std::isfinite(x);
    };
    return finite(s.ChaseDistanceCm) &&
        s.ChaseDistanceCm>=350.0 &&
        s.ChaseDistanceCm<=1600.0 &&
        finite(s.ChaseHeightCm) &&
        s.ChaseHeightCm>=120.0 &&
        s.ChaseHeightCm<=850.0 &&
        finite(s.AerialDistanceCm) &&
        s.AerialDistanceCm>=500.0 &&
        s.AerialDistanceCm<=5000.0 &&
        finite(s.AerialHeightCm) &&
        s.AerialHeightCm>=900.0 &&
        s.AerialHeightCm<=4500.0 &&
        finite(s.SideCameraOffsetCm) &&
        s.SideCameraOffsetCm>=450.0 &&
        s.SideCameraOffsetCm<=1800.0 &&
        finite(s.SideCameraHeightCm) &&
        s.SideCameraHeightCm>=100.0 &&
        s.SideCameraHeightCm<=800.0 &&
        finite(s.OrbitRadiusCm) &&
        s.OrbitRadiusCm>=450.0 &&
        s.OrbitRadiusCm<=1800.0 &&
        finite(s.OrbitHeightCm) &&
        s.OrbitHeightCm>=150.0 &&
        s.OrbitHeightCm<=1000.0 &&
        finite(s.TargetLeadSeconds) &&
        s.TargetLeadSeconds>=0.0 &&
        s.TargetLeadSeconds<=0.50 &&
        finite(s.MaximumTargetLeadCm) &&
        s.MaximumTargetLeadCm>=0.0 &&
        s.MaximumTargetLeadCm<=1500.0 &&
        finite(s.MinimumCameraFovDegrees) &&
        s.MinimumCameraFovDegrees>=35.0 &&
        s.MinimumCameraFovDegrees<=85.0 &&
        finite(s.MaximumCameraFovDegrees) &&
        s.MaximumCameraFovDegrees>=s.MinimumCameraFovDegrees+5.0 &&
        s.MaximumCameraFovDegrees<=115.0 &&
        finite(s.BaseCameraFovDegrees) &&
        s.BaseCameraFovDegrees>=s.MinimumCameraFovDegrees &&
        s.BaseCameraFovDegrees<=s.MaximumCameraFovDegrees &&
        finite(s.SpeedFovGainDegrees) &&
        s.SpeedFovGainDegrees>=0.0 &&
        s.SpeedFovGainDegrees<=40.0 &&
        finite(s.MaximumCameraFrameSeconds) &&
        s.MaximumCameraFrameSeconds>0.0 &&
        s.MaximumCameraFrameSeconds<=0.10 &&
        finite(s.PositionSmoothingRate) &&
        s.PositionSmoothingRate>0.0 &&
        s.PositionSmoothingRate<=30.0 &&
        finite(s.RotationSmoothingRate) &&
        s.RotationSmoothingRate>0.0 &&
        s.RotationSmoothingRate<=30.0 &&
        finite(s.FovSmoothingRate) &&
        s.FovSmoothingRate>0.0 &&
        s.FovSmoothingRate<=30.0 &&
        s.CameraIsVisualOnly &&
        s.NeverObscureOfficialHud;
}

inline bool ValidReplayShot(PhysicalReplayShot shot) noexcept
{
    switch(shot)
    {
    case PhysicalReplayShot::Chase:
    case PhysicalReplayShot::Bonnet:
    case PhysicalReplayShot::RightSide:
    case PhysicalReplayShot::LeftSide:
    case PhysicalReplayShot::HighAerial:
    case PhysicalReplayShot::RearFacing:
    case PhysicalReplayShot::Orbit:
        return true;
    }
    return false;
}

inline const PhysicalReplayPlaybackCar* FindReplayCameraFocus(
    const PhysicalReplayPlaybackFrame& frame,
    std::uint64_t actor) noexcept
{
    if(!frame.Valid || actor==0 ||
       !frame.NoCollisionAuthority ||
       !frame.NoOfficialTimingAuthority ||
       !frame.SourceOnly)
        return nullptr;
    for(const auto& car:frame.Cars)
    {
        if(car.Actor==actor && car.ProvenanceMatched)
            return &car;
    }
    return nullptr;
}

inline PhysicalReplayCameraPose PlanPhysicalReplayCamera(
    const PhysicalReplayCameraInput& in,
    PhysicalReplayCameraSettings settings={}) noexcept
{
    PhysicalReplayCameraPose out;
    if(!ValidPhysicalReplayCameraSettings(settings) ||
       !ValidReplayShot(in.Shot) ||
       !in.EngineReplayAuthorized ||
       !in.UserCameraEnabled ||
       in.Playback==nullptr ||
       !std::isfinite(in.ActiveClockSeconds) ||
       in.ActiveClockSeconds<=0.0 ||
       !std::isfinite(in.FrameDeltaSeconds) ||
       in.FrameDeltaSeconds<=0.0 ||
       in.FrameDeltaSeconds>settings.MaximumCameraFrameSeconds ||
       !std::isfinite(in.Playback->ActiveClockSeconds) ||
       std::abs(in.ActiveClockSeconds-
                in.Playback->ActiveClockSeconds)>1e-4)
        return out;
    const auto* focus=FindReplayCameraFocus(
        *in.Playback,in.FocusActor);
    if(focus==nullptr ||
       !std::isfinite(focus->Xcm) ||
       !std::isfinite(focus->Ycm) ||
       !std::isfinite(focus->Zcm) ||
       !std::isfinite(focus->HeadingRadians) ||
       !std::isfinite(focus->ObservedSpeedCmps) ||
       focus->ObservedSpeedCmps<0.0 ||
       focus->ObservedSpeedCmps>PhysicalReplayMaximumSpeedCmps)
        return out;
    const double x=focus->Xcm;
    const double y=focus->Ycm;
    const double z=focus->Zcm;
    const double heading=focus->HeadingRadians;
    const double fx=std::cos(heading);
    const double fy=std::sin(heading);
    const double rx=-fy;
    const double ry=fx;
    const double speed=focus->ObservedSpeedCmps;
    const double lead=std::min(
        settings.MaximumTargetLeadCm,
        speed*settings.TargetLeadSeconds);
    out.CameraXcm=x-settings.ChaseDistanceCm*fx;
    out.CameraYcm=y-settings.ChaseDistanceCm*fy;
    out.CameraZcm=z+settings.ChaseHeightCm;
    out.LookXcm=x+lead*fx;
    out.LookYcm=y+lead*fy;
    out.LookZcm=z+110.0;
    switch(in.Shot)
    {
    case PhysicalReplayShot::Chase:
        break;
    case PhysicalReplayShot::Bonnet:
        out.CameraXcm=x+135.0*fx;
        out.CameraYcm=y+135.0*fy;
        out.CameraZcm=z+96.0;
        out.LookXcm=x+1300.0*fx;
        out.LookYcm=y+1300.0*fy;
        out.LookZcm=z+55.0;
        break;
    case PhysicalReplayShot::RightSide:
    case PhysicalReplayShot::LeftSide:
    {
        const double side=in.Shot==PhysicalReplayShot::RightSide ?
            -1.0:1.0;
        out.CameraXcm=x+side*settings.SideCameraOffsetCm*rx;
        out.CameraYcm=y+side*settings.SideCameraOffsetCm*ry;
        out.CameraZcm=z+settings.SideCameraHeightCm;
        out.LookXcm=x+0.25*lead*fx;
        out.LookYcm=y+0.25*lead*fy;
        out.LookZcm=z+110.0;
        break;
    }
    case PhysicalReplayShot::HighAerial:
        out.CameraXcm=x-settings.AerialDistanceCm*fx;
        out.CameraYcm=y-settings.AerialDistanceCm*fy;
        out.CameraZcm=z+settings.AerialHeightCm;
        break;
    case PhysicalReplayShot::RearFacing:
        out.CameraXcm=x+550.0*fx;
        out.CameraYcm=y+550.0*fy;
        out.CameraZcm=z+250.0;
        out.LookXcm=x-900.0*fx;
        out.LookYcm=y-900.0*fy;
        out.LookZcm=z+105.0;
        break;
    case PhysicalReplayShot::Orbit:
    {
        const double azimuth=in.ActiveClockSeconds*0.28;
        const double clockwise=std::cos(azimuth);
        const double tangent=std::sin(azimuth);
        out.CameraXcm=x+settings.OrbitRadiusCm*
            (clockwise*rx+tangent*fx);
        out.CameraYcm=y+settings.OrbitRadiusCm*
            (clockwise*ry+tangent*fy);
        out.CameraZcm=z+settings.OrbitHeightCm;
        out.LookXcm=x+0.20*lead*fx;
        out.LookYcm=y+0.20*lead*fy;
        break;
    }
    default:
        return {};
    }
    const double dx=out.LookXcm-out.CameraXcm;
    const double dy=out.LookYcm-out.CameraYcm;
    const double dz=out.LookZcm-out.CameraZcm;
    const double horizontal=std::hypot(dx,dy);
    const double distance=std::hypot(horizontal,dz);
    if(!std::isfinite(distance) ||
       distance<80.0 || distance>10000.0 ||
       !std::isfinite(out.CameraXcm) ||
       !std::isfinite(out.CameraYcm) ||
       !std::isfinite(out.CameraZcm) ||
       !std::isfinite(out.LookXcm) ||
       !std::isfinite(out.LookYcm) ||
       !std::isfinite(out.LookZcm))
        return {};
    out.YawRadians=std::atan2(dy,dx);
    out.PitchRadians=std::atan2(dz,horizontal);
    out.RollRadians=0.0;
    out.HorizontalFovDegrees=std::clamp(
        settings.BaseCameraFovDegrees+
            settings.SpeedFovGainDegrees*speed/
                PhysicalReplayMaximumSpeedCmps,
        settings.MinimumCameraFovDegrees,
        settings.MaximumCameraFovDegrees);
    out.CameraTargetDistanceCm=distance;
    out.FocusActor=in.FocusActor;
    out.Shot=in.Shot;
    out.HasValidReplayActor=true;
    out.Valid=std::isfinite(out.YawRadians) &&
        std::isfinite(out.PitchRadians) &&
        std::isfinite(out.HorizontalFovDegrees);
    return out;
}

class PhysicalReplayCameraRig final
{
public:
    void Reset() noexcept
    {
        HavePrevious=false;
        Last={};
    }
    bool HasPrevious() const noexcept
    {
        return HavePrevious;
    }
    PhysicalReplayCameraPose Update(
        const PhysicalReplayCameraInput& in,
        PhysicalReplayCameraSettings settings={}) noexcept
    {
        const auto desired=PlanPhysicalReplayCamera(in,settings);
        if(!desired.Valid)
        {
            Reset();
            return {};
        }
        // Cuts between actor identities or camera types are explicit.
        // Interpolating a cinematic "teleport" through the track could
        // place an untrusted camera inside road collision geometry.
        if(!HavePrevious ||
           Last.FocusActor!=desired.FocusActor ||
           Last.Shot!=desired.Shot)
        {
            Last=desired;
            HavePrevious=true;
            return Last;
        }
        const double dt=in.FrameDeltaSeconds;
        const double positionAlpha=1.0-
            std::exp(-settings.PositionSmoothingRate*dt);
        const double rotationAlpha=1.0-
            std::exp(-settings.RotationSmoothingRate*dt);
        const double lensAlpha=1.0-
            std::exp(-settings.FovSmoothingRate*dt);
        if(!std::isfinite(positionAlpha) ||
           !std::isfinite(rotationAlpha) ||
           !std::isfinite(lensAlpha))
        {
            Reset();
            return {};
        }
        auto blend=[positionAlpha](double from,double to) noexcept {
            return from+positionAlpha*(to-from);
        };
        Last.CameraXcm=blend(Last.CameraXcm,desired.CameraXcm);
        Last.CameraYcm=blend(Last.CameraYcm,desired.CameraYcm);
        Last.CameraZcm=blend(Last.CameraZcm,desired.CameraZcm);
        Last.LookXcm=blend(Last.LookXcm,desired.LookXcm);
        Last.LookYcm=blend(Last.LookYcm,desired.LookYcm);
        Last.LookZcm=blend(Last.LookZcm,desired.LookZcm);
        const double deltaYaw=std::remainder(
            desired.YawRadians-Last.YawRadians,
            2.0*PhysicalReplayCameraPi);
        Last.YawRadians=std::remainder(
            Last.YawRadians+rotationAlpha*deltaYaw,
            2.0*PhysicalReplayCameraPi);
        Last.PitchRadians+=rotationAlpha*
            (desired.PitchRadians-Last.PitchRadians);
        Last.HorizontalFovDegrees+=lensAlpha*
            (desired.HorizontalFovDegrees-
             Last.HorizontalFovDegrees);
        Last.CameraTargetDistanceCm=std::hypot(
            std::hypot(Last.LookXcm-Last.CameraXcm,
                       Last.LookYcm-Last.CameraYcm),
            Last.LookZcm-Last.CameraZcm);
        if(!std::isfinite(Last.CameraTargetDistanceCm) ||
           Last.CameraTargetDistanceCm<80.0 ||
           Last.CameraTargetDistanceCm>10000.0)
        {
            Reset();
            return {};
        }
        return Last;
    }

private:
    bool HavePrevious=false;
    PhysicalReplayCameraPose Last{};
};
} // namespace latebrake
