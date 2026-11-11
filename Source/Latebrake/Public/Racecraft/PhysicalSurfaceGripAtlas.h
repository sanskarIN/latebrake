#pragma once
#include "Racecraft/CornerGripForecast.h"
#include "Racecraft/CircuitGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int GripAtlasCells=32;
enum class GripRecordFault : std::uint8_t
{
    None, NotConfigured, UnqualifiedEngine, WrongActor,
    DuplicateFrame, BadSessionClock, MissingRoad,
    InvalidWheel, MissingContact, BadFriction,
    UnphysicalSlip, UnphysicalChange, MissingSession
};
struct GripAtlasSettings final
{
    double CircuitLengthCm=OriginalCircuitLapCm;
    double MaximumFrameSeconds=0.1;
    double MaximumClockJumpSeconds=0.15;
    double MaximumSensorAgeSeconds=10.0;
    double MinimumClockAdvanceSeconds=0.0001;
    double MinimumPhysicalWheelLoadN=200.0;
    double MaximumPhysicalWheelLoadN=40000.0;
    double MinimumTotalNormalLoadN=6000.0;
    double MaximumTotalNormalLoadN=80000.0;
    double MinimumPhysicalSpeedCmS=300.0;
    double MaximumTrustedSpeedCmS=MaximumPhysicalSpeedCmS;
    double MinimumRoadGrip=0.15;
    double MaximumRoadGrip=2.5;
    double MinimumFrictionScale=0.15;
    double MaximumFrictionScale=2.0;
    double MaximumTrustedSlipRatio=0.60;
    double MaximumTrustedSlipAngleRadians=0.40;
    double MaximumLoadImbalance=0.90;
    double MaximumWetnessFraction=1.0;
    double MaximumRoadVariationPerSecond=0.60;
    double MinimumSensorConfidence=0.65;
    double MaximumStatisticalVariance=0.50;
    double ConfidenceVariancePenalty=0.40;
    double MinimumCellObservations=3.0;
    double ExponentialFilterTimeSeconds=0.50;
    double MaximumPredictionDistanceCm=3000.0;
    double MaximumRoadGripBenefitFraction=0.05;
    double HazardGripThreshold=0.65;
    double WetnessHazardThreshold=0.50;
    bool RequireActualWheelPhysics=true;
    bool RequireOriginalAsphalt=true;
    bool RequireContactSensor=true;
    bool RejectUnmeasuredSegments=true;
    bool RejectActorChanges=true;
};

struct GripContactFrame final
{
    RaceIdentity Identity{};
    std::array<WheelObservation,PhysicalWheelCount> Wheels{};
    double DistanceAlongOriginalLapCm=0.0;
    double RaceClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double MeasuredForwardSpeedCmS=0.0;
    double RoadFrictionScale=1.0;
    double ActualWetnessFraction=0.0;
    double MeasurementConfidence=1.0;
    double MaximumPhysicalRoadGradeRadians=0.0;
    ContactSurface MeasuredMaterial=ContactSurface::Unknown;
    bool EngineRaceRunning=false;
    bool OriginalMapPhysicsReady=false;
    bool RoadGeometryVerified=false;
    bool OriginalSurfaceContact=false;
    bool RealFrictionSensorReady=false;
};

struct GripCellEvidence final
{
    double MeasuredGrip=0.0;
    double MeanGrip=0.0;
    double VarianceNumerator=0.0;
    double WetnessFraction=0.0;
    double FilteredRoadFriction=0.0;
    double LastClockSeconds=0.0;
    double MaximumSlipRatio=0.0;
    double MaximumSlipAngleRadians=0.0;
    double TotalNormalLoadN=0.0;
    double MeanSensorConfidence=0.0;
    double MinimumObservedGrip=0.0;
    double MaximumObservedGrip=0.0;
    double MinimumObservedWetness=0.0;
    double MaximumObservedWetness=0.0;
    std::uint64_t Samples=0;
    std::uint64_t LastFrame=0;
    std::uint64_t SourceActor=0;
    ContactSurface MostRecentMaterial=ContactSurface::Unknown;
    bool OnPhysicalRoad=false;
    bool HasMeasuredContact=false;
};

struct GripRecordResult final
{
    GripRecordFault Fault=GripRecordFault::NotConfigured;
    int CellIndex=-1;
    double NewGrip=0.0;
    double MeasuredWetness=0.0;
    double MeasuredNormalLoadN=0.0;
    double Confidence=0.0;
    double MaximumSlip=0.0;
    bool ChangedWetnessClass=false;
    bool Qualified=false;
    bool AppliedToChaos=false;
};

struct GripAtlasQuery final
{
    int CellIndex=-1;
    double DistanceCm=0.0;
    double MeasuredGripScale=0.0;
    double MinimumMeasuredGrip=0.0;
    double RoadFrictionScale=0.0;
    double FilteredWetness=0.0;
    double MaximumObservedSlip=0.0;
    double StatisticalVariance=0.0;
    double Confidence=0.0;
    double SensorAgeSeconds=0.0;
    std::uint64_t Samples=0;
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    ContactSurface Material=ContactSurface::Unknown;
    bool HazardousLowGrip=false;
    bool StandingWaterHazard=false;
    bool ProvenOriginalRoad=false;
    bool PhysicsObserved=false;
    bool ReadyForPlanning=false;
    bool Valid=false;
};

struct GripRoadAheadReview final
{
    double StartingDistanceCm=0.0;
    double LookaheadCm=0.0;
    double NearestPhysicalHazardCm=0.0;
    double MinimumMeasuredGrip=0.0;
    double MaximumMeasuredWetness=0.0;
    double MinimumConfidence=0.0;
    double OldestEvidenceSeconds=0.0;
    double VerifiedAheadCm=0.0;
    double UnknownAheadCm=0.0;
    int NearestHazardCell=-1;
    int VerifiedCells=0;
    int UnknownCells=0;
    int HazardCells=0;
    int RoadCellsEncountered=0;
    bool WetHazardSeen=false;
    bool LowGripHazardSeen=false;
    bool FullRoadEvidence=false;
    bool NeverInferredUnobservedGrip=true;
    bool Valid=false;
};
struct GripAtlasProgress final
{
    int ConfirmedCells=0;
    int HazardousCells=0;
    int FreshCells=0;
    double PhysicalCoverageFraction=0.0;
    double MinimumConfirmedGrip=0.0;
    double MaximumConfirmedGrip=0.0;
    double AverageSensorConfidence=0.0;
    std::uint64_t QualifiedMeasurements=0;
    std::uint64_t RejectedMeasurements=0;
    std::uint64_t DuplicateFrames=0;
    std::uint64_t RealWetTransitions=0;
    bool CoverageIsMeasuredOnly=true;
};

inline bool ValidGripAtlasSettings(
    GripAtlasSettings p) noexcept
{
    return Finite(p.CircuitLengthCm) &&
        p.CircuitLengthCm>5000.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumClockJumpSeconds) &&
        p.MaximumClockJumpSeconds>=p.MaximumFrameSeconds &&
        Finite(p.MaximumSensorAgeSeconds) &&
        p.MaximumSensorAgeSeconds>0.0 &&
        Finite(p.MinimumClockAdvanceSeconds) &&
        p.MinimumClockAdvanceSeconds>0.0 &&
        p.MinimumClockAdvanceSeconds<p.MaximumFrameSeconds &&
        Finite(p.MinimumPhysicalWheelLoadN) &&
        p.MinimumPhysicalWheelLoadN>0.0 &&
        Finite(p.MaximumPhysicalWheelLoadN) &&
        p.MaximumPhysicalWheelLoadN>
            p.MinimumPhysicalWheelLoadN &&
        Finite(p.MinimumTotalNormalLoadN) &&
        p.MinimumTotalNormalLoadN>0.0 &&
        Finite(p.MaximumTotalNormalLoadN) &&
        p.MaximumTotalNormalLoadN>
            p.MinimumTotalNormalLoadN &&
        Finite(p.MinimumPhysicalSpeedCmS) &&
        p.MinimumPhysicalSpeedCmS>=0.0 &&
        Finite(p.MaximumTrustedSpeedCmS) &&
        p.MaximumTrustedSpeedCmS>
            p.MinimumPhysicalSpeedCmS &&
        p.MaximumTrustedSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MinimumRoadGrip) &&
        p.MinimumRoadGrip>0.0 &&
        Finite(p.MaximumRoadGrip) &&
        p.MaximumRoadGrip>=p.MinimumRoadGrip &&
        Finite(p.MinimumFrictionScale) &&
        p.MinimumFrictionScale>0.0 &&
        Finite(p.MaximumFrictionScale) &&
        p.MaximumFrictionScale>=p.MinimumFrictionScale &&
        Finite(p.MaximumTrustedSlipRatio) &&
        p.MaximumTrustedSlipRatio>0.0 &&
        Finite(p.MaximumTrustedSlipAngleRadians) &&
        p.MaximumTrustedSlipAngleRadians>0.0 &&
        Finite(p.MaximumLoadImbalance) &&
        p.MaximumLoadImbalance>=0.0 &&
        p.MaximumLoadImbalance<=1.0 &&
        Finite(p.MaximumWetnessFraction) &&
        p.MaximumWetnessFraction>0.0 &&
        p.MaximumWetnessFraction<=1.0 &&
        Finite(p.MaximumRoadVariationPerSecond) &&
        p.MaximumRoadVariationPerSecond>0.0 &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.MaximumStatisticalVariance) &&
        p.MaximumStatisticalVariance>0.0 &&
        Finite(p.ConfidenceVariancePenalty) &&
        p.ConfidenceVariancePenalty>=0.0 &&
        p.ConfidenceVariancePenalty<=1.0 &&
        Finite(p.MinimumCellObservations) &&
        p.MinimumCellObservations>=1.0 &&
        Finite(p.ExponentialFilterTimeSeconds) &&
        p.ExponentialFilterTimeSeconds>0.0 &&
        Finite(p.MaximumPredictionDistanceCm) &&
        p.MaximumPredictionDistanceCm>0.0 &&
        Finite(p.MaximumRoadGripBenefitFraction) &&
        p.MaximumRoadGripBenefitFraction>=0.0 &&
        p.MaximumRoadGripBenefitFraction<1.0 &&
        Finite(p.HazardGripThreshold) &&
        p.HazardGripThreshold>0.0 &&
        Finite(p.WetnessHazardThreshold) &&
        p.WetnessHazardThreshold>0.0 &&
        p.WetnessHazardThreshold<=1.0;
}

inline bool ValidGripContactFrame(
    const GripContactFrame& sample,
    GripAtlasSettings p) noexcept
{
    if (!ValidGripAtlasSettings(p) ||
        !PhysicalActorIdentityValid(sample.Identity) ||
        sample.Identity.Frame==0 ||
        !sample.EngineRaceRunning ||
        !sample.OriginalMapPhysicsReady ||
        !sample.RoadGeometryVerified ||
        (p.RequireOriginalAsphalt &&
         !sample.OriginalSurfaceContact) ||
        (p.RequireContactSensor &&
         !sample.RealFrictionSensorReady) ||
        !Finite(sample.DistanceAlongOriginalLapCm) ||
        sample.DistanceAlongOriginalLapCm<0.0 ||
        sample.DistanceAlongOriginalLapCm>=p.CircuitLengthCm ||
        !Finite(sample.RaceClockSeconds) ||
        sample.RaceClockSeconds<0.0 ||
        !Finite(sample.DeltaSeconds) ||
        sample.DeltaSeconds<=0.0 ||
        sample.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(sample.MeasuredForwardSpeedCmS) ||
        sample.MeasuredForwardSpeedCmS<
            p.MinimumPhysicalSpeedCmS ||
        sample.MeasuredForwardSpeedCmS>
            p.MaximumTrustedSpeedCmS ||
        !Finite(sample.RoadFrictionScale) ||
        sample.RoadFrictionScale<p.MinimumFrictionScale ||
        sample.RoadFrictionScale>p.MaximumFrictionScale ||
        !Finite(sample.ActualWetnessFraction) ||
        sample.ActualWetnessFraction<0.0 ||
        sample.ActualWetnessFraction>
            p.MaximumWetnessFraction ||
        !Finite(sample.MeasurementConfidence) ||
        sample.MeasurementConfidence<
            p.MinimumSensorConfidence ||
        sample.MeasurementConfidence>1.0 ||
        !Finite(sample.MaximumPhysicalRoadGradeRadians) ||
        std::abs(sample.MaximumPhysicalRoadGradeRadians)>Pi/4.0 ||
        (sample.MeasuredMaterial!=ContactSurface::Asphalt &&
         sample.MeasuredMaterial!=ContactSurface::WetAsphalt))
        return false;
    double load=0.0;
    double maxLoad=0.0;
    double minLoad=std::numeric_limits<double>::infinity();
    for (const WheelObservation& wheel:sample.Wheels)
    {
        if (!ValidWheelObservation(wheel) ||
            (p.RequireActualWheelPhysics &&
             (!wheel.ContactVerified || !wheel.PhysicalContact)) ||
            !Finite(wheel.NormalLoadN) ||
            wheel.NormalLoadN<p.MinimumPhysicalWheelLoadN ||
            wheel.NormalLoadN>p.MaximumPhysicalWheelLoadN ||
            !Finite(wheel.SurfaceGrip) ||
            wheel.SurfaceGrip<p.MinimumRoadGrip ||
            wheel.SurfaceGrip>p.MaximumRoadGrip ||
            std::abs(wheel.SlipRatio)>
                p.MaximumTrustedSlipRatio ||
            std::abs(wheel.SlipAngleRadians)>
                p.MaximumTrustedSlipAngleRadians)
            return false;
        load+=wheel.NormalLoadN;
        minLoad=std::min(minLoad,wheel.NormalLoadN);
        maxLoad=std::max(maxLoad,wheel.NormalLoadN);
    }
    return load>=p.MinimumTotalNormalLoadN &&
        load<=p.MaximumTotalNormalLoadN &&
        SafeDivision(maxLoad-minLoad,maxLoad)<=
            p.MaximumLoadImbalance;
}

inline int GripAtlasIndex(
    double distanceCm,GripAtlasSettings settings) noexcept
{
    if (!Finite(distanceCm) ||
        distanceCm<0.0 ||
        distanceCm>=settings.CircuitLengthCm)
        return -1;
    const int result=static_cast<int>(
        std::floor(distanceCm/settings.CircuitLengthCm*
                   GripAtlasCells));
    return std::clamp(result,0,GripAtlasCells-1);
}

inline double MeasuredWheelGrip(
    const GripContactFrame& sample) noexcept
{
    double gripWeighted=0.0;
    double load=0.0;
    for (const WheelObservation& wheel:sample.Wheels)
    {
        gripWeighted+=wheel.SurfaceGrip*wheel.NormalLoadN;
        load+=wheel.NormalLoadN;
    }
    return SafeDivision(gripWeighted,load);
}

inline double MaximumMeasuredSlip(
    const GripContactFrame& sample) noexcept
{
    double severity=0.0;
    for (const WheelObservation& wheel:sample.Wheels)
        severity=std::max({severity,
            std::abs(wheel.SlipRatio),
            std::abs(wheel.SlipAngleRadians)});
    return severity;
}

class PhysicalSurfaceGripAtlas final
{
public:
    bool Configure(GripAtlasSettings settings) noexcept
    {
        if (!ValidGripAtlasSettings(settings))
            return false;
        Settings=settings;
        Clear();
        Configured=true;
        return true;
    }
    void Clear() noexcept
    {
        Cells={};
        Session=0;
        SessionRevision=0;
        Actor=0;
        Slot=-1;
        LastFrame=0;
        LastClock=0.0;
        Accepted=0;
        Rejected=0;
        Duplicates=0;
        WetTransitions=0;
        Started=false;
    }
    bool BeginSession(
        std::uint64_t session,std::uint32_t revision) noexcept
    {
        if (!Configured || session==0)
            return false;
        Clear();
        Session=session;
        SessionRevision=revision;
        Started=true;
        return true;
    }
    GripRecordResult Record(
        const GripContactFrame& frame) noexcept
    {
        GripRecordResult out;
        if (!Configured)
            return Reject(out,GripRecordFault::NotConfigured);
        if (!Started || frame.Identity.Session!=Session ||
            frame.Identity.SessionRevision!=SessionRevision)
            return Reject(out,GripRecordFault::MissingSession);
        if (!ValidGripContactFrame(frame,Settings))
            return Reject(out,GripRecordFault::InvalidWheel);
        if (Actor!=0 &&
            (Actor!=frame.Identity.Actor ||
             Slot!=frame.Identity.Slot) &&
            Settings.RejectActorChanges)
            return Reject(out,GripRecordFault::WrongActor);
        if (LastFrame!=0 && frame.Identity.Frame<=LastFrame)
        {
            ++Duplicates;
            return Reject(out,GripRecordFault::DuplicateFrame);
        }
        if (LastFrame!=0 &&
            (frame.RaceClockSeconds-LastClock<
                Settings.MinimumClockAdvanceSeconds ||
             frame.RaceClockSeconds-LastClock>
                Settings.MaximumClockJumpSeconds))
            return Reject(out,GripRecordFault::BadSessionClock);
        const int index=GripAtlasIndex(
            frame.DistanceAlongOriginalLapCm,Settings);
        if (index<0)
            return Reject(out,GripRecordFault::MissingRoad);
        const double grip=MeasuredWheelGrip(frame);
        const double slip=MaximumMeasuredSlip(frame);
        if (!Finite(grip) ||
            grip<Settings.MinimumRoadGrip ||
            grip>Settings.MaximumRoadGrip ||
            !Finite(slip))
            return Reject(out,GripRecordFault::BadFriction);
        GripCellEvidence& cell=Cells[
            static_cast<std::size_t>(index)];
        const double measured=std::clamp(
            grip*frame.RoadFrictionScale,
            Settings.MinimumRoadGrip,
            Settings.MaximumRoadGrip);
        double filtered=measured;
        if (cell.Samples>0)
        {
            if (measured<cell.MeasuredGrip)
            {
                // Sudden rain, oil or physical loss of tire traction must
                // lower the braking envelope immediately. Smoothing this
                // direction would grant unsafe grip that sensors no longer
                // support. Only a measured improvement can be filtered.
                filtered=measured;
            }
            else
            {
                const double elapsed=std::max(
                    Settings.MinimumClockAdvanceSeconds,
                    frame.RaceClockSeconds-cell.LastClockSeconds);
                const double maximumStep=
                    Settings.MaximumRoadVariationPerSecond*
                    elapsed+
                    0.10*std::abs(frame.ActualWetnessFraction-
                                  cell.WetnessFraction);
                const double permitted=std::min(
                    measured,cell.MeasuredGrip+maximumStep);
                filtered=ExponentialResponse(
                    cell.MeasuredGrip,permitted,
                    Settings.ExponentialFilterTimeSeconds,
                    frame.DeltaSeconds);
            }
        }
        const double previousMean=cell.MeanGrip;
        const std::uint64_t count=cell.Samples+1;
        const double delta=measured-previousMean;
        cell.MeanGrip+=delta/static_cast<double>(count);
        cell.VarianceNumerator+=
            delta*(measured-cell.MeanGrip);
        const bool wetChange=cell.Samples>0 &&
            ((cell.WetnessFraction>=
                Settings.WetnessHazardThreshold)!=(frame.ActualWetnessFraction>=
                Settings.WetnessHazardThreshold));
        cell.MinimumObservedGrip=cell.Samples==0 ?
            measured:std::min(cell.MinimumObservedGrip,measured);
        cell.MaximumObservedGrip=std::max(
            cell.MaximumObservedGrip,measured);
        cell.MinimumObservedWetness=cell.Samples==0 ?
            frame.ActualWetnessFraction:
            std::min(cell.MinimumObservedWetness,
                     frame.ActualWetnessFraction);
        cell.MaximumObservedWetness=std::max(
            cell.MaximumObservedWetness,frame.ActualWetnessFraction);
        cell.MeasuredGrip=filtered;
        cell.FilteredRoadFriction=ExponentialResponse(
            cell.FilteredRoadFriction,
            frame.RoadFrictionScale,
            Settings.ExponentialFilterTimeSeconds,
            frame.DeltaSeconds);
        if (cell.Samples==0)
            cell.FilteredRoadFriction=frame.RoadFrictionScale;
        cell.WetnessFraction=frame.ActualWetnessFraction;
        cell.LastClockSeconds=frame.RaceClockSeconds;
        cell.LastFrame=frame.Identity.Frame;
        cell.SourceActor=frame.Identity.Actor;
        cell.MostRecentMaterial=frame.MeasuredMaterial;
        cell.MaximumSlipRatio=std::max(
            cell.MaximumSlipRatio,slip);
        cell.MaximumSlipAngleRadians=std::max(
            cell.MaximumSlipAngleRadians,slip);
        double normal=0.0;
        for(const WheelObservation& wheel:frame.Wheels)
            normal+=wheel.NormalLoadN;
        cell.TotalNormalLoadN=normal;
        cell.MeanSensorConfidence+=
            (frame.MeasurementConfidence-
                cell.MeanSensorConfidence)/
            static_cast<double>(count);
        cell.Samples=count;
        cell.OnPhysicalRoad=true;
        cell.HasMeasuredContact=true;
        if (wetChange)++WetTransitions;
        Actor=frame.Identity.Actor;
        Slot=frame.Identity.Slot;
        LastFrame=frame.Identity.Frame;
        LastClock=frame.RaceClockSeconds;
        ++Accepted;
        out.Fault=GripRecordFault::None;
        out.CellIndex=index;
        out.NewGrip=cell.MeasuredGrip;
        out.MeasuredWetness=cell.WetnessFraction;
        out.MeasuredNormalLoadN=normal;
        out.Confidence=frame.MeasurementConfidence;
        out.MaximumSlip=slip;
        out.ChangedWetnessClass=wetChange;
        out.Qualified=true;
        out.AppliedToChaos=false;
        return out;
    }
    GripAtlasQuery Query(
        double distanceCm,double nowSeconds) const noexcept
    {
        GripAtlasQuery out;
        if (!Started ||
            !Finite(nowSeconds) ||
            nowSeconds<0.0)
            return out;
        const int index=GripAtlasIndex(
            distanceCm,Settings);
        if (index<0)
            return out;
        const GripCellEvidence& cell=
            Cells[static_cast<std::size_t>(index)];
        if (!cell.HasMeasuredContact ||
            cell.Samples<
                static_cast<std::uint64_t>(
                    Settings.MinimumCellObservations) ||
            nowSeconds<cell.LastClockSeconds)
            return out;
        const double age=nowSeconds-cell.LastClockSeconds;
        if (age>Settings.MaximumSensorAgeSeconds)
            return out;
        out.CellIndex=index;
        out.DistanceCm=distanceCm;
        out.MeasuredGripScale=cell.MeasuredGrip;
        out.MinimumMeasuredGrip=cell.MinimumObservedGrip;
        out.RoadFrictionScale=cell.FilteredRoadFriction;
        out.FilteredWetness=cell.WetnessFraction;
        out.MaximumObservedSlip=cell.MaximumSlipRatio;
        out.StatisticalVariance=cell.Samples>1 ?
            cell.VarianceNumerator/
                static_cast<double>(cell.Samples-1):0.0;
        out.Confidence=cell.MeanSensorConfidence*
            (1.0-Settings.ConfidenceVariancePenalty*
             Clamp01(SafeDivision(
                out.StatisticalVariance,
                Settings.MaximumStatisticalVariance)));
        out.SensorAgeSeconds=age;
        out.Samples=cell.Samples;
        out.Session=Session;
        out.Actor=cell.SourceActor;
        out.Material=cell.MostRecentMaterial;
        out.HazardousLowGrip=
            out.MeasuredGripScale<
            Settings.HazardGripThreshold;
        out.StandingWaterHazard=
            out.FilteredWetness>=
            Settings.WetnessHazardThreshold;
        out.ProvenOriginalRoad=cell.OnPhysicalRoad;
        out.PhysicsObserved=cell.HasMeasuredContact;
        out.ReadyForPlanning=
            out.Confidence>=Settings.MinimumSensorConfidence &&
            out.ProvenOriginalRoad && out.PhysicsObserved;
        out.Valid=Finite(out.MeasuredGripScale) &&
            Finite(out.Confidence);
        return out;
    }
    bool ApplyToRoute(
        CornerForecastObservation& target,
        double vehicleTrackDistanceCm,
        double raceClockSeconds) const noexcept
    {
        if (!Started || target.KnotCount<2 ||
            target.KnotCount>CornerForecastCapacity ||
            target.Identity.Session!=Session ||
            target.Identity.SessionRevision!=SessionRevision ||
            target.Identity.Actor!=Actor ||
            !Finite(vehicleTrackDistanceCm) ||
            vehicleTrackDistanceCm<0.0 ||
            vehicleTrackDistanceCm>=Settings.CircuitLengthCm)
            return false;
        std::array<double,CornerForecastCapacity> evidence{};
        for(int i=0;i<target.KnotCount;++i)
        {
            auto& knot=target.Route[
                static_cast<std::size_t>(i)];
            if (!knot.OriginalAsphaltSample ||
                !knot.ActualGeometryQualified ||
                !Finite(knot.AlongRouteCm) ||
                knot.AlongRouteCm<0.0 ||
                knot.AlongRouteCm>
                    Settings.MaximumPredictionDistanceCm)
                return false;
            const double distance=WrapDistance(
                vehicleTrackDistanceCm+
                    knot.AlongRouteCm,
                Settings.CircuitLengthCm);
            const GripAtlasQuery observed=
                Query(distance,raceClockSeconds);
            if (!observed.Valid ||
                !observed.ReadyForPlanning)
                return false;
            evidence[static_cast<std::size_t>(i)]=
                observed.MeasuredGripScale;
        }
        // Apply atomically only after ALL route cells were certified.
        // Never boost route grip beyond a fresh physical sensor reading.
        for(int i=0;i<target.KnotCount;++i)
        {
            auto& knot=target.Route[
                static_cast<std::size_t>(i)];
            knot.MeasuredFrictionScale=std::min(
                knot.MeasuredFrictionScale,
                evidence[static_cast<std::size_t>(i)]);
            knot.RealFrictionQualified=true;
        }
        return true;
    }
    GripRoadAheadReview ReviewRoadAhead(
        double startDistanceCm,double lookaheadCm,
        double nowSeconds) const noexcept
    {
        GripRoadAheadReview out;
        if (!Started ||
            !Finite(startDistanceCm) ||
            startDistanceCm<0.0 ||
            startDistanceCm>=Settings.CircuitLengthCm ||
            !Finite(lookaheadCm) ||
            lookaheadCm<=0.0 ||
            lookaheadCm>Settings.MaximumPredictionDistanceCm ||
            !Finite(nowSeconds) ||
            nowSeconds<LastClock)
            return out;
        out.StartingDistanceCm=startDistanceCm;
        out.LookaheadCm=lookaheadCm;
        out.MinimumMeasuredGrip=
            std::numeric_limits<double>::infinity();
        out.MinimumConfidence=1.0;
        out.NearestPhysicalHazardCm=
            std::numeric_limits<double>::infinity();
        const double cellWidth=
            Settings.CircuitLengthCm/GripAtlasCells;
        // Traverse actual occupied spatial buckets; never fill unmeasured
        // gaps by extrapolating the previous good wet or dry sample.
        double distance=0.0;
        int index=GripAtlasIndex(startDistanceCm,Settings);
        if (index<0)return out;
        for(int traversed=0;
            traversed<=GripAtlasCells &&
            distance<lookaheadCm;
            ++traversed)
        {
            const double cellStart=
                static_cast<double>(index)*cellWidth;
            const double cellEnd=cellStart+cellWidth;
            const double position=WrapDistance(
                startDistanceCm+distance,
                Settings.CircuitLengthCm);
            const double remainingInCell=std::max(
                0.0,cellEnd-position);
            const double extent=std::min(
                lookaheadCm-distance,
                remainingInCell);
            const double queryDistance=std::min(
                Settings.CircuitLengthCm-PhysicsEpsilon,
                cellStart+cellWidth*0.5);
            const GripAtlasQuery evidence=
                Query(queryDistance,nowSeconds);
            ++out.RoadCellsEncountered;
            if (evidence.Valid &&
                evidence.ReadyForPlanning)
            {
                ++out.VerifiedCells;
                out.VerifiedAheadCm+=extent;
                out.MinimumMeasuredGrip=std::min(
                    out.MinimumMeasuredGrip,
                    evidence.MeasuredGripScale);
                out.MaximumMeasuredWetness=std::max(
                    out.MaximumMeasuredWetness,
                    evidence.FilteredWetness);
                out.MinimumConfidence=std::min(
                    out.MinimumConfidence,
                    evidence.Confidence);
                out.OldestEvidenceSeconds=std::max(
                    out.OldestEvidenceSeconds,
                    evidence.SensorAgeSeconds);
                if (evidence.HazardousLowGrip ||
                    evidence.StandingWaterHazard)
                {
                    ++out.HazardCells;
                    out.LowGripHazardSeen |=
                        evidence.HazardousLowGrip;
                    out.WetHazardSeen |=
                        evidence.StandingWaterHazard;
                    if (distance<
                        out.NearestPhysicalHazardCm)
                    {
                        out.NearestPhysicalHazardCm=distance;
                        out.NearestHazardCell=index;
                    }
                }
            }
            else
            {
                ++out.UnknownCells;
                out.UnknownAheadCm+=extent;
            }
            // Advance to the next physical cell. A zero-length portion
            // at a floating-point boundary still progresses one cell.
            distance+=extent;
            if (extent<=PhysicsEpsilon)
                distance+=std::min(
                    PhysicsEpsilon*1000.0,
                    lookaheadCm-distance);
            index=(index+1)%GripAtlasCells;
        }
        if (out.VerifiedCells==0)
        {
            out.MinimumMeasuredGrip=0.0;
            out.MinimumConfidence=0.0;
        }
        out.FullRoadEvidence=
            out.UnknownCells==0 &&
            out.VerifiedCells>0 &&
            out.VerifiedAheadCm>=lookaheadCm-
                PhysicsEpsilon*1000.0;
        out.Valid=Finite(out.MinimumMeasuredGrip) &&
            Finite(out.MaximumMeasuredWetness) &&
            Finite(out.UnknownAheadCm) &&
            Finite(out.VerifiedAheadCm);
        return out;
    }
    GripAtlasProgress Progress(
        double nowSeconds) const noexcept
    {
        GripAtlasProgress out;
        out.QualifiedMeasurements=Accepted;
        out.RejectedMeasurements=Rejected;
        out.DuplicateFrames=Duplicates;
        out.RealWetTransitions=WetTransitions;
        double totalConfidence=0.0;
        out.MinimumConfirmedGrip=
            std::numeric_limits<double>::infinity();
        for(const GripCellEvidence& cell:Cells)
        {
            if (!cell.HasMeasuredContact ||
                cell.Samples<
                    static_cast<std::uint64_t>(
                        Settings.MinimumCellObservations))
                continue;
            ++out.ConfirmedCells;
            out.MinimumConfirmedGrip=std::min(
                out.MinimumConfirmedGrip,
                cell.MeasuredGrip);
            out.MaximumConfirmedGrip=std::max(
                out.MaximumConfirmedGrip,
                cell.MeasuredGrip);
            totalConfidence+=cell.MeanSensorConfidence;
            if (cell.MeasuredGrip<
                Settings.HazardGripThreshold)
                ++out.HazardousCells;
            if (Finite(nowSeconds) &&
                nowSeconds>=cell.LastClockSeconds &&
                nowSeconds-cell.LastClockSeconds<=
                    Settings.MaximumSensorAgeSeconds)
                ++out.FreshCells;
        }
        if (out.ConfirmedCells==0)
            out.MinimumConfirmedGrip=0.0;
        out.PhysicalCoverageFraction=
            static_cast<double>(out.ConfirmedCells)/
            GripAtlasCells;
        out.AverageSensorConfidence=SafeDivision(
            totalConfidence,
            static_cast<double>(out.ConfirmedCells));
        out.CoverageIsMeasuredOnly=true;
        return out;
    }
    GripCellEvidence Cell(int index) const noexcept
    {
        return index>=0 && index<GripAtlasCells ?
            Cells[static_cast<std::size_t>(index)]:
            GripCellEvidence{};
    }
    std::uint64_t QualifiedFrames() const noexcept
    {return Accepted;}
    std::uint64_t RejectedFrames() const noexcept
    {return Rejected;}
    std::uint64_t CurrentSession() const noexcept
    {return Session;}
    int TrackedActorSlot() const noexcept {return Slot;}
private:
    GripRecordResult Reject(
        GripRecordResult result,GripRecordFault fault) noexcept
    {
        ++Rejected;
        result.Fault=fault;
        return result;
    }
    GripAtlasSettings Settings{};
    std::array<GripCellEvidence,GripAtlasCells> Cells{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t LastFrame=0;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t Duplicates=0;
    std::uint64_t WetTransitions=0;
    std::uint32_t SessionRevision=0;
    double LastClock=0.0;
    int Slot=-1;
    bool Configured=false;
    bool Started=false;
};
} // namespace latebrake::racecraft
