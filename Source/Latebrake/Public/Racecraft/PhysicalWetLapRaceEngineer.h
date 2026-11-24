#pragma once

#include "Racecraft/PhysicalWetLapTrace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Race-engineer REVIEW of physical six-car road/water trace data.
// It aggregates only trusted native frames already stored in the trace
// journal. No artificial lap times, sector rewards, brake commands,
// AI steering, racing-line teleportation or causal wet-grip claim.
enum class WetLapEngineerFault : std::uint8_t
{
    None,
    InvalidPolicy,
    TooFewObservedFrames,
    BadSourceFrame,
    BrokenNativeContinuity,
    SourceRosterChanged,
    NonfiniteStatistics
};

enum class WetLapEngineerCue : std::uint8_t
{
    Unqualified,
    NormalObservedDriving,
    WetRoadAttention,
    StandingWaterCaution,
    StrongMeasuredLateralSlip,
    RepeatedWetBraking
};

enum class WetLapObservedSurface : std::uint8_t
{
    Dry,
    Damp,
    Wet,
    StandingWater
};

struct WetLapEngineerSettings final
{
    unsigned MinimumSourceFrames=3u;
    unsigned MaximumSourceFrames=WetLapTraceCapacity;
    unsigned MinimumWetBrakingFramesForCue=2u;
    unsigned MinimumSourceSectorVisitsForPace=2u;
    double MinimumElapsedSeconds=0.01;
    double MaximumElapsedSeconds=10.0;
    double MaximumFrameGapSeconds=0.10;
    double MinimumFrameGapSeconds=0.001;
    double WetRoadThresholdMm=0.5;
    double StandingWaterThresholdMm=3.0;
    double MinimumActualBrakeFractionForCue=0.35;
    double MinimumActualThrottleFractionForCue=0.45;
    double StrongSlipRadians=0.20;
    double MaximumAcceptedSlipRadians=WetLapTracePi;
    double MaximumSpeedCmps=PhysicalReplayMaximumSpeedCmps;
    double MinimumGripFraction=0.2;
    double MaximumGripFraction=1.0;
    double MaximumSourceClockSkewSeconds=0.02;
    bool RequirePhysicalCarAndRoadWitnesses=true;
    bool RequireOriginalNativeSessionIdentity=true;
    bool NeverTreatStatisticsAsOfficialLapTime=true;
    bool NeverInferRainFromVehicleSpeed=true;
    bool NeverChangeVehicleControlInputs=true;
    bool NeverSuggestUnverifiedCollisionForce=true;
};

struct WetLapEngineerSectorSummary final
{
    unsigned SourceSector=0u;
    unsigned PhysicalSourceSamples=0u;
    unsigned WetSourceSamples=0u;
    unsigned StandingSourceSamples=0u;
    unsigned ObservedBrakingSamples=0u;
    double ObservedSeconds=0.0;
    double WetRoadSeconds=0.0;
    double DryRoadSeconds=0.0;
    double WetSpeedIntegralCm=0.0;
    double DrySpeedIntegralCm=0.0;
    double MinimumMeasuredGrip=1.0;
    double PeakMeasuredWaterMm=0.0;
    double PeakObservedSpeedCmps=0.0;
    double SpeedIntegralCm=0.0;
    double MeanObservedSpeedCmps=0.0;
    bool ActualTimingGateNotVerified=true;
    bool NoSectorRewardIssued=true;
};

struct WetLapEngineerCarSummary final
{
    bool Valid=false;
    std::uint64_t Actor=0u;
    unsigned SourceFrameCount=0u;
    unsigned SourceWetSamples=0u;
    unsigned SourceDrySamples=0u;
    unsigned SourceStandingSamples=0u;
    unsigned BrakeWitnessSamples=0u;
    unsigned ThrottleWitnessSamples=0u;
    unsigned StrongSlipWitnessSamples=0u;
    unsigned CrossedSourceSectorBoundaries=0u;
    unsigned FullyQualifiedSourceSectors=0u;
    double ObservedSeconds=0.0;
    double WetSeconds=0.0;
    double StandingWaterSeconds=0.0;
    double BrakeInputSeconds=0.0;
    double ThrottleInputSeconds=0.0;
    double HighSlipSeconds=0.0;
    double MeasuredTravelDistanceCm=0.0;
    double RouteProgressDistanceCm=0.0;
    double MinimumMeasuredGrip=1.0;
    double MaximumMeasuredGrip=0.0;
    double MaximumWaterFilmMm=0.0;
    double MaximumObservedSlipRadians=0.0;
    double PeakObservedSpeedCmps=0.0;
    double MinimumObservedSpeedCmps=0.0;
    double TimeIntegratedSpeedCm=0.0;
    double MeanObservedSpeedCmps=0.0;
    double MeanWetSpeedCmps=0.0;
    double MeanDrySpeedCmps=0.0;
    double MaximumRecordedBrakeInput=0.0;
    double MaximumRecordedThrottleInput=0.0;
    double SourceFractionWet=0.0;
    double SourceFractionBraking=0.0;
    WetLapEngineerCue Cue=WetLapEngineerCue::Unqualified;
    std::array<WetLapEngineerSectorSummary,
        WetLapTraceSectorCount> Sectors{};
    bool NoNativeDrivingControlChanged=true;
    bool NoOfficialLapRecordIssued=true;
};

struct WetLapEngineerFleetReport final
{
    bool Valid=false;
    WetLapEngineerFault Fault=WetLapEngineerFault::TooFewObservedFrames;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t FirstSourceFrame=0u;
    std::uint64_t LastSourceFrame=0u;
    double FirstSourceClockSeconds=0.0;
    double LastSourceClockSeconds=0.0;
    double MeasuredSourceSeconds=0.0;
    unsigned PhysicalFramesReviewed=0u;
    unsigned QualifiedPhysicalCars=0u;
    unsigned WetCarsObserved=0u;
    unsigned StandingCarsObserved=0u;
    unsigned StrongSlipCarsObserved=0u;
    unsigned WetBrakingCarsObserved=0u;
    double TotalWetVehicleSeconds=0.0;
    double TotalStandingWaterVehicleSeconds=0.0;
    double TotalPhysicalTravelCm=0.0;
    double MaximumObservedWetnessMm=0.0;
    double MinimumMeasuredPhysicalGrip=1.0;
    double PeakObservedChassisSpeedCmps=0.0;
    double MeanFleetObservedSpeedCmps=0.0;
    std::array<WetLapEngineerCarSummary,WetLapTraceCarCount> Cars{};
    bool PhysicalRaceAuthorityUnchanged=true;
    bool NoOfficialClassificationIssued=true;
    bool SourceOnly=true;
};

struct WetLapEngineerComparison final
{
    bool Valid=false;
    std::uint64_t Session=0u;
    std::uint64_t EarlierLastSourceFrame=0u;
    std::uint64_t LaterLastSourceFrame=0u;
    double MeanFleetSpeedChangeCmps=0.0;
    double MaximumObservedWaterChangeMm=0.0;
    double MinimumGripChange=0.0;
    double WetVehicleSecondsChange=0.0;
    unsigned MoreWetCars=0u;
    unsigned HigherSlipCars=0u;
    unsigned SlowerMeanSpeedCars=0u;
    unsigned FasterMeanSpeedCars=0u;
    unsigned SimilarMeanSpeedCars=0u;
    bool ObservationalComparisonOnly=true;
    bool CausalityNotEstablished=true;
    bool NoOfficialTimingAuthority=true;
};

inline bool ValidWetLapEngineerSettings(
    WetLapEngineerSettings settings) noexcept
{
    return settings.MinimumSourceFrames>=2u &&
        settings.MinimumSourceFrames<=8u &&
        settings.MaximumSourceFrames>=settings.MinimumSourceFrames &&
        settings.MaximumSourceFrames<=WetLapTraceCapacity &&
        settings.MinimumWetBrakingFramesForCue>=1u &&
        settings.MinimumWetBrakingFramesForCue<=8u &&
        settings.MinimumSourceSectorVisitsForPace>=1u &&
        settings.MinimumSourceSectorVisitsForPace<=8u &&
        std::isfinite(settings.MinimumElapsedSeconds) &&
        settings.MinimumElapsedSeconds>=0.001 &&
        settings.MinimumElapsedSeconds<=0.05 &&
        std::isfinite(settings.MaximumElapsedSeconds) &&
        settings.MaximumElapsedSeconds>=1.0 &&
        settings.MaximumElapsedSeconds<=30.0 &&
        settings.MaximumElapsedSeconds>settings.MinimumElapsedSeconds &&
        std::isfinite(settings.MaximumFrameGapSeconds) &&
        settings.MaximumFrameGapSeconds>=0.05 &&
        settings.MaximumFrameGapSeconds<=0.10 &&
        std::isfinite(settings.MinimumFrameGapSeconds) &&
        settings.MinimumFrameGapSeconds>=0.001 &&
        settings.MinimumFrameGapSeconds<=0.01 &&
        settings.MaximumFrameGapSeconds>
            settings.MinimumFrameGapSeconds &&
        std::isfinite(settings.WetRoadThresholdMm) &&
        settings.WetRoadThresholdMm>=0.05 &&
        settings.WetRoadThresholdMm<=1.0 &&
        std::isfinite(settings.StandingWaterThresholdMm) &&
        settings.StandingWaterThresholdMm>=2.0 &&
        settings.StandingWaterThresholdMm<=6.0 &&
        settings.StandingWaterThresholdMm>
            settings.WetRoadThresholdMm &&
        std::isfinite(settings.MinimumActualBrakeFractionForCue) &&
        settings.MinimumActualBrakeFractionForCue>=0.1 &&
        settings.MinimumActualBrakeFractionForCue<=0.6 &&
        std::isfinite(settings.MinimumActualThrottleFractionForCue) &&
        settings.MinimumActualThrottleFractionForCue>=0.2 &&
        settings.MinimumActualThrottleFractionForCue<=0.7 &&
        std::isfinite(settings.StrongSlipRadians) &&
        settings.StrongSlipRadians>=0.1 &&
        settings.StrongSlipRadians<=0.5 &&
        std::isfinite(settings.MaximumAcceptedSlipRadians) &&
        settings.MaximumAcceptedSlipRadians>=2.0 &&
        settings.MaximumAcceptedSlipRadians<=WetLapTracePi &&
        std::isfinite(settings.MaximumSpeedCmps) &&
        settings.MaximumSpeedCmps>=10000.0 &&
        settings.MaximumSpeedCmps<=PhysicalReplayMaximumSpeedCmps &&
        std::isfinite(settings.MinimumGripFraction) &&
        settings.MinimumGripFraction>=0.1 &&
        settings.MinimumGripFraction<=0.4 &&
        std::isfinite(settings.MaximumGripFraction) &&
        settings.MaximumGripFraction>=0.8 &&
        settings.MaximumGripFraction<=1.0 &&
        settings.MinimumGripFraction<settings.MaximumGripFraction &&
        std::isfinite(settings.MaximumSourceClockSkewSeconds) &&
        settings.MaximumSourceClockSkewSeconds>=0.0 &&
        settings.MaximumSourceClockSkewSeconds<=0.05 &&
        settings.RequirePhysicalCarAndRoadWitnesses &&
        settings.RequireOriginalNativeSessionIdentity &&
        settings.NeverTreatStatisticsAsOfficialLapTime &&
        settings.NeverInferRainFromVehicleSpeed &&
        settings.NeverChangeVehicleControlInputs &&
        settings.NeverSuggestUnverifiedCollisionForce;
}

inline bool ValidWetLapEngineerTraceCar(
    const WetLapTraceCar& car,
    WetLapEngineerSettings p) noexcept
{
    return car.Valid &&
        car.Actor!=0u &&
        car.NoNativeRaceAuthority &&
        car.NoWheelContactModified &&
        std::isfinite(car.WrappedStationCm) &&
        std::isfinite(car.SignedLateralCm) &&
        std::isfinite(car.WorldXcm) &&
        std::isfinite(car.WorldYcm) &&
        std::isfinite(car.WorldZcm) &&
        std::isfinite(car.PlanarSpeedCmps) &&
        std::isfinite(car.Speed3dCmps) &&
        std::isfinite(car.LateralSlipRadians) &&
        std::isfinite(car.ActualBrakeFraction) &&
        std::isfinite(car.ActualThrottleFraction) &&
        std::isfinite(car.ActualGripFraction) &&
        std::isfinite(car.ActualWaterFilmMm) &&
        car.WrappedStationCm>=0.0 &&
        car.PlanarSpeedCmps>=0.0 &&
        car.Speed3dCmps>=car.PlanarSpeedCmps-1.0e-5 &&
        car.Speed3dCmps<=p.MaximumSpeedCmps &&
        std::abs(car.LateralSlipRadians)<=
            p.MaximumAcceptedSlipRadians &&
        car.ActualBrakeFraction>=0.0 &&
        car.ActualBrakeFraction<=1.0 &&
        car.ActualThrottleFraction>=0.0 &&
        car.ActualThrottleFraction<=1.0 &&
        car.ActualGripFraction>=p.MinimumGripFraction &&
        car.ActualGripFraction<=p.MaximumGripFraction &&
        car.ActualWaterFilmMm>=0.0 &&
        car.ActualWaterFilmMm<=ObservedWaterMaximumMm &&
        car.SourceRoadSector<WetLapTraceSectorCount;
}

inline WetLapObservedSurface ClassifyObservedWetLapSurface(
    double measuredFilmMm,
    WetLapEngineerSettings p={}) noexcept
{
    if(!std::isfinite(measuredFilmMm) ||
       measuredFilmMm<0.0 ||
       measuredFilmMm>ObservedWaterMaximumMm)
        return WetLapObservedSurface::Dry;
    if(measuredFilmMm>=p.StandingWaterThresholdMm)
        return WetLapObservedSurface::StandingWater;
    if(measuredFilmMm>=p.WetRoadThresholdMm)
        return WetLapObservedSurface::Wet;
    if(measuredFilmMm>=0.05)
        return WetLapObservedSurface::Damp;
    return WetLapObservedSurface::Dry;
}

inline WetLapEngineerCue ClassifyObservedWetLapCue(
    const WetLapEngineerCarSummary& car,
    WetLapEngineerSettings settings={}) noexcept
{
    if(!car.Valid ||
       car.SourceFrameCount<settings.MinimumSourceFrames)
        return WetLapEngineerCue::Unqualified;
    if(car.StandingWaterSeconds>0.0 ||
       car.SourceStandingSamples>0u)
        return WetLapEngineerCue::StandingWaterCaution;
    if(car.MaximumObservedSlipRadians>=settings.StrongSlipRadians ||
       car.StrongSlipWitnessSamples>0u)
        return WetLapEngineerCue::StrongMeasuredLateralSlip;
    if(car.SourceWetSamples>0u &&
       car.BrakeWitnessSamples>=
           settings.MinimumWetBrakingFramesForCue)
        return WetLapEngineerCue::RepeatedWetBraking;
    if(car.WetSeconds>0.0)
        return WetLapEngineerCue::WetRoadAttention;
    return WetLapEngineerCue::NormalObservedDriving;
}

inline WetLapEngineerFleetReport BuildPhysicalWetLapRaceEngineer(
    const PhysicalWetLapTraceJournal& journal,
    WetLapEngineerSettings settings={}) noexcept
{
    WetLapEngineerFleetReport out;
    if(!ValidWetLapEngineerSettings(settings))
    {
        out.Fault=WetLapEngineerFault::InvalidPolicy;
        return out;
    }
    const std::size_t count=journal.Count();
    if(count<settings.MinimumSourceFrames ||
       count>settings.MaximumSourceFrames)
        return out;
    WetLapTraceFrame first;
    if(!journal.Read(0u,first) ||
       !first.Valid ||
       first.VerifiedRoadBodies!=WetLapTraceCarCount ||
       !first.NativeChaosUnchanged ||
       !first.NoOfficialLapTimeChange)
    {
        out.Fault=WetLapEngineerFault::BadSourceFrame;
        return out;
    }
    out.Circuit=first.Circuit;
    out.Session=first.Session;
    out.Revision=first.Revision;
    out.FirstSourceFrame=first.SourceFrame;
    out.FirstSourceClockSeconds=first.SourceClockSeconds;
    out.MinimumMeasuredPhysicalGrip=1.0;
    for(unsigned i=0u;i<WetLapTraceCarCount;++i)
    {
        auto& result=out.Cars[i];
        result.Actor=first.Cars[i].Actor;
        result.MinimumMeasuredGrip=1.0;
        result.MinimumObservedSpeedCmps=
            std::numeric_limits<double>::infinity();
        for(unsigned j=0u;j<WetLapTraceSectorCount;++j)
        {
            result.Sectors[j].SourceSector=j;
            result.Sectors[j].MinimumMeasuredGrip=1.0;
        }
    }

    WetLapTraceFrame previous=first;
    for(std::size_t n=0u;n<count;++n)
    {
        WetLapTraceFrame frame;
        if(!journal.Read(n,frame) ||
           !frame.Valid ||
           !frame.SourceOnly ||
           !frame.NativeChaosUnchanged ||
           !frame.NoOfficialLapTimeChange ||
           frame.VerifiedRoadBodies!=WetLapTraceCarCount ||
           frame.Cars.size()!=WetLapTraceCarCount)
        {
            out.Fault=WetLapEngineerFault::BadSourceFrame;
            return out;
        }
        if(frame.Circuit!=out.Circuit ||
           frame.Session!=out.Session ||
           frame.Revision!=out.Revision ||
           frame.CircuitLapLengthCm!=first.CircuitLapLengthCm)
        {
            out.Fault=WetLapEngineerFault::SourceRosterChanged;
            return out;
        }
        double elapsed=0.0;
        if(n>0u)
        {
            elapsed=frame.SourceClockSeconds-
                previous.SourceClockSeconds;
            if(!std::isfinite(elapsed) ||
               elapsed<settings.MinimumFrameGapSeconds ||
               elapsed>settings.MaximumFrameGapSeconds)
            {
                out.Fault=WetLapEngineerFault::BrokenNativeContinuity;
                return out;
            }
            WetLapTraceSettings continuityPolicy;
            continuityPolicy.MaximumFrameIntervalSeconds=
                settings.MaximumFrameGapSeconds;
            if(!WetLapTracePlausibleContinuity(
                previous,frame,continuityPolicy))
            {
                out.Fault=WetLapEngineerFault::BrokenNativeContinuity;
                return out;
            }
        }
        for(unsigned i=0u;i<WetLapTraceCarCount;++i)
        {
            const auto& car=frame.Cars[i];
            auto& result=out.Cars[i];
            if(!ValidWetLapEngineerTraceCar(car,settings) ||
               result.Actor!=car.Actor)
            {
                out.Fault=WetLapEngineerFault::SourceRosterChanged;
                return out;
            }
            auto& sector=result.Sectors[car.SourceRoadSector];
            ++sector.PhysicalSourceSamples;
            ++result.SourceFrameCount;
            result.MaximumMeasuredGrip=std::max(
                result.MaximumMeasuredGrip,car.ActualGripFraction);
            result.MinimumMeasuredGrip=std::min(
                result.MinimumMeasuredGrip,car.ActualGripFraction);
            result.MaximumWaterFilmMm=std::max(
                result.MaximumWaterFilmMm,car.ActualWaterFilmMm);
            result.MaximumObservedSlipRadians=std::max(
                result.MaximumObservedSlipRadians,
                std::abs(car.LateralSlipRadians));
            result.PeakObservedSpeedCmps=std::max(
                result.PeakObservedSpeedCmps,car.Speed3dCmps);
            result.MinimumObservedSpeedCmps=std::min(
                result.MinimumObservedSpeedCmps,car.Speed3dCmps);
            result.MaximumRecordedBrakeInput=std::max(
                result.MaximumRecordedBrakeInput,car.ActualBrakeFraction);
            result.MaximumRecordedThrottleInput=std::max(
                result.MaximumRecordedThrottleInput,
                car.ActualThrottleFraction);
            sector.MinimumMeasuredGrip=std::min(
                sector.MinimumMeasuredGrip,car.ActualGripFraction);
            sector.PeakMeasuredWaterMm=std::max(
                sector.PeakMeasuredWaterMm,car.ActualWaterFilmMm);
            sector.PeakObservedSpeedCmps=std::max(
                sector.PeakObservedSpeedCmps,car.Speed3dCmps);
            out.MaximumObservedWetnessMm=std::max(
                out.MaximumObservedWetnessMm,car.ActualWaterFilmMm);
            out.MinimumMeasuredPhysicalGrip=std::min(
                out.MinimumMeasuredPhysicalGrip,car.ActualGripFraction);
            out.PeakObservedChassisSpeedCmps=std::max(
                out.PeakObservedChassisSpeedCmps,car.Speed3dCmps);
            const auto surface=ClassifyObservedWetLapSurface(
                car.ActualWaterFilmMm,settings);
            const bool wet=surface==WetLapObservedSurface::Wet ||
                surface==WetLapObservedSurface::StandingWater;
            const bool standing=
                surface==WetLapObservedSurface::StandingWater;
            const bool braking=car.ActualBrakeFraction>=
                settings.MinimumActualBrakeFractionForCue;
            const bool throttle=car.ActualThrottleFraction>=
                settings.MinimumActualThrottleFractionForCue;
            const bool slipping=std::abs(car.LateralSlipRadians)>=
                settings.StrongSlipRadians;
            if(wet)
            {
                ++result.SourceWetSamples;
                ++sector.WetSourceSamples;
            }else
                ++result.SourceDrySamples;
            if(standing)
            {
                ++result.SourceStandingSamples;
                ++sector.StandingSourceSamples;
            }
            if(braking)
            {
                ++result.BrakeWitnessSamples;
                ++sector.ObservedBrakingSamples;
            }
            if(throttle)++result.ThrottleWitnessSamples;
            if(slipping)++result.StrongSlipWitnessSamples;
            if(n==0u)continue;
            const auto& before=previous.Cars[i];
            const double distance=std::hypot(
                std::hypot(car.WorldXcm-before.WorldXcm,
                           car.WorldYcm-before.WorldYcm),
                car.WorldZcm-before.WorldZcm);
            const double routeGap=WetLapTraceWrappedStationDifference(
                before.WrappedStationCm,car.WrappedStationCm,
                frame.CircuitLapLengthCm);
            if(!std::isfinite(distance) ||
               !std::isfinite(routeGap))
            {
                out.Fault=WetLapEngineerFault::NonfiniteStatistics;
                return out;
            }
            result.ObservedSeconds+=elapsed;
            result.TimeIntegratedSpeedCm+=
                elapsed*0.5*(car.Speed3dCmps+before.Speed3dCmps);
            result.MeasuredTravelDistanceCm+=distance;
            result.RouteProgressDistanceCm+=routeGap;
            sector.ObservedSeconds+=elapsed;
            const double observedSpeedIntegral=
                elapsed*0.5*(car.Speed3dCmps+before.Speed3dCmps);
            sector.SpeedIntegralCm+=observedSpeedIntegral;
            if(car.SourceRoadSector!=before.SourceRoadSector)
                ++result.CrossedSourceSectorBoundaries;
            if(wet)
            {
                result.WetSeconds+=elapsed;
                sector.WetRoadSeconds+=elapsed;
                sector.WetSpeedIntegralCm+=observedSpeedIntegral;
            }else
            {
                sector.DryRoadSeconds+=elapsed;
                sector.DrySpeedIntegralCm+=observedSpeedIntegral;
            }
            if(standing)
                result.StandingWaterSeconds+=elapsed;
            if(braking)
                result.BrakeInputSeconds+=elapsed;
            if(throttle)
                result.ThrottleInputSeconds+=elapsed;
            if(slipping)
                result.HighSlipSeconds+=elapsed;
        }
        ++out.PhysicalFramesReviewed;
        previous=frame;
    }
    out.LastSourceFrame=previous.SourceFrame;
    out.LastSourceClockSeconds=previous.SourceClockSeconds;
    out.MeasuredSourceSeconds=out.LastSourceClockSeconds-
        out.FirstSourceClockSeconds;
    if(!std::isfinite(out.MeasuredSourceSeconds) ||
       out.MeasuredSourceSeconds<settings.MinimumElapsedSeconds ||
       out.MeasuredSourceSeconds>settings.MaximumElapsedSeconds)
    {
        out.Fault=WetLapEngineerFault::NonfiniteStatistics;
        return out;
    }
    for(auto& car:out.Cars)
    {
        if(car.ObservedSeconds<=0.0 ||
           car.SourceFrameCount!=out.PhysicalFramesReviewed)
        {
            out.Fault=WetLapEngineerFault::NonfiniteStatistics;
            return out;
        }
        car.MeanObservedSpeedCmps=
            car.TimeIntegratedSpeedCm/car.ObservedSeconds;
        car.SourceFractionWet=
            car.WetSeconds/car.ObservedSeconds;
        car.SourceFractionBraking=
            car.BrakeInputSeconds/car.ObservedSeconds;
        // Compute wet and dry speed means from their OWN physical
        // source intervals, never by redistributing a sector average
        // across both weather conditions (which would hide differences).
        double wetSpeedIntegral=0.0;
        double drySpeedIntegral=0.0;
        double weightedWetTime=0.0;
        double weightedDryTime=0.0;
        for(auto& sector:car.Sectors)
        {
            if(sector.PhysicalSourceSamples>=
               settings.MinimumSourceSectorVisitsForPace)
                ++car.FullyQualifiedSourceSectors;
            if(sector.ObservedSeconds>0.0)
                sector.MeanObservedSpeedCmps=
                    sector.SpeedIntegralCm/sector.ObservedSeconds;
            wetSpeedIntegral+=sector.WetSpeedIntegralCm;
            drySpeedIntegral+=sector.DrySpeedIntegralCm;
            weightedWetTime+=sector.WetRoadSeconds;
            weightedDryTime+=sector.DryRoadSeconds;
        }
        if(weightedWetTime>0.0)
            car.MeanWetSpeedCmps=
                wetSpeedIntegral/weightedWetTime;
        if(weightedDryTime>0.0)
            car.MeanDrySpeedCmps=
                drySpeedIntegral/weightedDryTime;
        // These are observational grouping averages, not a claim
        // that water caused a difference in driver speed.
        car.Valid=std::isfinite(car.MeanObservedSpeedCmps) &&
            std::isfinite(car.SourceFractionWet) &&
            std::isfinite(car.SourceFractionBraking) &&
            car.MeanObservedSpeedCmps>=0.0 &&
            car.SourceFractionWet>=0.0 &&
            car.SourceFractionWet<=1.0+1e-9 &&
            car.SourceFractionBraking>=0.0 &&
            car.SourceFractionBraking<=1.0+1e-9 &&
            car.MeasuredTravelDistanceCm>=0.0 &&
            car.RouteProgressDistanceCm>=0.0;
        if(!car.Valid)
        {
            out.Fault=WetLapEngineerFault::NonfiniteStatistics;
            return out;
        }
        car.Cue=ClassifyObservedWetLapCue(car,settings);
        ++out.QualifiedPhysicalCars;
        out.TotalPhysicalTravelCm+=car.MeasuredTravelDistanceCm;
        out.TotalWetVehicleSeconds+=car.WetSeconds;
        out.TotalStandingWaterVehicleSeconds+=
            car.StandingWaterSeconds;
        out.MeanFleetObservedSpeedCmps+=
            car.MeanObservedSpeedCmps;
        if(car.SourceWetSamples>0u)++out.WetCarsObserved;
        if(car.SourceStandingSamples>0u)
            ++out.StandingCarsObserved;
        if(car.StrongSlipWitnessSamples>0u)
            ++out.StrongSlipCarsObserved;
        if(car.SourceWetSamples>0u &&
           car.BrakeWitnessSamples>=
               settings.MinimumWetBrakingFramesForCue)
            ++out.WetBrakingCarsObserved;
    }
    out.MeanFleetObservedSpeedCmps/=
        static_cast<double>(WetLapTraceCarCount);
    out.Valid=out.QualifiedPhysicalCars==WetLapTraceCarCount &&
        out.PhysicalFramesReviewed==count &&
        std::isfinite(out.TotalPhysicalTravelCm) &&
        std::isfinite(out.TotalWetVehicleSeconds) &&
        std::isfinite(out.TotalStandingWaterVehicleSeconds) &&
        std::isfinite(out.MeanFleetObservedSpeedCmps);
    out.Fault=out.Valid?WetLapEngineerFault::None:
        WetLapEngineerFault::NonfiniteStatistics;
    return out;
}

inline WetLapEngineerComparison ComparePhysicalWetLapEngineer(
    const WetLapEngineerFleetReport& earlier,
    const WetLapEngineerFleetReport& later,
    double speedToleranceCmps=20.0) noexcept
{
    WetLapEngineerComparison out;
    if(!earlier.Valid || !later.Valid ||
       !earlier.SourceOnly || !later.SourceOnly ||
       earlier.Session!=later.Session ||
       earlier.Revision!=later.Revision ||
       earlier.Circuit!=later.Circuit ||
       earlier.LastSourceFrame>=later.LastSourceFrame ||
       earlier.LastSourceClockSeconds>=later.LastSourceClockSeconds ||
       !std::isfinite(speedToleranceCmps) ||
       speedToleranceCmps<0.0 ||
       speedToleranceCmps>100.0)
        return out;
    out.Session=earlier.Session;
    out.EarlierLastSourceFrame=earlier.LastSourceFrame;
    out.LaterLastSourceFrame=later.LastSourceFrame;
    out.MeanFleetSpeedChangeCmps=
        later.MeanFleetObservedSpeedCmps-
        earlier.MeanFleetObservedSpeedCmps;
    out.MaximumObservedWaterChangeMm=
        later.MaximumObservedWetnessMm-
        earlier.MaximumObservedWetnessMm;
    out.MinimumGripChange=
        later.MinimumMeasuredPhysicalGrip-
        earlier.MinimumMeasuredPhysicalGrip;
    out.WetVehicleSecondsChange=
        later.TotalWetVehicleSeconds-
        earlier.TotalWetVehicleSeconds;
    for(unsigned i=0u;i<WetLapTraceCarCount;++i)
    {
        const auto& a=earlier.Cars[i];
        const auto& b=later.Cars[i];
        if(!a.Valid || !b.Valid || a.Actor!=b.Actor)
            return {};
        if(b.SourceWetSamples>a.SourceWetSamples)
            ++out.MoreWetCars;
        if(b.MaximumObservedSlipRadians>
           a.MaximumObservedSlipRadians+1e-7)
            ++out.HigherSlipCars;
        const double speedChange=
            b.MeanObservedSpeedCmps-a.MeanObservedSpeedCmps;
        if(speedChange< -speedToleranceCmps)
            ++out.SlowerMeanSpeedCars;
        else if(speedChange>speedToleranceCmps)
            ++out.FasterMeanSpeedCars;
        else
            ++out.SimilarMeanSpeedCars;
    }
    out.Valid=std::isfinite(out.MeanFleetSpeedChangeCmps) &&
        std::isfinite(out.MaximumObservedWaterChangeMm) &&
        std::isfinite(out.MinimumGripChange) &&
        std::isfinite(out.WetVehicleSecondsChange) &&
        out.SlowerMeanSpeedCars+out.FasterMeanSpeedCars+
            out.SimilarMeanSpeedCars==WetLapTraceCarCount;
    return out;
}
} // namespace latebrake
