#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int ForwardVisibilityRayCount=12;
enum class VisibilityObstacleKind : std::uint8_t
{
    None, OriginalBarrier, PhysicalVehicle, TrackEdge,
    PitEntryWall, StandingWater, Debris, UnknownCollision
};
enum class VisibilitySafetyClass : std::uint8_t
{
    Unqualified, Clear, ReducedSight, LowContrast,
    BarrierAhead, TrafficAhead, InsufficientStoppingSight,
    UnknownRoadSurface, SensorFault
};
struct VisibilitySafetyParameters final
{
    double MaximumRayDistanceCm=11000.0;
    double MinimumRayDistanceCm=50.0;
    double MaximumRayClockSkewSeconds=0.04;
    double MaximumFrameSeconds=0.10;
    double MaximumSpeedCmS=MaximumPhysicalSpeedCmS;
    double MaximumQualifiedDecelerationCmS2=1500.0;
    double MinimumQualifiedDecelerationCmS2=200.0;
    double DriverReactionSeconds=0.28;
    double RoadSurfaceSafetyMarginCm=250.0;
    double BarrierSafetyMarginCm=350.0;
    double TrafficSafetyMarginCm=500.0;
    double WeatherVisibilityMarginCm=420.0;
    double MaximumRoadVisibilityFraction=1.0;
    double MinimumRoadVisibilityFraction=0.08;
    double MaximumRainMmHour=250.0;
    double MaximumFogDensityFraction=1.0;
    double MaximumLightLossFraction=1.0;
    double MinimumLuminanceFraction=0.05;
    double MaximumGlareFraction=1.0;
    double MaximumPhysicalDustFraction=1.0;
    double MinimumRoadGripG=0.15;
    double MaximumRoadGripG=2.5;
    double MinimumSensorConfidence=0.80;
    double MaximumRayNormalError=0.25;
    double MaximumRoadCrossTrackCm=900.0;
    double MinimumRoadHalfWidthCm=450.0;
    double MaximumHeadingErrorRadians=1.1;
    double MaximumYawRateRadiansSecond=7.0;
    double MaximumVehicleHalfWidthCm=250.0;
    double MaximumCarForwardHalfLengthCm=500.0;
    double MaximumCameraEyeHeightCm=250.0;
    double MinimumCameraEyeHeightCm=30.0;
    double MaximumSensorFieldOfViewRadians=2.5;
    double MinimumSensorFieldOfViewRadians=0.2;
    double MaximumOcclusionPenalty=0.8;
    double MaximumHazardSeverity=1.0;
    double SpeedReleasePerSecondCmS=1000.0;
    double MaximumThrottleRecoveryPerSecond=0.75;
    double MaximumBrakeReleasePerSecond=2.0;
    double MaximumRequiredBrakeFraction=1.0;
    double EmergencyTimeToHazardSeconds=0.75;
    double MinimumUsefulForwardRays=4.0;
    bool RequireOriginalCollisionWorld=true;
    bool RequireActualChaosRaycasts=true;
    bool RejectStaleVisibility=true;
    bool NeverAuthorizeAccelerationInFog=true;
    bool NeverModifyUnrealScene=true;
};
struct PhysicalVisibilityRay final
{
    RaceIdentity Identity{};
    std::uint64_t OriginalCollisionComponent=0;
    std::uint64_t OtherActorToken=0;
    Vec3 RayOriginWorldCm{};
    Vec3 RayDirectionWorld{};
    Vec3 HitPointWorldCm{};
    Vec3 SurfaceNormal{};
    double HitDistanceCm=0.0;
    double MaximumTraceDistanceCm=0.0;
    double SourceClockSeconds=0.0;
    double SensorConfidence=1.0;
    VisibilityObstacleKind Kind=VisibilityObstacleKind::None;
    bool RealTracePerformed=false;
    bool OriginalWorldComponent=true;
    bool HitRecorded=false;
    bool LineOfSightQualified=false;
    bool ChaosSceneQuerySucceeded=false;
};
struct PhysicalVisibilityFrame final
{
    RaceIdentity Identity{};
    std::array<PhysicalVisibilityRay,ForwardVisibilityRayCount> Rays{};
    int RayCount=0;
    Vec3 ChassisWorldPositionCm{};
    Vec3 DriverEyeWorldPositionCm{};
    Vec2 VelocityCmS{};
    double ChassisHeadingRadians=0.0;
    double ChassisYawRateRadiansSecond=0.0;
    double CurrentRoadGripG=1.0;
    double MeasuredRoadHalfWidthCm=800.0;
    double ActualCrossTrackCm=0.0;
    double RainRateMmHour=0.0;
    double FogDensityFraction=0.0;
    double ActualLightLossFraction=0.0;
    double MeasuredLuminanceFraction=1.0;
    double MeasuredGlareFraction=0.0;
    double PhysicalDustFraction=0.0;
    double PhysicsClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double CurrentThrottle=0.0;
    double CurrentBrake=0.0;
    double CurrentSteering=0.0;
    bool OriginalMapCollisionReady=false;
    bool RealVisibilitySensorsReady=false;
    bool ChaosVehicleBodyQualified=false;
    bool RaceGreen=false;
};
struct VisibilityRayReview final
{
    int RayOrdinal=-1;
    VisibilityObstacleKind Kind=VisibilityObstacleKind::None;
    double MeasuredClearDistanceCm=0.0;
    double ActualHitDistanceCm=0.0;
    double MeasuredConfidence=0.0;
    double HeadingDifferenceRadians=0.0;
    double LateralHitSeparationCm=0.0;
    double EstimatedReactionDistanceCm=0.0;
    double EstimatedBrakingDistanceCm=0.0;
    double RemainingSightReserveCm=0.0;
    double PhysicalOcclusionFraction=0.0;
    double ObstacleSeverity=0.0;
    bool OriginalWorldHit=false;
    bool PhysicalVehicleHit=false;
    bool ActualCollisionQueryReady=false;
    bool HazardWithinStoppingEnvelope=false;
    bool Valid=false;
};
struct VisibilitySafetyDecision final
{
    RaceIdentity Identity{};
    std::array<VisibilityRayReview,ForwardVisibilityRayCount> Rays{};
    VisibilitySafetyClass Class=VisibilitySafetyClass::Unqualified;
    double PhysicalForwardVisibilityCm=0.0;
    double WeatherVisibilityCm=0.0;
    double RequiredStoppingSightCm=0.0;
    double AvailableStoppingReserveCm=0.0;
    double MaximumSafeSpeedCmS=0.0;
    double MeasuredCurrentSpeedCmS=0.0;
    double CurrentGripG=0.0;
    double MaximumNearFieldOcclusion=0.0;
    double MinimumRayConfidence=0.0;
    double MeanConfidence=0.0;
    double MinimumForwardObstacleDistanceCm=0.0;
    double NearestOriginalBarrierCm=0.0;
    double NearestPhysicalVehicleCm=0.0;
    double MaximumBrakeDemand=0.0;
    double ThrottleCeiling=0.0;
    double BrakeFloor=1.0;
    double ProposedThrottle=0.0;
    double ProposedBrake=1.0;
    int VerifiedPhysicalRays=0;
    int BarriersSeen=0;
    int RealVehiclesSeen=0;
    int UnknownSurfaceWarnings=0;
    int RaycastFailures=0;
    bool NoInventedClearRoad=true;
    bool EmergencyStopRequired=false;
    bool SourceOnly=true;
    bool Valid=false;
};
inline bool ValidVisibilitySafetyParameters(
    VisibilitySafetyParameters p) noexcept
{
    return Finite(p.MaximumRayDistanceCm) &&
        p.MaximumRayDistanceCm>0.0 &&
        Finite(p.MinimumRayDistanceCm) &&
        p.MinimumRayDistanceCm>0.0 &&
        p.MaximumRayDistanceCm>p.MinimumRayDistanceCm &&
        Finite(p.MaximumRayClockSkewSeconds) &&
        p.MaximumRayClockSkewSeconds>=0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumSpeedCmS) &&
        p.MaximumSpeedCmS>0.0 &&
        p.MaximumSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MaximumQualifiedDecelerationCmS2) &&
        p.MaximumQualifiedDecelerationCmS2>0.0 &&
        Finite(p.MinimumQualifiedDecelerationCmS2) &&
        p.MinimumQualifiedDecelerationCmS2>0.0 &&
        p.MaximumQualifiedDecelerationCmS2>=
            p.MinimumQualifiedDecelerationCmS2 &&
        Finite(p.DriverReactionSeconds) &&
        p.DriverReactionSeconds>=0.0 &&
        Finite(p.RoadSurfaceSafetyMarginCm) &&
        p.RoadSurfaceSafetyMarginCm>=0.0 &&
        Finite(p.BarrierSafetyMarginCm) &&
        p.BarrierSafetyMarginCm>=0.0 &&
        Finite(p.TrafficSafetyMarginCm) &&
        p.TrafficSafetyMarginCm>=0.0 &&
        Finite(p.WeatherVisibilityMarginCm) &&
        p.WeatherVisibilityMarginCm>=0.0 &&
        Finite(p.MaximumRoadVisibilityFraction) &&
        p.MaximumRoadVisibilityFraction>0.0 &&
        Finite(p.MinimumRoadVisibilityFraction) &&
        p.MinimumRoadVisibilityFraction>0.0 &&
        Finite(p.MaximumRainMmHour) &&
        p.MaximumRainMmHour>0.0 &&
        Finite(p.MaximumFogDensityFraction) &&
        p.MaximumFogDensityFraction>0.0 &&
        Finite(p.MaximumLightLossFraction) &&
        p.MaximumLightLossFraction>0.0 &&
        Finite(p.MinimumLuminanceFraction) &&
        p.MinimumLuminanceFraction>0.0 &&
        Finite(p.MaximumGlareFraction) &&
        p.MaximumGlareFraction>0.0 &&
        Finite(p.MaximumPhysicalDustFraction) &&
        p.MaximumPhysicalDustFraction>0.0 &&
        Finite(p.MinimumRoadGripG) &&
        p.MinimumRoadGripG>0.0 &&
        Finite(p.MaximumRoadGripG) &&
        p.MaximumRoadGripG>p.MinimumRoadGripG &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.MaximumRayNormalError) &&
        p.MaximumRayNormalError>=0.0 &&
        Finite(p.MaximumRoadCrossTrackCm) &&
        p.MaximumRoadCrossTrackCm>0.0 &&
        Finite(p.MinimumRoadHalfWidthCm) &&
        p.MinimumRoadHalfWidthCm>0.0 &&
        Finite(p.MaximumHeadingErrorRadians) &&
        p.MaximumHeadingErrorRadians>0.0 &&
        Finite(p.MaximumYawRateRadiansSecond) &&
        p.MaximumYawRateRadiansSecond>0.0 &&
        Finite(p.MaximumVehicleHalfWidthCm) &&
        p.MaximumVehicleHalfWidthCm>0.0 &&
        Finite(p.MaximumCarForwardHalfLengthCm) &&
        p.MaximumCarForwardHalfLengthCm>0.0 &&
        Finite(p.MaximumCameraEyeHeightCm) &&
        p.MaximumCameraEyeHeightCm>0.0 &&
        Finite(p.MinimumCameraEyeHeightCm) &&
        p.MinimumCameraEyeHeightCm>0.0 &&
        p.MaximumCameraEyeHeightCm>p.MinimumCameraEyeHeightCm &&
        Finite(p.MaximumSensorFieldOfViewRadians) &&
        p.MaximumSensorFieldOfViewRadians>0.0 &&
        Finite(p.MinimumSensorFieldOfViewRadians) &&
        p.MinimumSensorFieldOfViewRadians>0.0 &&
        p.MaximumSensorFieldOfViewRadians>
            p.MinimumSensorFieldOfViewRadians &&
        Finite(p.MaximumOcclusionPenalty) &&
        p.MaximumOcclusionPenalty>=0.0 &&
        Finite(p.MaximumHazardSeverity) &&
        p.MaximumHazardSeverity>0.0 &&
        Finite(p.SpeedReleasePerSecondCmS) &&
        p.SpeedReleasePerSecondCmS>0.0 &&
        Finite(p.MaximumThrottleRecoveryPerSecond) &&
        p.MaximumThrottleRecoveryPerSecond>0.0 &&
        Finite(p.MaximumBrakeReleasePerSecond) &&
        p.MaximumBrakeReleasePerSecond>0.0 &&
        Finite(p.MaximumRequiredBrakeFraction) &&
        p.MaximumRequiredBrakeFraction>0.0 &&
        p.MaximumRequiredBrakeFraction<=1.0 &&
        Finite(p.EmergencyTimeToHazardSeconds) &&
        p.EmergencyTimeToHazardSeconds>0.0 &&
        Finite(p.MinimumUsefulForwardRays) &&
        p.MinimumUsefulForwardRays>=1.0 &&
        p.MinimumUsefulForwardRays<=ForwardVisibilityRayCount;
}
inline bool ValidVisibilityRay(
    PhysicalVisibilityRay s,
    VisibilitySafetyParameters p) noexcept
{
    return PhysicalActorIdentityValid(s.Identity) &&
        s.Identity.Frame>0 &&
        (!p.RequireActualChaosRaycasts ||
         (s.RealTracePerformed &&
          s.ChaosSceneQuerySucceeded &&
          s.LineOfSightQualified)) &&
        (!p.RequireOriginalCollisionWorld ||
         s.OriginalWorldComponent) &&
        Finite(s.RayOriginWorldCm) &&
        Finite(s.RayDirectionWorld) &&
        std::abs(Length(s.RayDirectionWorld)-1.0)<0.02 &&
        Finite(s.HitPointWorldCm) &&
        Finite(s.SurfaceNormal) &&
        Finite(s.HitDistanceCm) &&
        s.HitDistanceCm>=0.0 &&
        s.HitDistanceCm<=p.MaximumRayDistanceCm &&
        Finite(s.MaximumTraceDistanceCm) &&
        s.MaximumTraceDistanceCm>=p.MinimumRayDistanceCm &&
        s.MaximumTraceDistanceCm<=p.MaximumRayDistanceCm &&
        s.HitDistanceCm<=s.MaximumTraceDistanceCm &&
        Finite(s.SourceClockSeconds) &&
        s.SourceClockSeconds>=0.0 &&
        Finite(s.SensorConfidence) &&
        s.SensorConfidence>=p.MinimumSensorConfidence &&
        s.SensorConfidence<=1.0 &&
        (!s.HitRecorded ||
         (s.OriginalCollisionComponent!=0 &&
          s.Kind!=VisibilityObstacleKind::None));
}
inline bool ValidPhysicalVisibilityFrame(
    const PhysicalVisibilityFrame& f,
    VisibilitySafetyParameters p) noexcept
{
    if (!ValidVisibilitySafetyParameters(p) ||
        !PhysicalActorIdentityValid(f.Identity) ||
        f.Identity.Frame==0 ||
        !f.OriginalMapCollisionReady ||
        !f.RealVisibilitySensorsReady ||
        !f.ChaosVehicleBodyQualified ||
        !f.RaceGreen ||
        f.RayCount<static_cast<int>(p.MinimumUsefulForwardRays) ||
        f.RayCount>ForwardVisibilityRayCount ||
        !Finite(f.ChassisWorldPositionCm) ||
        !Finite(f.DriverEyeWorldPositionCm) ||
        !Finite(f.VelocityCmS) ||
        Length(f.VelocityCmS)>p.MaximumSpeedCmS ||
        !Finite(f.ChassisHeadingRadians) ||
        !Finite(f.ChassisYawRateRadiansSecond) ||
        std::abs(f.ChassisYawRateRadiansSecond)>
            p.MaximumYawRateRadiansSecond ||
        !Finite(f.CurrentRoadGripG) ||
        f.CurrentRoadGripG<p.MinimumRoadGripG ||
        f.CurrentRoadGripG>p.MaximumRoadGripG ||
        !Finite(f.MeasuredRoadHalfWidthCm) ||
        f.MeasuredRoadHalfWidthCm<p.MinimumRoadHalfWidthCm ||
        !Finite(f.ActualCrossTrackCm) ||
        std::abs(f.ActualCrossTrackCm)>p.MaximumRoadCrossTrackCm ||
        !Finite(f.RainRateMmHour) ||
        f.RainRateMmHour<0.0 ||
        f.RainRateMmHour>p.MaximumRainMmHour ||
        !Finite(f.FogDensityFraction) ||
        f.FogDensityFraction<0.0 ||
        f.FogDensityFraction>p.MaximumFogDensityFraction ||
        !Finite(f.ActualLightLossFraction) ||
        f.ActualLightLossFraction<0.0 ||
        f.ActualLightLossFraction>p.MaximumLightLossFraction ||
        !Finite(f.MeasuredLuminanceFraction) ||
        f.MeasuredLuminanceFraction<p.MinimumLuminanceFraction ||
        f.MeasuredLuminanceFraction>1.0 ||
        !Finite(f.MeasuredGlareFraction) ||
        f.MeasuredGlareFraction<0.0 ||
        f.MeasuredGlareFraction>p.MaximumGlareFraction ||
        !Finite(f.PhysicalDustFraction) ||
        f.PhysicalDustFraction<0.0 ||
        f.PhysicalDustFraction>p.MaximumPhysicalDustFraction ||
        !Finite(f.PhysicsClockSeconds) ||
        f.PhysicsClockSeconds<0.0 ||
        !Finite(f.DeltaSeconds) ||
        f.DeltaSeconds<=0.0 ||
        f.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(f.CurrentThrottle) ||
        f.CurrentThrottle<0.0 ||
        f.CurrentThrottle>1.0 ||
        !Finite(f.CurrentBrake) ||
        f.CurrentBrake<0.0 ||
        f.CurrentBrake>1.0 ||
        !Finite(f.CurrentSteering) ||
        std::abs(f.CurrentSteering)>1.0)
        return false;
    for(int i=0;i<f.RayCount;++i)
    {
        const auto& ray=f.Rays[static_cast<std::size_t>(i)];
        if(!ValidVisibilityRay(ray,p) ||
           ray.Identity.Session!=f.Identity.Session ||
           ray.Identity.SessionRevision!=
               f.Identity.SessionRevision ||
           ray.Identity.Actor!=f.Identity.Actor ||
           ray.Identity.Frame!=f.Identity.Frame ||
           std::abs(ray.SourceClockSeconds-
                    f.PhysicsClockSeconds)>
                p.MaximumRayClockSkewSeconds)
            return false;
    }
    return true;
}
inline VisibilityRayReview EvaluateForwardRay(
    PhysicalVisibilityRay ray,
    Vec3 chassisWorld,double currentSpeed,
    double deceleration,
    VisibilitySafetyParameters p) noexcept
{
    VisibilityRayReview out;
    if(!ValidVisibilityRay(ray,p) ||
       !Finite(chassisWorld) ||
       !Finite(currentSpeed) ||
       !Finite(deceleration) ||
       deceleration<=0.0)
        return out;
    out.Kind=ray.Kind;
    out.MeasuredClearDistanceCm=ray.HitRecorded?
        ray.HitDistanceCm:ray.MaximumTraceDistanceCm;
    out.ActualHitDistanceCm=ray.HitDistanceCm;
    out.MeasuredConfidence=ray.SensorConfidence;
    out.EstimatedReactionDistanceCm=
        currentSpeed*p.DriverReactionSeconds;
    out.EstimatedBrakingDistanceCm=SafeDivision(
        currentSpeed*currentSpeed,2.0*deceleration);
    out.RemainingSightReserveCm=
        out.MeasuredClearDistanceCm-
        out.EstimatedReactionDistanceCm-
        out.EstimatedBrakingDistanceCm;
    out.PhysicalOcclusionFraction=ray.HitRecorded?
        Clamp01(1.0-SafeDivision(
            ray.HitDistanceCm,ray.MaximumTraceDistanceCm)):0.0;
    out.ObstacleSeverity=ray.HitRecorded?
        Clamp01(out.PhysicalOcclusionFraction*
            p.MaximumOcclusionPenalty):0.0;
    out.LateralHitSeparationCm=std::hypot(
        ray.HitPointWorldCm.X-chassisWorld.X,
        ray.HitPointWorldCm.Y-chassisWorld.Y);
    out.HeadingDifferenceRadians=
        std::atan2(ray.RayDirectionWorld.Y,
                   ray.RayDirectionWorld.X);
    out.OriginalWorldHit=ray.HitRecorded &&
        ray.OriginalWorldComponent;
    out.PhysicalVehicleHit=ray.HitRecorded &&
        ray.Kind==VisibilityObstacleKind::PhysicalVehicle &&
        ray.OtherActorToken!=0;
    out.ActualCollisionQueryReady=
        ray.ChaosSceneQuerySucceeded &&
        ray.RealTracePerformed;
    out.HazardWithinStoppingEnvelope=
        ray.HitRecorded &&
        out.RemainingSightReserveCm<
            p.BarrierSafetyMarginCm;
    out.Valid=Finite(out.MeasuredClearDistanceCm) &&
        Finite(out.RemainingSightReserveCm) &&
        Finite(out.PhysicalOcclusionFraction);
    return out;
}
inline VisibilitySafetyDecision EvaluatePhysicalVisibility(
    const PhysicalVisibilityFrame& f,
    VisibilitySafetyParameters p={}) noexcept
{
    VisibilitySafetyDecision out;
    if (!ValidPhysicalVisibilityFrame(f,p))
        return out;
    out.Identity=f.Identity;
    out.MeasuredCurrentSpeedCmS=Length(f.VelocityCmS);
    out.CurrentGripG=f.CurrentRoadGripG;
    out.MinimumRayConfidence=1.0;
    out.MinimumForwardObstacleDistanceCm=
        p.MaximumRayDistanceCm;
    out.NearestOriginalBarrierCm=p.MaximumRayDistanceCm;
    out.NearestPhysicalVehicleCm=p.MaximumRayDistanceCm;
    const double visibility=Clamp01(
        1.0-
        0.45*f.FogDensityFraction-
        0.2*f.ActualLightLossFraction-
        0.15*f.PhysicalDustFraction-
        0.10*f.MeasuredGlareFraction-
        0.10*SafeDivision(f.RainRateMmHour,
                          p.MaximumRainMmHour));
    const double decel=std::clamp(
        f.CurrentRoadGripG*GravityCmS2,
        p.MinimumQualifiedDecelerationCmS2,
        p.MaximumQualifiedDecelerationCmS2);
    out.RequiredStoppingSightCm=
        out.MeasuredCurrentSpeedCmS*p.DriverReactionSeconds+
        SafeDivision(out.MeasuredCurrentSpeedCmS*
                     out.MeasuredCurrentSpeedCmS,2.0*decel)+
        p.RoadSurfaceSafetyMarginCm;
    for(int i=0;i<f.RayCount;++i)
    {
        const auto review=EvaluateForwardRay(
            f.Rays[static_cast<std::size_t>(i)],
            f.ChassisWorldPositionCm,
            out.MeasuredCurrentSpeedCmS,decel,p);
        if(!review.Valid)return {};
        out.Rays[static_cast<std::size_t>(i)]=review;
        ++out.VerifiedPhysicalRays;
        out.MinimumRayConfidence=std::min(
            out.MinimumRayConfidence,review.MeasuredConfidence);
        out.MeanConfidence+=review.MeasuredConfidence;
        out.MaximumNearFieldOcclusion=std::max(
            out.MaximumNearFieldOcclusion,
            review.PhysicalOcclusionFraction);
        if(f.Rays[static_cast<std::size_t>(i)].HitRecorded)
        {
            out.MinimumForwardObstacleDistanceCm=std::min(
                out.MinimumForwardObstacleDistanceCm,
                review.MeasuredClearDistanceCm);
            if(review.Kind==VisibilityObstacleKind::OriginalBarrier)
            {
                ++out.BarriersSeen;
                out.NearestOriginalBarrierCm=std::min(
                    out.NearestOriginalBarrierCm,
                    review.ActualHitDistanceCm);
            }
            if(review.PhysicalVehicleHit)
            {
                ++out.RealVehiclesSeen;
                out.NearestPhysicalVehicleCm=std::min(
                    out.NearestPhysicalVehicleCm,
                    review.ActualHitDistanceCm);
            }
            out.UnknownSurfaceWarnings+=
                review.Kind==VisibilityObstacleKind::UnknownCollision?
                1:0;
        }
    }
    out.MeanConfidence/=out.VerifiedPhysicalRays;
    out.PhysicalForwardVisibilityCm=
        out.MinimumForwardObstacleDistanceCm;
    out.WeatherVisibilityCm=
        p.MaximumRayDistanceCm*visibility;
    const double available=std::min(
        out.PhysicalForwardVisibilityCm,
        out.WeatherVisibilityCm);
    out.AvailableStoppingReserveCm=
        available-out.RequiredStoppingSightCm;
    const double safeDistance=std::max(
        0.0,available-p.RoadSurfaceSafetyMarginCm);
    // Solve reaction distance plus constant-deceleration distance
    // for maximum safe speed rather than linearly scaling current speed.
    const double reaction=p.DriverReactionSeconds;
    out.MaximumSafeSpeedCmS=std::min(
        p.MaximumSpeedCmS,
        std::max(0.0,
            std::sqrt(
                decel*decel*reaction*reaction+
                2.0*decel*safeDistance)-
            decel*reaction));
    out.EmergencyStopRequired=
        available<out.RequiredStoppingSightCm &&
        out.MeasuredCurrentSpeedCmS>out.MaximumSafeSpeedCmS;
    const double overspeed=std::max(
        0.0,out.MeasuredCurrentSpeedCmS-
            out.MaximumSafeSpeedCmS);
    out.MaximumBrakeDemand=Clamp01(
        SafeDivision(overspeed,
            std::max(200.0,out.MaximumSafeSpeedCmS))*
        p.MaximumRequiredBrakeFraction);
    out.BrakeFloor=out.EmergencyStopRequired ?
        std::max(0.15,out.MaximumBrakeDemand):0.0;
    out.ThrottleCeiling=out.EmergencyStopRequired?
        0.0:Clamp01(
            1.0-SafeDivision(overspeed,1000.0));
    out.ProposedThrottle=std::min(
        f.CurrentThrottle,out.ThrottleCeiling);
    out.ProposedBrake=std::max(
        f.CurrentBrake,out.BrakeFloor);
    if(out.ProposedBrake>0.05)
        out.ProposedThrottle=0.0;
    out.Class=out.UnknownSurfaceWarnings>0?
        VisibilitySafetyClass::UnknownRoadSurface:
        out.EmergencyStopRequired?
            VisibilitySafetyClass::InsufficientStoppingSight:
        out.BarriersSeen>0?
            VisibilitySafetyClass::BarrierAhead:
        out.RealVehiclesSeen>0?
            VisibilitySafetyClass::TrafficAhead:
        visibility<0.5?
            VisibilitySafetyClass::LowContrast:
        visibility<0.8?
            VisibilitySafetyClass::ReducedSight:
            VisibilitySafetyClass::Clear;
    out.NoInventedClearRoad=true;
    out.SourceOnly=true;
    out.Valid=out.VerifiedPhysicalRays==f.RayCount &&
        Finite(out.MaximumSafeSpeedCmS) &&
        Finite(out.RequiredStoppingSightCm) &&
        Finite(out.ProposedThrottle) &&
        Finite(out.ProposedBrake);
    return out;
}
class PhysicalVisibilitySupervisor final
{
public:
    bool Configure(VisibilitySafetyParameters p) noexcept
    {
        if(!ValidVisibilitySafetyParameters(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Identity={};
        PreviousThrottle=0.0;
        PreviousBrake=0.0;
        LastSpeedLimitCmS=0.0;
        HasFrame=false;
        Qualified=0;
        Rejected=0;
        LimitedSightFrames=0;
        EmergencyFrames=0;
        MaximumObservedOcclusion=0.0;
    }
    VisibilitySafetyDecision Step(
        const PhysicalVisibilityFrame& f) noexcept
    {
        auto result=EvaluatePhysicalVisibility(f,Settings);
        const bool sameEpoch=HasFrame &&
            Identity.Session==f.Identity.Session &&
            Identity.SessionRevision==f.Identity.SessionRevision &&
            Identity.Actor==f.Identity.Actor;
        if(!result.Valid ||
           (sameEpoch && f.Identity.Frame<=Identity.Frame))
        {
            ++Rejected;
            Last={};
            return {};
        }
        if(!sameEpoch)
        {
            PreviousThrottle=0.0;
            PreviousBrake=0.0;
            LastSpeedLimitCmS=0.0;
        }
        if(LastSpeedLimitCmS>0.0)
            result.MaximumSafeSpeedCmS=std::min(
                result.MaximumSafeSpeedCmS,
                LastSpeedLimitCmS+
                Settings.SpeedReleasePerSecondCmS*
                    f.DeltaSeconds);
        result.ProposedThrottle=std::min(
            result.ProposedThrottle,
            PreviousThrottle+
            Settings.MaximumThrottleRecoveryPerSecond*
                f.DeltaSeconds);
        result.ProposedBrake=std::max(
            result.ProposedBrake,
            PreviousBrake-
            Settings.MaximumBrakeReleasePerSecond*
                f.DeltaSeconds);
        if(result.ProposedBrake>0.05)
            result.ProposedThrottle=0.0;
        result.ProposedThrottle=std::min(
            result.ProposedThrottle,f.CurrentThrottle);
        result.ProposedBrake=std::max(
            result.ProposedBrake,f.CurrentBrake);
        Identity=f.Identity;
        HasFrame=true;
        PreviousThrottle=result.ProposedThrottle;
        PreviousBrake=result.ProposedBrake;
        LastSpeedLimitCmS=result.MaximumSafeSpeedCmS;
        ++Qualified;
        LimitedSightFrames+=
            result.Class!=VisibilitySafetyClass::Clear?1U:0U;
        EmergencyFrames+=
            result.EmergencyStopRequired?1U:0U;
        MaximumObservedOcclusion=std::max(
            MaximumObservedOcclusion,
            result.MaximumNearFieldOcclusion);
        Last=result;
        return Last;
    }
    VisibilitySafetyDecision Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Qualified;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t LimitedVisibilitySamples() const noexcept
    {return LimitedSightFrames;}
    std::uint64_t EmergencyStoppingSamples() const noexcept
    {return EmergencyFrames;}
    double MaximumObservedOcclusionFraction() const noexcept
    {return MaximumObservedOcclusion;}
private:
    VisibilitySafetyParameters Settings{};
    VisibilitySafetyDecision Last{};
    RaceIdentity Identity{};
    double PreviousThrottle=0.0;
    double PreviousBrake=0.0;
    double LastSpeedLimitCmS=0.0;
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t LimitedSightFrames=0;
    std::uint64_t EmergencyFrames=0;
    double MaximumObservedOcclusion=0.0;
    bool HasFrame=false;
};
} // namespace latebrake::racecraft
