#pragma once
#include "Racecraft/RivalOvertakeCommitment.h"
#include "Racecraft/PhysicalPitLaneApproach.h"
#include "Racecraft/PhysicalPitCrewService.h"
#include "Racecraft/MeasuredRoadWaterRouting.h"
#include "Racecraft/PhysicalSectorTimingEvidence.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int TelemetrySensorWindow=64;
enum class SensorFrameQuality : std::uint8_t
{
    Unknown, Qualified, Degraded, MissingSource,
    CrossSession, ReplayOrRollback, ClockDrift,
    InconsistentPhysics, SuspectedTeleport,
    UnverifiedMap, InvalidVehicleState
};
struct TelemetrySensorParameters final
{
    double MaximumFrameSeconds=0.10;
    double MaximumClockSkewSeconds=0.04;
    double MaximumSpeedCmS=MaximumPhysicalSpeedCmS;
    double MaximumAccelerationCmS2=10000.0;
    double MaximumYawRateRadiansSecond=9.0;
    double MaximumPositionDiscontinuityCm=2000.0;
    double MaximumVelocityDiscontinuityCmS=2500.0;
    double MaximumHeadingDiscontinuityRadians=0.7;
    double MaximumWheelSpeedDisagreementCmS=900.0;
    double MaximumReportedThrottleError=0.05;
    double MaximumReportedBrakeError=0.05;
    double MaximumReportedSteeringError=0.08;
    double MinimumSensorConfidence=0.80;
    double MinimumHealthConfidence=0.50;
    double MinimumQualifyingWheelCount=4.0;
    double MaximumClockGapSeconds=0.25;
    double MaximumSensorAgeSeconds=0.20;
    double MinimumSensorAgeSeconds=0.0;
    double MaximumRoadGripDisagreement=0.5;
    double MaximumHydraulicMismatch=0.60;
    double MaximumLateralLoadDisagreement=0.70;
    double MaximumWheelLoadN=30000.0;
    double MaximumMeasuredTireTemperatureC=200.0;
    double MaximumObservedChassisMassKg=9000.0;
    double MinimumObservedChassisMassKg=300.0;
    double MaximumObservedRoadHalfWidthCm=3500.0;
    double MinimumObservedRoadHalfWidthCm=350.0;
    double MaximumRaceClockSeconds=86400.0;
    double MinimumPhysicalInteractionConfidence=0.65;
    double MaximumSessionRevisionJump=1.0;
    double MaximumAllowedPhysicsFrameGap=8.0;
    double SuspiciousOverspeedRatio=1.02;
    double MinimumEvidenceFramesForTrust=3.0;
    bool RequireOriginalTrack=true;
    bool RequireActualChaosBody=true;
    bool RequireAllWheelSensors=true;
    bool NeverGrantInputAuthority=true;
    bool NeverModifyOfficialResults=true;
};
struct RaceTelemetryWheelFrame final
{
    WheelObservation Wheel{};
    double MeasuredHubSpeedCmS=0.0;
    double MeasuredBrakeTorqueNm=0.0;
    double MeasuredLateralForceN=0.0;
    double MeasuredLongitudinalForceN=0.0;
    double SensorConfidence=1.0;
    double SourceClockSeconds=0.0;
    bool EngineWheelComponentVerified=false;
};
struct RaceTelemetryPhysicalFrame final
{
    RaceIdentity Identity{};
    Pose2 ChassisPose{};
    Vec2 MeasuredVelocityCmS{};
    Vec2 MeasuredAccelerationCmS2{};
    std::array<RaceTelemetryWheelFrame,PhysicalWheelCount> Wheels{};
    double PhysicsClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double ActualYawRateRadiansSecond=0.0;
    double ReportedSpeedCmS=0.0;
    double MeasuredRoadGrip=1.0;
    double MeasuredRoadHalfWidthCm=800.0;
    double MeasuredChassisMassKg=1300.0;
    double DriverThrottle=0.0;
    double DriverBrake=0.0;
    double DriverSteering=0.0;
    double ActualEngineThrottle=0.0;
    double ActualEngineBrake=0.0;
    double ActualEngineSteering=0.0;
    double SourceConfidence=1.0;
    bool EngineRaceGreen=false;
    bool OriginalTrackVerified=false;
    bool OriginalMapCollisionQualified=false;
    bool ChaosBodySimulating=false;
    bool EngineInputAuthorityObserved=false;
};
struct RaceTelemetryAnomaly final
{
    SensorFrameQuality Quality=SensorFrameQuality::Unknown;
    RaceIdentity Identity{};
    double VelocityDisagreementCmS=0.0;
    double AverageWheelSpeedCmS=0.0;
    double MaximumWheelSpeedErrorCmS=0.0;
    double LongitudinalForceN=0.0;
    double LateralForceN=0.0;
    double BrakingTorqueNm=0.0;
    double TotalNormalLoadN=0.0;
    double YawRateRadiansSecond=0.0;
    double MeasuredChassisEnergyJ=0.0;
    double MaximumSourceClockSkewSeconds=0.0;
    double MinimumSourceConfidence=0.0;
    double ControlThrottleError=0.0;
    double ControlBrakeError=0.0;
    double ControlSteeringError=0.0;
    double GripConsistency=0.0;
    double WheelContactFraction=0.0;
    double VehicleAccelerationG=0.0;
    int VerifiedWheels=0;
    int SourceErrors=0;
    bool OriginalPhysicsEvidence=true;
    bool PotentialTeleport=false;
    bool GameplayAuthorityIssued=false;
    bool Valid=false;
};
struct RaceTelemetryWindowSummary final
{
    RaceIdentity LastSource{};
    double ElapsedMeasuredSeconds=0.0;
    double TotalDistanceCm=0.0;
    double PeakMeasuredSpeedCmS=0.0;
    double PeakMeasuredAccelerationG=0.0;
    double PeakMeasuredYawRateRadiansSecond=0.0;
    double MinimumConfidence=0.0;
    double MaximumClockSkewSeconds=0.0;
    double MeanMeasuredGrip=0.0;
    double MeanWheelContactFraction=0.0;
    double EstimatedDistanceFromWheelSpeedCm=0.0;
    double SourceCorrectnessFraction=0.0;
    int FramesInWindow=0;
    int QualifiedFrames=0;
    int DegradedFrames=0;
    int RejectedFrames=0;
    int TeleportWarnings=0;
    int MissingWheelSamples=0;
    bool AllAcceptedOriginalMap=true;
    bool SourceOnly=true;
    bool Valid=false;
};
inline bool ValidTelemetrySensorParameters(
    TelemetrySensorParameters p) noexcept
{
    return Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        Finite(p.MaximumSpeedCmS) &&
        p.MaximumSpeedCmS>0.0 &&
        p.MaximumSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MaximumAccelerationCmS2) &&
        p.MaximumAccelerationCmS2>0.0 &&
        Finite(p.MaximumYawRateRadiansSecond) &&
        p.MaximumYawRateRadiansSecond>0.0 &&
        Finite(p.MaximumPositionDiscontinuityCm) &&
        p.MaximumPositionDiscontinuityCm>0.0 &&
        Finite(p.MaximumVelocityDiscontinuityCmS) &&
        p.MaximumVelocityDiscontinuityCmS>0.0 &&
        Finite(p.MaximumHeadingDiscontinuityRadians) &&
        p.MaximumHeadingDiscontinuityRadians>0.0 &&
        Finite(p.MaximumWheelSpeedDisagreementCmS) &&
        p.MaximumWheelSpeedDisagreementCmS>0.0 &&
        Finite(p.MaximumReportedThrottleError) &&
        p.MaximumReportedThrottleError>=0.0 &&
        Finite(p.MaximumReportedBrakeError) &&
        p.MaximumReportedBrakeError>=0.0 &&
        Finite(p.MaximumReportedSteeringError) &&
        p.MaximumReportedSteeringError>=0.0 &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.MinimumHealthConfidence) &&
        p.MinimumHealthConfidence>0.0 &&
        p.MinimumHealthConfidence<=1.0 &&
        Finite(p.MinimumQualifyingWheelCount) &&
        p.MinimumQualifyingWheelCount>=1.0 &&
        p.MinimumQualifyingWheelCount<=4.0 &&
        Finite(p.MaximumClockGapSeconds) &&
        p.MaximumClockGapSeconds>0.0 &&
        Finite(p.MaximumSensorAgeSeconds) &&
        p.MaximumSensorAgeSeconds>0.0 &&
        Finite(p.MinimumSensorAgeSeconds) &&
        p.MinimumSensorAgeSeconds>=0.0 &&
        Finite(p.MaximumRoadGripDisagreement) &&
        p.MaximumRoadGripDisagreement>=0.0 &&
        Finite(p.MaximumHydraulicMismatch) &&
        p.MaximumHydraulicMismatch>=0.0 &&
        Finite(p.MaximumLateralLoadDisagreement) &&
        p.MaximumLateralLoadDisagreement>=0.0 &&
        Finite(p.MaximumWheelLoadN) &&
        p.MaximumWheelLoadN>0.0 &&
        Finite(p.MaximumMeasuredTireTemperatureC) &&
        p.MaximumMeasuredTireTemperatureC>0.0 &&
        Finite(p.MaximumObservedChassisMassKg) &&
        p.MaximumObservedChassisMassKg>0.0 &&
        Finite(p.MinimumObservedChassisMassKg) &&
        p.MinimumObservedChassisMassKg>0.0 &&
        p.MinimumObservedChassisMassKg<
            p.MaximumObservedChassisMassKg &&
        Finite(p.MaximumObservedRoadHalfWidthCm) &&
        p.MaximumObservedRoadHalfWidthCm>0.0 &&
        Finite(p.MinimumObservedRoadHalfWidthCm) &&
        p.MinimumObservedRoadHalfWidthCm>0.0 &&
        p.MinimumObservedRoadHalfWidthCm<
            p.MaximumObservedRoadHalfWidthCm &&
        Finite(p.MaximumRaceClockSeconds) &&
        p.MaximumRaceClockSeconds>0.0 &&
        Finite(p.MinimumPhysicalInteractionConfidence) &&
        p.MinimumPhysicalInteractionConfidence>0.0 &&
        p.MinimumPhysicalInteractionConfidence<=1.0 &&
        Finite(p.MaximumSessionRevisionJump) &&
        p.MaximumSessionRevisionJump>=0.0 &&
        Finite(p.MaximumAllowedPhysicsFrameGap) &&
        p.MaximumAllowedPhysicsFrameGap>=1.0 &&
        Finite(p.SuspiciousOverspeedRatio) &&
        p.SuspiciousOverspeedRatio>=1.0 &&
        Finite(p.MinimumEvidenceFramesForTrust) &&
        p.MinimumEvidenceFramesForTrust>=1.0;
}
inline bool ValidTelemetryWheel(
    RaceTelemetryWheelFrame w,
    TelemetrySensorParameters p) noexcept
{
    return ValidWheelObservation(w.Wheel) &&
        (!p.RequireAllWheelSensors ||
         (w.EngineWheelComponentVerified &&
          w.Wheel.ContactVerified &&
          w.Wheel.PhysicalContact)) &&
        Finite(w.MeasuredHubSpeedCmS) &&
        std::abs(w.MeasuredHubSpeedCmS)<=p.MaximumSpeedCmS &&
        Finite(w.MeasuredBrakeTorqueNm) &&
        std::abs(w.MeasuredBrakeTorqueNm)<=20000.0 &&
        Finite(w.MeasuredLateralForceN) &&
        std::abs(w.MeasuredLateralForceN)<=80000.0 &&
        Finite(w.MeasuredLongitudinalForceN) &&
        std::abs(w.MeasuredLongitudinalForceN)<=80000.0 &&
        Finite(w.SensorConfidence) &&
        w.SensorConfidence>=p.MinimumSensorConfidence &&
        w.SensorConfidence<=1.0 &&
        Finite(w.SourceClockSeconds) &&
        w.SourceClockSeconds>=0.0 &&
        w.Wheel.NormalLoadN<=p.MaximumWheelLoadN &&
        w.Wheel.TemperatureC<=
            p.MaximumMeasuredTireTemperatureC;
}
inline bool ValidTelemetryPhysicalFrame(
    const RaceTelemetryPhysicalFrame& s,
    TelemetrySensorParameters p) noexcept
{
    if (!ValidTelemetrySensorParameters(p) ||
        !PhysicalActorIdentityValid(s.Identity) ||
        s.Identity.Frame==0 ||
        !s.EngineRaceGreen ||
        !s.OriginalMapCollisionQualified ||
        !s.EngineInputAuthorityObserved ||
        (p.RequireOriginalTrack &&
         !s.OriginalTrackVerified) ||
        (p.RequireActualChaosBody &&
         !s.ChaosBodySimulating) ||
        !Finite(s.ChassisPose) ||
        !Finite(s.MeasuredVelocityCmS) ||
        Length(s.MeasuredVelocityCmS)>
            p.MaximumSpeedCmS ||
        !Finite(s.MeasuredAccelerationCmS2) ||
        Length(s.MeasuredAccelerationCmS2)>
            p.MaximumAccelerationCmS2 ||
        !Finite(s.PhysicsClockSeconds) ||
        s.PhysicsClockSeconds<0.0 ||
        s.PhysicsClockSeconds>p.MaximumRaceClockSeconds ||
        !Finite(s.DeltaSeconds) ||
        s.DeltaSeconds<=0.0 ||
        s.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(s.ActualYawRateRadiansSecond) ||
        std::abs(s.ActualYawRateRadiansSecond)>
            p.MaximumYawRateRadiansSecond ||
        !Finite(s.ReportedSpeedCmS) ||
        s.ReportedSpeedCmS<0.0 ||
        s.ReportedSpeedCmS>p.MaximumSpeedCmS ||
        !Finite(s.MeasuredRoadGrip) ||
        s.MeasuredRoadGrip<0.15 ||
        s.MeasuredRoadGrip>2.5 ||
        !Finite(s.MeasuredRoadHalfWidthCm) ||
        s.MeasuredRoadHalfWidthCm<
            p.MinimumObservedRoadHalfWidthCm ||
        s.MeasuredRoadHalfWidthCm>
            p.MaximumObservedRoadHalfWidthCm ||
        !Finite(s.MeasuredChassisMassKg) ||
        s.MeasuredChassisMassKg<
            p.MinimumObservedChassisMassKg ||
        s.MeasuredChassisMassKg>
            p.MaximumObservedChassisMassKg ||
        !Finite(s.DriverThrottle) ||
        s.DriverThrottle<0.0 ||
        s.DriverThrottle>1.0 ||
        !Finite(s.DriverBrake) ||
        s.DriverBrake<0.0 ||
        s.DriverBrake>1.0 ||
        !Finite(s.DriverSteering) ||
        std::abs(s.DriverSteering)>1.0 ||
        !Finite(s.ActualEngineThrottle) ||
        s.ActualEngineThrottle<0.0 ||
        s.ActualEngineThrottle>1.0 ||
        !Finite(s.ActualEngineBrake) ||
        s.ActualEngineBrake<0.0 ||
        s.ActualEngineBrake>1.0 ||
        !Finite(s.ActualEngineSteering) ||
        std::abs(s.ActualEngineSteering)>1.0 ||
        !Finite(s.SourceConfidence) ||
        s.SourceConfidence<p.MinimumSensorConfidence ||
        s.SourceConfidence>1.0)
        return false;
    for(const auto& wheel:s.Wheels)
        if (!ValidTelemetryWheel(wheel,p) ||
            std::abs(wheel.SourceClockSeconds-
                     s.PhysicsClockSeconds)>
                p.MaximumClockSkewSeconds)
            return false;
    return true;
}
inline RaceTelemetryAnomaly AuditPhysicalTelemetry(
    const RaceTelemetryPhysicalFrame& s,
    TelemetrySensorParameters p={}) noexcept
{
    RaceTelemetryAnomaly out;
    if (!ValidTelemetryPhysicalFrame(s,p))
        return out;
    out.Identity=s.Identity;
    out.YawRateRadiansSecond=
        s.ActualYawRateRadiansSecond;
    out.MeasuredChassisEnergyJ=
        0.5*s.MeasuredChassisMassKg*
            Square(Length(s.MeasuredVelocityCmS)/100.0);
    out.MinimumSourceConfidence=s.SourceConfidence;
    out.VehicleAccelerationG=
        Length(s.MeasuredAccelerationCmS2)/GravityCmS2;
    out.ControlThrottleError=std::abs(
        s.DriverThrottle-s.ActualEngineThrottle);
    out.ControlBrakeError=std::abs(
        s.DriverBrake-s.ActualEngineBrake);
    out.ControlSteeringError=std::abs(
        s.DriverSteering-s.ActualEngineSteering);
    out.VelocityDisagreementCmS=std::abs(
        Length(s.MeasuredVelocityCmS)-s.ReportedSpeedCmS);
    for(const auto& wheel:s.Wheels)
    {
        out.AverageWheelSpeedCmS+=
            std::abs(wheel.MeasuredHubSpeedCmS)*0.25;
        out.MaximumWheelSpeedErrorCmS=std::max(
            out.MaximumWheelSpeedErrorCmS,
            std::abs(
                std::abs(wheel.MeasuredHubSpeedCmS)-
                s.ReportedSpeedCmS));
        out.LongitudinalForceN+=
            wheel.MeasuredLongitudinalForceN;
        out.LateralForceN+=
            wheel.MeasuredLateralForceN;
        out.BrakingTorqueNm+=
            wheel.MeasuredBrakeTorqueNm;
        out.TotalNormalLoadN+=wheel.Wheel.NormalLoadN;
        out.MaximumSourceClockSkewSeconds=std::max(
            out.MaximumSourceClockSkewSeconds,
            std::abs(wheel.SourceClockSeconds-
                     s.PhysicsClockSeconds));
        out.MinimumSourceConfidence=std::min(
            out.MinimumSourceConfidence,
            wheel.SensorConfidence);
        out.VerifiedWheels+=wheel.EngineWheelComponentVerified?1:0;
    }
    out.WheelContactFraction=
        static_cast<double>(out.VerifiedWheels)/
        PhysicalWheelCount;
    out.GripConsistency=SafeDivision(
        std::hypot(out.LongitudinalForceN,out.LateralForceN),
        std::max(1.0,
                 out.TotalNormalLoadN*s.MeasuredRoadGrip));
    out.SourceErrors+=
        out.ControlThrottleError>
            p.MaximumReportedThrottleError?1:0;
    out.SourceErrors+=
        out.ControlBrakeError>
            p.MaximumReportedBrakeError?1:0;
    out.SourceErrors+=
        out.ControlSteeringError>
            p.MaximumReportedSteeringError?1:0;
    out.SourceErrors+=
        out.VelocityDisagreementCmS>
            p.MaximumWheelSpeedDisagreementCmS?1:0;
    out.SourceErrors+=
        out.MaximumWheelSpeedErrorCmS>
            p.MaximumWheelSpeedDisagreementCmS?1:0;
    out.Quality=out.SourceErrors>0 ?
        SensorFrameQuality::InconsistentPhysics:
        out.MinimumSourceConfidence<
            p.MinimumHealthConfidence ?
            SensorFrameQuality::Degraded:
            SensorFrameQuality::Qualified;
    out.OriginalPhysicsEvidence=true;
    out.GameplayAuthorityIssued=false;
    out.Valid=Finite(out.MeasuredChassisEnergyJ) &&
        Finite(out.MinimumSourceConfidence) &&
        Finite(out.GripConsistency) &&
        Finite(out.MaximumWheelSpeedErrorCmS);
    return out;
}
class RaceTelemetrySourceWindow final
{
public:
    bool Configure(TelemetrySensorParameters settings) noexcept
    {
        if(!ValidTelemetrySensorParameters(settings))return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Frames={};
        Audit={};
        Identity={};
        HasFrame=false;
        Head=0;
        Size=0;
        Accepted=0;
        Rejected=0;
        TeleportWarnings=0;
    }
    RaceTelemetryAnomaly Record(
        const RaceTelemetryPhysicalFrame& frame) noexcept
    {
        RaceTelemetryAnomaly result=
            AuditPhysicalTelemetry(frame,Settings);
        const bool sameEpoch=HasFrame &&
            Identity.Session==frame.Identity.Session &&
            Identity.SessionRevision==
                frame.Identity.SessionRevision &&
            Identity.Actor==frame.Identity.Actor;
        if (!result.Valid ||
            (sameEpoch && frame.Identity.Frame<=Identity.Frame))
        {
            ++Rejected;
            return {};
        }
        if (sameEpoch && Size>0)
        {
            const auto& previous=Frames[
                (Head+TelemetrySensorWindow-1)%
                    TelemetrySensorWindow];
            const double dt=frame.PhysicsClockSeconds-
                previous.PhysicsClockSeconds;
            const double displacement=Length(
                frame.ChassisPose.Position-
                previous.ChassisPose.Position);
            const double velocityChange=Length(
                frame.MeasuredVelocityCmS-
                previous.MeasuredVelocityCmS);
            const double maxDistance=Settings.MaximumPositionDiscontinuityCm+
                Length(frame.MeasuredVelocityCmS)*
                    std::max(0.0,dt);
            if (dt<=0.0 ||
                dt>Settings.MaximumClockGapSeconds ||
                displacement>maxDistance ||
                velocityChange>
                    Settings.MaximumVelocityDiscontinuityCmS)
            {
                result.PotentialTeleport=true;
                result.Quality=
                    SensorFrameQuality::SuspectedTeleport;
                ++TeleportWarnings;
            }
        }
        Frames[Head]=frame;
        Audit[Head]=result;
        Head=(Head+1)%TelemetrySensorWindow;
        Size=std::min(
            Size+1,TelemetrySensorWindow);
        Identity=frame.Identity;
        HasFrame=true;
        ++Accepted;
        return result;
    }
    RaceTelemetryWindowSummary Summarize() const noexcept
    {
        RaceTelemetryWindowSummary out;
        if(Size==0)return out;
        out.LastSource=Identity;
        out.FramesInWindow=static_cast<int>(Size);
        out.MinimumConfidence=1.0;
        double totalGrip=0.0;
        double totalContact=0.0;
        for(std::size_t n=0;n<Size;++n)
        {
            const std::size_t index=
                (Head+TelemetrySensorWindow-Size+n)%
                TelemetrySensorWindow;
            const auto& frame=Frames[index];
            const auto& evidence=Audit[index];
            if(!evidence.Valid)
            {
                ++out.RejectedFrames;
                continue;
            }
            ++out.QualifiedFrames;
            out.DegradedFrames+=
                evidence.Quality!=SensorFrameQuality::Qualified?1:0;
            out.TeleportWarnings+=
                evidence.PotentialTeleport?1:0;
            out.PeakMeasuredSpeedCmS=std::max(
                out.PeakMeasuredSpeedCmS,
                frame.ReportedSpeedCmS);
            out.PeakMeasuredAccelerationG=std::max(
                out.PeakMeasuredAccelerationG,
                evidence.VehicleAccelerationG);
            out.PeakMeasuredYawRateRadiansSecond=std::max(
                out.PeakMeasuredYawRateRadiansSecond,
                std::abs(frame.ActualYawRateRadiansSecond));
            out.MinimumConfidence=std::min(
                out.MinimumConfidence,
                evidence.MinimumSourceConfidence);
            out.MaximumClockSkewSeconds=std::max(
                out.MaximumClockSkewSeconds,
                evidence.MaximumSourceClockSkewSeconds);
            totalGrip+=frame.MeasuredRoadGrip;
            totalContact+=evidence.WheelContactFraction;
            out.EstimatedDistanceFromWheelSpeedCm+=
                evidence.AverageWheelSpeedCmS*
                frame.DeltaSeconds;
            out.ElapsedMeasuredSeconds+=frame.DeltaSeconds;
        }
        const auto& newest=Frames[
            (Head+TelemetrySensorWindow-1)%
            TelemetrySensorWindow];
        const auto& oldest=Frames[
            (Head+TelemetrySensorWindow-Size)%
            TelemetrySensorWindow];
        out.TotalDistanceCm=Length(
            newest.ChassisPose.Position-
            oldest.ChassisPose.Position);
        out.MeanMeasuredGrip=SafeDivision(
            totalGrip,static_cast<double>(out.QualifiedFrames));
        out.MeanWheelContactFraction=SafeDivision(
            totalContact,static_cast<double>(out.QualifiedFrames));
        out.SourceCorrectnessFraction=SafeDivision(
            static_cast<double>(
                out.QualifiedFrames-out.DegradedFrames),
            static_cast<double>(out.FramesInWindow));
        out.AllAcceptedOriginalMap=true;
        out.SourceOnly=true;
        out.Valid=Finite(out.TotalDistanceCm) &&
            Finite(out.MeanMeasuredGrip) &&
            Finite(out.SourceCorrectnessFraction);
        return out;
    }
    std::uint64_t FramesAccepted() const noexcept{return Accepted;}
    std::uint64_t FramesRejected() const noexcept{return Rejected;}
    std::uint64_t TeleportWarningCount() const noexcept
    {return TeleportWarnings;}
private:
    TelemetrySensorParameters Settings{};
    std::array<RaceTelemetryPhysicalFrame,TelemetrySensorWindow> Frames{};
    std::array<RaceTelemetryAnomaly,TelemetrySensorWindow> Audit{};
    RaceIdentity Identity{};
    std::size_t Head=0;
    std::size_t Size=0;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t TeleportWarnings=0;
    bool HasFrame=false;
};
} // namespace latebrake::racecraft
