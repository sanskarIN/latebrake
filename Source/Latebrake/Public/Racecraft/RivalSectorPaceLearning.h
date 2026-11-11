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
inline constexpr int RivalPaceSectors=8;
enum class RivalPaceDecisionClass : std::uint8_t
{
    Unqualified, NoHistory, BrakingZone, FasterThanReference,
    SlowerThanReference, MatchingReference, WetGripLimited,
    TrafficLimited, TrackEdgeLimited, ConfidenceLimited
};
struct RivalPaceParameters final
{
    double CircuitLengthCm=250000.0;
    double MaximumPhysicalSpeedCmS=MaximumPhysicalSpeedCmS;
    double MinimumMeasuredSpeedCmS=200.0;
    double MaximumSectorSpeedCmS=14000.0;
    double MinimumRoadGripG=0.15;
    double MaximumRoadGripG=2.5;
    double MinimumRoadHalfWidthCm=450.0;
    double MaximumRoadHalfWidthCm=2400.0;
    double MinimumTrackEdgeReserveCm=200.0;
    double MinimumReferenceSamples=4.0;
    double MaximumReferenceSamples=10000.0;
    double MaximumFrameSeconds=0.10;
    double MaximumClockGapSeconds=0.25;
    double MaximumSourceAgeSeconds=5.0;
    double MinimumConfidence=0.75;
    double MinimumSectorTimeSeconds=1.0;
    double MaximumSectorTimeSeconds=300.0;
    double MinimumReferenceSpeedFraction=0.35;
    double MaximumReferenceSpeedFraction=1.08;
    double WetGripSafetyScale=0.80;
    double TrafficSafetyScale=0.70;
    double TrackEdgeSafetyScale=0.45;
    double DriverAggressionFactor=0.50;
    double OverSpeedThrottleCut=0.75;
    double BrakeDemandPerCmS=0.0008;
    double MinimumBrakingFloor=0.10;
    double MaximumBrakingFloor=1.0;
    double ThrottleRecoveryRatePerSecond=1.0;
    double BrakeReleasePerSecond=2.5;
    double HistoricalDecaySeconds=150.0;
    double ConfidenceGrowthPerSample=0.12;
    double MaximumSpeedStepCmS=1400.0;
    double MinimumActualWheelContacts=4.0;
    double MaximumYawRateRadiansSecond=7.0;
    double MaximumLateralSlipRadians=0.8;
    double MinimumRivalRiskConfidence=0.7;
    double MinimumRouteSampleConfidence=0.80;
    bool RequireOriginalCircuit=true;
    bool RequireQualifiedChaosVehicle=true;
    bool RejectUnverifiedHistory=true;
    bool NeverIncreaseDriverThrottle=true;
    bool DoNotAffectOfficialTiming=true;
};
struct RivalPaceSensorFrame final
{
    RaceIdentity Identity{};
    double RaceClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double OriginalTrackDistanceCm=0.0;
    double MeasuredVehicleSpeedCmS=0.0;
    double MeasuredLongitudinalAccelerationCmS2=0.0;
    double MeasuredLateralAccelerationCmS2=0.0;
    double MeasuredRoadGripG=1.0;
    double MeasuredWetnessFraction=0.0;
    double MeasuredRoadHalfWidthCm=800.0;
    double MeasuredLateralOffsetCm=0.0;
    double MeasuredYawRateRadiansSecond=0.0;
    double MeasuredLateralSlipRadians=0.0;
    double ActualRouteConfidence=1.0;
    double ActualWheelContactCount=4.0;
    double ActualTrafficConfidence=1.0;
    double MaximumTrafficThrottleCeiling=1.0;
    double MaximumTrafficBrakeFloor=0.0;
    double CurrentThrottle=0.0;
    double CurrentBrake=0.0;
    double CurrentSteering=0.0;
    bool OriginalMapConfirmed=false;
    bool EngineRaceGreen=false;
    bool ChaosWheelObservationsQualified=false;
    bool ActualRivalActor=false;
    bool TrafficRosterPhysical=false;
    bool OriginalRouteMeasured=false;
};
struct RivalPaceSectorHistory final
{
    int SectorIndex=-1;
    double MeanMeasuredSpeedCmS=0.0;
    double MinimumMeasuredSpeedCmS=0.0;
    double MaximumMeasuredSpeedCmS=0.0;
    double MeanMeasuredGripG=0.0;
    double MeanLateralSlipRadians=0.0;
    double ObservedDistanceCm=0.0;
    double PhysicalExposureSeconds=0.0;
    double LastRaceClockSeconds=0.0;
    double MeanSourceConfidence=0.0;
    double BestQualifiedSectorSeconds=0.0;
    std::uint64_t Samples=0;
    std::uint64_t BrakingSamples=0;
    std::uint64_t WetSamples=0;
    std::uint64_t TrafficLimitedSamples=0;
    std::uint64_t OriginalActorToken=0;
    bool PhysicalHistoryProven=false;
};
struct RivalPaceDecision final
{
    RaceIdentity Identity{};
    RivalPaceDecisionClass Class=RivalPaceDecisionClass::Unqualified;
    int SectorIndex=-1;
    double ReferenceSpeedCmS=0.0;
    double MeasuredSpeedCmS=0.0;
    double SpeedErrorCmS=0.0;
    double MaximumSafeSpeedCmS=0.0;
    double MeasuredGripG=0.0;
    double MeasuredWetnessFraction=0.0;
    double TrafficThrottleCeiling=0.0;
    double TrafficBrakeFloor=1.0;
    double MaximumThrottleCeiling=0.0;
    double MinimumBrakeFloor=1.0;
    double CommandThrottle=0.0;
    double CommandBrake=1.0;
    double CommandSteering=0.0;
    double HistoryConfidence=0.0;
    double SensorConfidence=0.0;
    double SectorCoverageFraction=0.0;
    double LateralEdgeReserveCm=0.0;
    double EstimatedReferenceTimeSeconds=0.0;
    int ReferenceSamples=0;
    bool NoFictionalTelemetry=true;
    bool OriginalRaceClockUnchanged=true;
    bool SourceOnly=true;
    bool Valid=false;
};
inline bool ValidRivalPaceParameters(
    RivalPaceParameters p) noexcept
{
    return Finite(p.CircuitLengthCm) &&
        p.CircuitLengthCm>5000.0 &&
        Finite(p.MaximumPhysicalSpeedCmS) &&
        p.MaximumPhysicalSpeedCmS>0.0 &&
        p.MaximumPhysicalSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MinimumMeasuredSpeedCmS) &&
        p.MinimumMeasuredSpeedCmS>=0.0 &&
        Finite(p.MaximumSectorSpeedCmS) &&
        p.MaximumSectorSpeedCmS>p.MinimumMeasuredSpeedCmS &&
        Finite(p.MinimumRoadGripG) &&
        p.MinimumRoadGripG>0.0 &&
        Finite(p.MaximumRoadGripG) &&
        p.MaximumRoadGripG>p.MinimumRoadGripG &&
        Finite(p.MinimumRoadHalfWidthCm) &&
        p.MinimumRoadHalfWidthCm>0.0 &&
        Finite(p.MaximumRoadHalfWidthCm) &&
        p.MaximumRoadHalfWidthCm>p.MinimumRoadHalfWidthCm &&
        Finite(p.MinimumTrackEdgeReserveCm) &&
        p.MinimumTrackEdgeReserveCm>=0.0 &&
        Finite(p.MinimumReferenceSamples) &&
        p.MinimumReferenceSamples>=1.0 &&
        Finite(p.MaximumReferenceSamples) &&
        p.MaximumReferenceSamples>=p.MinimumReferenceSamples &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumClockGapSeconds) &&
        p.MaximumClockGapSeconds>=p.MaximumFrameSeconds &&
        Finite(p.MaximumSourceAgeSeconds) &&
        p.MaximumSourceAgeSeconds>0.0 &&
        Finite(p.MinimumConfidence) &&
        p.MinimumConfidence>0.0 &&
        p.MinimumConfidence<=1.0 &&
        Finite(p.MinimumSectorTimeSeconds) &&
        p.MinimumSectorTimeSeconds>0.0 &&
        Finite(p.MaximumSectorTimeSeconds) &&
        p.MaximumSectorTimeSeconds>p.MinimumSectorTimeSeconds &&
        Finite(p.MinimumReferenceSpeedFraction) &&
        p.MinimumReferenceSpeedFraction>0.0 &&
        Finite(p.MaximumReferenceSpeedFraction) &&
        p.MaximumReferenceSpeedFraction>=1.0 &&
        Finite(p.WetGripSafetyScale) &&
        p.WetGripSafetyScale>0.0 &&
        p.WetGripSafetyScale<=1.0 &&
        Finite(p.TrafficSafetyScale) &&
        p.TrafficSafetyScale>0.0 &&
        p.TrafficSafetyScale<=1.0 &&
        Finite(p.TrackEdgeSafetyScale) &&
        p.TrackEdgeSafetyScale>0.0 &&
        p.TrackEdgeSafetyScale<=1.0 &&
        Finite(p.DriverAggressionFactor) &&
        p.DriverAggressionFactor>=0.0 &&
        p.DriverAggressionFactor<=1.0 &&
        Finite(p.OverSpeedThrottleCut) &&
        p.OverSpeedThrottleCut>=0.0 &&
        p.OverSpeedThrottleCut<=1.0 &&
        Finite(p.BrakeDemandPerCmS) &&
        p.BrakeDemandPerCmS>=0.0 &&
        Finite(p.MinimumBrakingFloor) &&
        p.MinimumBrakingFloor>=0.0 &&
        Finite(p.MaximumBrakingFloor) &&
        p.MaximumBrakingFloor>=p.MinimumBrakingFloor &&
        p.MaximumBrakingFloor<=1.0 &&
        Finite(p.ThrottleRecoveryRatePerSecond) &&
        p.ThrottleRecoveryRatePerSecond>0.0 &&
        Finite(p.BrakeReleasePerSecond) &&
        p.BrakeReleasePerSecond>0.0 &&
        Finite(p.HistoricalDecaySeconds) &&
        p.HistoricalDecaySeconds>0.0 &&
        Finite(p.ConfidenceGrowthPerSample) &&
        p.ConfidenceGrowthPerSample>0.0 &&
        Finite(p.MaximumSpeedStepCmS) &&
        p.MaximumSpeedStepCmS>0.0 &&
        Finite(p.MinimumActualWheelContacts) &&
        p.MinimumActualWheelContacts>=1.0 &&
        p.MinimumActualWheelContacts<=4.0 &&
        Finite(p.MaximumYawRateRadiansSecond) &&
        p.MaximumYawRateRadiansSecond>0.0 &&
        Finite(p.MaximumLateralSlipRadians) &&
        p.MaximumLateralSlipRadians>0.0 &&
        Finite(p.MinimumRivalRiskConfidence) &&
        p.MinimumRivalRiskConfidence>0.0 &&
        p.MinimumRivalRiskConfidence<=1.0 &&
        Finite(p.MinimumRouteSampleConfidence) &&
        p.MinimumRouteSampleConfidence>0.0 &&
        p.MinimumRouteSampleConfidence<=1.0;
}
inline int RivalPaceSectorIndex(
    double trackDistanceCm,
    RivalPaceParameters p) noexcept
{
    if(!Finite(trackDistanceCm) ||
       trackDistanceCm<0.0 ||
       trackDistanceCm>=p.CircuitLengthCm)
        return -1;
    return std::clamp(static_cast<int>(
        trackDistanceCm/p.CircuitLengthCm*
        RivalPaceSectors),0,RivalPaceSectors-1);
}
inline bool ValidRivalPaceFrame(
    const RivalPaceSensorFrame& s,
    RivalPaceParameters p) noexcept
{
    return ValidRivalPaceParameters(p) &&
        PhysicalActorIdentityValid(s.Identity) &&
        s.Identity.Frame>0 &&
        s.Identity.Slot!=0 &&
        s.EngineRaceGreen &&
        s.ActualRivalActor &&
        s.ChaosWheelObservationsQualified &&
        s.TrafficRosterPhysical &&
        (!p.RequireOriginalCircuit ||
         (s.OriginalMapConfirmed &&
          s.OriginalRouteMeasured)) &&
        Finite(s.RaceClockSeconds) &&
        s.RaceClockSeconds>=0.0 &&
        Finite(s.DeltaSeconds) &&
        s.DeltaSeconds>0.0 &&
        s.DeltaSeconds<=p.MaximumFrameSeconds &&
        RivalPaceSectorIndex(
            s.OriginalTrackDistanceCm,p)>=0 &&
        Finite(s.MeasuredVehicleSpeedCmS) &&
        s.MeasuredVehicleSpeedCmS>=0.0 &&
        s.MeasuredVehicleSpeedCmS<=p.MaximumPhysicalSpeedCmS &&
        Finite(s.MeasuredLongitudinalAccelerationCmS2) &&
        std::abs(s.MeasuredLongitudinalAccelerationCmS2)<=10000.0 &&
        Finite(s.MeasuredLateralAccelerationCmS2) &&
        std::abs(s.MeasuredLateralAccelerationCmS2)<=10000.0 &&
        Finite(s.MeasuredRoadGripG) &&
        s.MeasuredRoadGripG>=p.MinimumRoadGripG &&
        s.MeasuredRoadGripG<=p.MaximumRoadGripG &&
        Finite(s.MeasuredWetnessFraction) &&
        s.MeasuredWetnessFraction>=0.0 &&
        s.MeasuredWetnessFraction<=1.0 &&
        Finite(s.MeasuredRoadHalfWidthCm) &&
        s.MeasuredRoadHalfWidthCm>=p.MinimumRoadHalfWidthCm &&
        s.MeasuredRoadHalfWidthCm<=p.MaximumRoadHalfWidthCm &&
        Finite(s.MeasuredLateralOffsetCm) &&
        std::abs(s.MeasuredLateralOffsetCm)<
            s.MeasuredRoadHalfWidthCm &&
        Finite(s.MeasuredYawRateRadiansSecond) &&
        std::abs(s.MeasuredYawRateRadiansSecond)<=
            p.MaximumYawRateRadiansSecond &&
        Finite(s.MeasuredLateralSlipRadians) &&
        std::abs(s.MeasuredLateralSlipRadians)<=
            p.MaximumLateralSlipRadians &&
        Finite(s.ActualRouteConfidence) &&
        s.ActualRouteConfidence>=p.MinimumRouteSampleConfidence &&
        s.ActualRouteConfidence<=1.0 &&
        Finite(s.ActualWheelContactCount) &&
        s.ActualWheelContactCount>=p.MinimumActualWheelContacts &&
        s.ActualWheelContactCount<=4.0 &&
        Finite(s.ActualTrafficConfidence) &&
        s.ActualTrafficConfidence>=p.MinimumRivalRiskConfidence &&
        s.ActualTrafficConfidence<=1.0 &&
        Finite(s.MaximumTrafficThrottleCeiling) &&
        s.MaximumTrafficThrottleCeiling>=0.0 &&
        s.MaximumTrafficThrottleCeiling<=1.0 &&
        Finite(s.MaximumTrafficBrakeFloor) &&
        s.MaximumTrafficBrakeFloor>=0.0 &&
        s.MaximumTrafficBrakeFloor<=1.0 &&
        Finite(s.CurrentThrottle) &&
        s.CurrentThrottle>=0.0 &&
        s.CurrentThrottle<=1.0 &&
        Finite(s.CurrentBrake) &&
        s.CurrentBrake>=0.0 &&
        s.CurrentBrake<=1.0 &&
        Finite(s.CurrentSteering) &&
        std::abs(s.CurrentSteering)<=1.0;
}
inline RivalPaceDecision EvaluateRivalSectorPace(
    const RivalPaceSensorFrame& s,
    const RivalPaceSectorHistory& history,
    RivalPaceParameters p={}) noexcept
{
    RivalPaceDecision out;
    if (!ValidRivalPaceFrame(s,p))
        return out;
    const int sector=RivalPaceSectorIndex(
        s.OriginalTrackDistanceCm,p);
    out.Identity=s.Identity;
    out.SectorIndex=sector;
    out.MeasuredSpeedCmS=s.MeasuredVehicleSpeedCmS;
    out.MeasuredGripG=s.MeasuredRoadGripG;
    out.MeasuredWetnessFraction=s.MeasuredWetnessFraction;
    out.TrafficThrottleCeiling=
        s.MaximumTrafficThrottleCeiling;
    out.TrafficBrakeFloor=
        s.MaximumTrafficBrakeFloor;
    out.CommandSteering=s.CurrentSteering;
    out.LateralEdgeReserveCm=
        s.MeasuredRoadHalfWidthCm-
        std::abs(s.MeasuredLateralOffsetCm);
    out.SensorConfidence=std::min(
        s.ActualRouteConfidence,
        s.ActualTrafficConfidence);
    const bool qualifiedHistory=
        history.PhysicalHistoryProven &&
        history.SectorIndex==sector &&
        history.Samples>=
            static_cast<std::uint64_t>(
                p.MinimumReferenceSamples) &&
        history.OriginalActorToken==s.Identity.Actor &&
        Finite(history.MeanMeasuredSpeedCmS) &&
        history.MeanMeasuredSpeedCmS>=
            p.MinimumMeasuredSpeedCmS &&
        history.MeanMeasuredSpeedCmS<=p.MaximumSectorSpeedCmS &&
        history.LastRaceClockSeconds<=s.RaceClockSeconds &&
        s.RaceClockSeconds-history.LastRaceClockSeconds<=
            p.MaximumSourceAgeSeconds;
    out.ReferenceSamples=qualifiedHistory?
        static_cast<int>(history.Samples):0;
    out.HistoryConfidence=qualifiedHistory?
        std::min(1.0,
            history.MeanSourceConfidence*
            (1.0-std::exp(
                -static_cast<double>(history.Samples)*
                p.ConfidenceGrowthPerSample))):0.0;
    out.ReferenceSpeedCmS=qualifiedHistory ?
        history.MeanMeasuredSpeedCmS:
        s.MeasuredVehicleSpeedCmS;
    out.SectorCoverageFraction=qualifiedHistory?
        Clamp01(SafeDivision(
            history.ObservedDistanceCm,
            p.CircuitLengthCm/RivalPaceSectors)):0.0;
    out.EstimatedReferenceTimeSeconds=SafeDivision(
        p.CircuitLengthCm/RivalPaceSectors,
        std::max(p.MinimumMeasuredSpeedCmS,
                 out.ReferenceSpeedCmS));
    const double safeGrip=Clamp01(SafeDivision(
        s.MeasuredRoadGripG,p.MaximumRoadGripG));
    const double wetMultiplier=1.0-
        s.MeasuredWetnessFraction*(1.0-p.WetGripSafetyScale);
    const double trafficMultiplier=
        s.MaximumTrafficThrottleCeiling<0.99 ?
            p.TrafficSafetyScale:1.0;
    const double edgeMultiplier=
        out.LateralEdgeReserveCm<
            p.MinimumTrackEdgeReserveCm ?
            p.TrackEdgeSafetyScale:1.0;
    const double referenceFactor=std::clamp(
        p.MinimumReferenceSpeedFraction+
        p.DriverAggressionFactor*
            (p.MaximumReferenceSpeedFraction-
             p.MinimumReferenceSpeedFraction),
        p.MinimumReferenceSpeedFraction,
        p.MaximumReferenceSpeedFraction);
    out.MaximumSafeSpeedCmS=std::min(
        p.MaximumSectorSpeedCmS,
        out.ReferenceSpeedCmS*
            std::max(0.35,safeGrip)*
            wetMultiplier*trafficMultiplier*
            edgeMultiplier*referenceFactor);
    out.SpeedErrorCmS=
        s.MeasuredVehicleSpeedCmS-
        out.MaximumSafeSpeedCmS;
    const double overspeed=std::max(0.0,out.SpeedErrorCmS);
    out.MaximumThrottleCeiling=std::min(
        s.MaximumTrafficThrottleCeiling,
        Clamp01(1.0-p.OverSpeedThrottleCut*
            Clamp01(SafeDivision(
                overspeed,p.MaximumSpeedStepCmS))));
    out.MinimumBrakeFloor=std::max(
        s.MaximumTrafficBrakeFloor,
        overspeed>0.0 ?
            std::clamp(
                p.MinimumBrakingFloor+
                    overspeed*p.BrakeDemandPerCmS,
                0.0,p.MaximumBrakingFloor):0.0);
    out.CommandThrottle=std::min(
        s.CurrentThrottle,out.MaximumThrottleCeiling);
    out.CommandBrake=std::max(
        s.CurrentBrake,out.MinimumBrakeFloor);
    if (out.CommandBrake>0.05)
        out.CommandThrottle=0.0;
    out.Class=!qualifiedHistory ?
        RivalPaceDecisionClass::NoHistory :
        s.MeasuredWetnessFraction>0.50 ?
            RivalPaceDecisionClass::WetGripLimited :
        trafficMultiplier<1.0 ?
            RivalPaceDecisionClass::TrafficLimited :
        edgeMultiplier<1.0 ?
            RivalPaceDecisionClass::TrackEdgeLimited :
        out.SpeedErrorCmS>p.MaximumSpeedStepCmS*0.1 ?
            RivalPaceDecisionClass::FasterThanReference :
        out.SpeedErrorCmS< -p.MaximumSpeedStepCmS*0.1 ?
            RivalPaceDecisionClass::SlowerThanReference :
            RivalPaceDecisionClass::MatchingReference;
    out.NoFictionalTelemetry=true;
    out.OriginalRaceClockUnchanged=true;
    out.SourceOnly=true;
    out.Valid=Finite(out.MaximumSafeSpeedCmS) &&
        Finite(out.MinimumBrakeFloor) &&
        Finite(out.CommandThrottle) &&
        Finite(out.CommandBrake);
    return out;
}
class RivalSectorPaceSupervisor final
{
public:
    bool Configure(RivalPaceParameters p) noexcept
    {
        if (!ValidRivalPaceParameters(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        History={};
        Last={};
        Source={};
        PreviousThrottle=0.0;
        PreviousBrake=0.0;
        Accepted=0;
        Rejected=0;
        WetFrames=0;
        TrafficLimitedFrames=0;
        ExcessSpeedFrames=0;
        HasFrame=false;
    }
    RivalPaceDecision Step(
        const RivalPaceSensorFrame& s) noexcept
    {
        if (!ValidRivalPaceFrame(s,Settings))
        {
            ++Rejected;
            Last={};
            return {};
        }
        const bool sameEpoch=HasFrame &&
            Source.Session==s.Identity.Session &&
            Source.SessionRevision==s.Identity.SessionRevision &&
            Source.Actor==s.Identity.Actor;
        if (sameEpoch &&
            (s.Identity.Frame<=Source.Frame ||
             s.RaceClockSeconds<=LastClock))
        {
            ++Rejected;
            Last={};
            return {};
        }
        if (!sameEpoch)
        {
            History={};
            PreviousThrottle=0.0;
            PreviousBrake=0.0;
        }
        const int sector=RivalPaceSectorIndex(
            s.OriginalTrackDistanceCm,Settings);
        RivalPaceSectorHistory& history=
            History[static_cast<std::size_t>(sector)];
        if(history.Samples==0)
        {
            history.SectorIndex=sector;
            history.MinimumMeasuredSpeedCmS=
                s.MeasuredVehicleSpeedCmS;
            history.OriginalActorToken=s.Identity.Actor;
        }
        if(history.Samples<
            static_cast<std::uint64_t>(
                Settings.MaximumReferenceSamples))
        {
            const double oldN=static_cast<double>(history.Samples);
            const double newN=oldN+1.0;
            history.MeanMeasuredSpeedCmS+=
                (s.MeasuredVehicleSpeedCmS-
                 history.MeanMeasuredSpeedCmS)/newN;
            history.MeanMeasuredGripG+=
                (s.MeasuredRoadGripG-
                 history.MeanMeasuredGripG)/newN;
            history.MeanLateralSlipRadians+=
                (std::abs(s.MeasuredLateralSlipRadians)-
                 history.MeanLateralSlipRadians)/newN;
            history.MeanSourceConfidence+=
                (s.ActualRouteConfidence-
                 history.MeanSourceConfidence)/newN;
            ++history.Samples;
        }
        history.MaximumMeasuredSpeedCmS=std::max(
            history.MaximumMeasuredSpeedCmS,
            s.MeasuredVehicleSpeedCmS);
        history.MinimumMeasuredSpeedCmS=std::min(
            history.MinimumMeasuredSpeedCmS,
            s.MeasuredVehicleSpeedCmS);
        history.PhysicalExposureSeconds+=s.DeltaSeconds;
        history.ObservedDistanceCm+=
            s.MeasuredVehicleSpeedCmS*s.DeltaSeconds;
        history.LastRaceClockSeconds=s.RaceClockSeconds;
        history.BrakingSamples+=s.CurrentBrake>0.05?1U:0U;
        history.WetSamples+=
            s.MeasuredWetnessFraction>0.50?1U:0U;
        history.TrafficLimitedSamples+=
            s.MaximumTrafficThrottleCeiling<0.99?1U:0U;
        history.PhysicalHistoryProven=true;
        RivalPaceDecision decision=
            EvaluateRivalSectorPace(s,history,Settings);
        if(!decision.Valid)
        {
            ++Rejected;
            Last={};
            return {};
        }
        decision.CommandThrottle=std::min(
            decision.CommandThrottle,
            PreviousThrottle+
                Settings.ThrottleRecoveryRatePerSecond*
                s.DeltaSeconds);
        decision.CommandBrake=std::max(
            decision.CommandBrake,
            PreviousBrake-
                Settings.BrakeReleasePerSecond*s.DeltaSeconds);
        if(decision.CommandBrake>0.05)
            decision.CommandThrottle=0.0;
        decision.CommandThrottle=std::min(
            decision.CommandThrottle,s.CurrentThrottle);
        decision.CommandBrake=std::max(
            decision.CommandBrake,s.CurrentBrake);
        Source=s.Identity;
        LastClock=s.RaceClockSeconds;
        HasFrame=true;
        ++Accepted;
        WetFrames+=decision.Class==
            RivalPaceDecisionClass::WetGripLimited?1U:0U;
        TrafficLimitedFrames+=decision.Class==
            RivalPaceDecisionClass::TrafficLimited?1U:0U;
        ExcessSpeedFrames+=
            decision.SpeedErrorCmS>0.0?1U:0U;
        PreviousThrottle=decision.CommandThrottle;
        PreviousBrake=decision.CommandBrake;
        Last=decision;
        return Last;
    }
    RivalPaceDecision Snapshot() const noexcept{return Last;}
    RivalPaceSectorHistory Sector(int index) const noexcept
    {
        return index>=0 && index<RivalPaceSectors?
            History[static_cast<std::size_t>(index)]:
            RivalPaceSectorHistory{};
    }
    std::uint64_t Frames() const noexcept{return Accepted;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t WetRoadFrames() const noexcept{return WetFrames;}
    std::uint64_t TrafficLimitedSamples() const noexcept
    {return TrafficLimitedFrames;}
    std::uint64_t OverReferenceSpeedFrames() const noexcept
    {return ExcessSpeedFrames;}
private:
    RivalPaceParameters Settings{};
    std::array<RivalPaceSectorHistory,RivalPaceSectors> History{};
    RivalPaceDecision Last{};
    RaceIdentity Source{};
    double LastClock=0.0;
    double PreviousThrottle=0.0;
    double PreviousBrake=0.0;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t WetFrames=0;
    std::uint64_t TrafficLimitedFrames=0;
    std::uint64_t ExcessSpeedFrames=0;
    bool HasFrame=false;
};
} // namespace latebrake::racecraft
