#pragma once

#include "Racecraft/PhysicalRaceWeatherEventJournal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Aggregates accepted, digest-chained six-car weather observations into
// race-engineer REVIEW data. This is not a lap-time computation, predicted
// driver behavior, official race classification or vehicle controller.
//
// Every time-weighted value is computed from actual source frame-to-frame
// elapsed time. No virtual sample is inserted across a missing clock span.
enum class PhysicalWeatherInsightFault : std::uint8_t
{
    None,
    InvalidSettings,
    InsufficientSourceFrames,
    CorruptDigest,
    MixedSourceEpoch,
    NonmonotonicClock,
    IdentityChanged,
    InvalidPhysicalMeasure
};

enum class PhysicalWeatherInsightTrend : std::uint8_t
{
    Unqualified,
    Unchanged,
    SightImproving,
    SightDeteriorating,
    MixedRisk
};

struct PhysicalWeatherInsightSettings final
{
    unsigned MinimumQualifiedEvents=2u;
    unsigned MaximumQualifiedEvents=PhysicalWeatherEventCapacity;
    double MaximumFrameGapSeconds=0.2;
    double MinimumFrameGapSeconds=0.001;
    double MinimumWetSectionFilmMm=1.0;
    double LowTrackLuxThreshold=5.0;
    double SightCautionRatio=1.1;
    double SightCriticalRatio=1.9;
    double RiskTrendDeadband=0.05;
    double MaximumSingleCarStoppingDistanceCm=250000.0;
    double MaximumSightDistanceCm=250000.0;
    bool RequireExactPreviousDigest=true;
    bool RequireOriginalActorOrder=true;
    bool NeverPublishOfficialRaceResults=true;
    bool NeverInferUnseenWeather=true;
    bool NeverActuateVehiclePhysics=true;
};

struct PhysicalWeatherCarInsight final
{
    bool Valid=false;
    std::uint64_t Actor=0u;
    unsigned QualifiedSamples=0u;
    unsigned CautionSamples=0u;
    unsigned CriticalSamples=0u;
    unsigned WetRoadSamples=0u;
    unsigned LowLightSamples=0u;
    double MeasuredExposureSeconds=0.0;
    double CautionExposureSeconds=0.0;
    double CriticalExposureSeconds=0.0;
    double WetRoadExposureSeconds=0.0;
    double LowLightExposureSeconds=0.0;
    double TimeIntegratedRiskRatio=0.0;
    double MeanRiskRatio=0.0;
    double MaximumRiskRatio=0.0;
    double MinimumSightReserveCm=0.0;
    double MaximumWitnessedFilmMm=0.0;
    double MinimumWitnessedTrackLux=0.0;
    double LastRiskRatio=0.0;
    double EarlyRiskRatio=0.0;
    double LargestObservedRatioChange=0.0;
    NightWetSafetyBand LastSafety=NightWetSafetyBand::Unqualified;
    PhysicalWeatherInsightTrend Trend=PhysicalWeatherInsightTrend::Unqualified;
    bool NoRealVisibilityAuthority=true;
    bool NoActualBrakingChanges=true;
};

struct PhysicalWeatherRaceInsight final
{
    bool Valid=false;
    PhysicalWeatherInsightFault Fault=
        PhysicalWeatherInsightFault::InsufficientSourceFrames;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t FirstNativeFrame=0u;
    std::uint64_t LastNativeFrame=0u;
    std::uint64_t LastVerifiedDigest=0u;
    double BeginningSourceClockSeconds=0.0;
    double EndingSourceClockSeconds=0.0;
    double TotalObservedSeconds=0.0;
    double TotalCautionVehicleSeconds=0.0;
    double TotalCriticalVehicleSeconds=0.0;
    double TotalWetRoadVehicleSeconds=0.0;
    double TotalLowLightVehicleSeconds=0.0;
    double MeanFleetRiskRatio=0.0;
    double PeakFleetRiskRatio=0.0;
    double LowestSightReserveCm=0.0;
    double MaximumObservedWaterMm=0.0;
    double MinimumMeasuredLux=0.0;
    double ProportionCarsCautioned=0.0;
    unsigned QualifiedEvents=0u;
    unsigned QualifiedCars=0u;
    unsigned CriticalEventCount=0u;
    unsigned PitWaitEventCount=0u;
    unsigned PitSafetyReviewEventCount=0u;
    unsigned RecoveringEventCount=0u;
    unsigned UnchangedEventCount=0u;
    std::array<PhysicalWeatherCarInsight,
        PhysicalWeatherEventCars> Cars{};
    bool ExactSourceTimeOnly=true;
    bool NoNativeCommandIssued=true;
    bool NoOfficialResultIssued=true;
};

struct PhysicalWeatherInsightComparison final
{
    bool Valid=false;
    std::uint64_t Session=0u;
    std::uint64_t EarlierLastFrame=0u;
    std::uint64_t LaterLastFrame=0u;
    double MeanFleetRiskChange=0.0;
    double PeakFleetRiskChange=0.0;
    double SourceWetnessChangeMm=0.0;
    double MinimumSightReserveChangeCm=0.0;
    double CautionSecondsChange=0.0;
    unsigned ImprovingCars=0u;
    unsigned DeterioratingCars=0u;
    unsigned UnchangedCars=0u;
    PhysicalWeatherInsightTrend Trend=PhysicalWeatherInsightTrend::Unqualified;
    bool SourceOnly=true;
    bool NoRaceProgressionChange=true;
};

inline bool ValidPhysicalWeatherInsightSettings(
    PhysicalWeatherInsightSettings p) noexcept
{
    return p.MinimumQualifiedEvents>=2u &&
        p.MinimumQualifiedEvents<=8u &&
        p.MaximumQualifiedEvents>=p.MinimumQualifiedEvents &&
        p.MaximumQualifiedEvents<=PhysicalWeatherEventCapacity &&
        std::isfinite(p.MaximumFrameGapSeconds) &&
        p.MaximumFrameGapSeconds>=0.05 &&
        p.MaximumFrameGapSeconds<=0.2 &&
        std::isfinite(p.MinimumFrameGapSeconds) &&
        p.MinimumFrameGapSeconds>=0.001 &&
        p.MinimumFrameGapSeconds<=0.01 &&
        p.MinimumFrameGapSeconds<p.MaximumFrameGapSeconds &&
        std::isfinite(p.MinimumWetSectionFilmMm) &&
        p.MinimumWetSectionFilmMm>=0.5 &&
        p.MinimumWetSectionFilmMm<=3.0 &&
        std::isfinite(p.LowTrackLuxThreshold) &&
        p.LowTrackLuxThreshold>=1.0 &&
        p.LowTrackLuxThreshold<=20.0 &&
        std::isfinite(p.SightCautionRatio) &&
        p.SightCautionRatio>=0.8 &&
        p.SightCautionRatio<=1.4 &&
        std::isfinite(p.SightCriticalRatio) &&
        p.SightCriticalRatio>=1.5 &&
        p.SightCriticalRatio<=2.5 &&
        p.SightCriticalRatio>p.SightCautionRatio &&
        std::isfinite(p.RiskTrendDeadband) &&
        p.RiskTrendDeadband>=0.01 &&
        p.RiskTrendDeadband<=0.2 &&
        std::isfinite(p.MaximumSingleCarStoppingDistanceCm) &&
        p.MaximumSingleCarStoppingDistanceCm>=30000.0 &&
        p.MaximumSingleCarStoppingDistanceCm<=300000.0 &&
        std::isfinite(p.MaximumSightDistanceCm) &&
        p.MaximumSightDistanceCm>=10000.0 &&
        p.MaximumSightDistanceCm<=300000.0 &&
        p.RequireExactPreviousDigest &&
        p.RequireOriginalActorOrder &&
        p.NeverPublishOfficialRaceResults &&
        p.NeverInferUnseenWeather &&
        p.NeverActuateVehiclePhysics;
}

inline bool PhysicalWeatherInsightValidCar(
    const PhysicalWeatherCarEventSample& c,
    PhysicalWeatherInsightSettings p) noexcept
{
    return c.Actor!=0u &&
        c.Safety!=NightWetSafetyBand::Unqualified &&
        c.OriginalChaosCarUnchanged &&
        c.ActualRayOcclusionUnverified &&
        std::isfinite(c.SightDistanceCm) &&
        std::isfinite(c.StoppingDistanceCm) &&
        std::isfinite(c.SightMarginCm) &&
        std::isfinite(c.RiskRatio) &&
        std::isfinite(c.RoadWaterFilmMm) &&
        std::isfinite(c.TrackLux) &&
        c.SightDistanceCm>0.0 &&
        c.SightDistanceCm<=p.MaximumSightDistanceCm &&
        c.StoppingDistanceCm>=0.0 &&
        c.StoppingDistanceCm<=
            p.MaximumSingleCarStoppingDistanceCm &&
        c.RiskRatio>=0.0 &&
        c.RiskRatio<=100.0 &&
        c.RoadWaterFilmMm>=0.0 &&
        c.RoadWaterFilmMm<=ObservedWaterMaximumMm &&
        c.TrackLux>=0.0 &&
        c.TrackLux<=1000.0 &&
        std::abs(c.SightDistanceCm-
            c.StoppingDistanceCm-
            c.SightMarginCm)<=1.0e-3;
}

inline bool PhysicalWeatherInsightValidEvent(
    const PhysicalWeatherEvent& event,
    PhysicalWeatherInsightSettings p) noexcept
{
    if(!event.Valid ||
       event.Session==0u ||
       event.Revision==0u ||
       event.NativeFrame==0u ||
       event.SourceCars!=PhysicalWeatherEventCars ||
       !event.SourceOnly ||
       !event.RealRaceDataUnaffected ||
       event.Digest==0u ||
       event.Digest!=PhysicalWeatherEventDigest(event) ||
       !std::isfinite(event.NativeClockSeconds) ||
       event.NativeClockSeconds<=0.0 ||
       !std::isfinite(event.MaximumSourceFilmMm) ||
       event.MaximumSourceFilmMm<0.0 ||
       !std::isfinite(event.MinimumSourceSightMarginCm))
        return false;
    unsigned caution=0u;
    unsigned critical=0u;
    double smallest=std::numeric_limits<double>::infinity();
    double mostWater=0.0;
    for(std::size_t i=0u;i<PhysicalWeatherEventCars;++i)
    {
        const auto& c=event.Cars[i];
        if(!PhysicalWeatherInsightValidCar(c,p))return false;
        for(std::size_t j=0u;j<i;++j)
            if(event.Cars[j].Actor==c.Actor)
                return false;
        smallest=std::min(smallest,c.SightMarginCm);
        mostWater=std::max(mostWater,c.RoadWaterFilmMm);
        if(c.Safety==NightWetSafetyBand::Critical)++critical;
        if(c.Safety==NightWetSafetyBand::Critical ||
           c.Safety==NightWetSafetyBand::LimitedSight ||
           c.Safety==NightWetSafetyBand::Caution)
            ++caution;
    }
    return caution==event.CautionCars &&
        critical==event.CriticalCars &&
        std::abs(smallest-event.MinimumSourceSightMarginCm)<=1e-3 &&
        std::abs(mostWater-event.MaximumSourceFilmMm)<=1e-3;
}

inline PhysicalWeatherInsightTrend PhysicalWeatherRiskTrend(
    double first,double latest,double deadband=0.05) noexcept
{
    if(!std::isfinite(first) ||
       !std::isfinite(latest) ||
       !std::isfinite(deadband) ||
       deadband<0.0)
        return PhysicalWeatherInsightTrend::Unqualified;
    const double delta=latest-first;
    if(delta>deadband)
        return PhysicalWeatherInsightTrend::SightDeteriorating;
    if(delta< -deadband)
        return PhysicalWeatherInsightTrend::SightImproving;
    return PhysicalWeatherInsightTrend::Unchanged;
}

inline PhysicalWeatherRaceInsight BuildPhysicalWeatherRaceInsight(
    const PhysicalRaceWeatherEventJournal& journal,
    PhysicalWeatherInsightSettings settings={}) noexcept
{
    PhysicalWeatherRaceInsight out;
    if(!ValidPhysicalWeatherInsightSettings(settings))
    {
        out.Fault=PhysicalWeatherInsightFault::InvalidSettings;
        return out;
    }
    const std::size_t count=journal.Size();
    if(count<settings.MinimumQualifiedEvents ||
       count>settings.MaximumQualifiedEvents)
        return out;
    PhysicalWeatherEvent first;
    if(!journal.Read(0u,first) ||
       !PhysicalWeatherInsightValidEvent(first,settings))
    {
        out.Fault=PhysicalWeatherInsightFault::CorruptDigest;
        return out;
    }
    out.Circuit=first.Circuit;
    out.Session=first.Session;
    out.Revision=first.Revision;
    out.FirstNativeFrame=first.NativeFrame;
    out.BeginningSourceClockSeconds=first.NativeClockSeconds;
    out.LowestSightReserveCm=
        std::numeric_limits<double>::infinity();
    out.MinimumMeasuredLux=std::numeric_limits<double>::infinity();
    for(std::size_t car=0u;car<PhysicalWeatherEventCars;++car)
    {
        auto& result=out.Cars[car];
        const auto& source=first.Cars[car];
        result.Actor=source.Actor;
        result.MinimumSightReserveCm=
            std::numeric_limits<double>::infinity();
        result.MinimumWitnessedTrackLux=
            std::numeric_limits<double>::infinity();
        result.EarlyRiskRatio=source.RiskRatio;
    }

    PhysicalWeatherEvent previous=first;
    for(std::size_t n=0u;n<count;++n)
    {
        PhysicalWeatherEvent current;
        if(!journal.Read(n,current) ||
           !PhysicalWeatherInsightValidEvent(current,settings))
        {
            out.Fault=PhysicalWeatherInsightFault::CorruptDigest;
            return out;
        }
        if(current.Session!=out.Session ||
           current.Revision!=out.Revision ||
           current.Circuit!=out.Circuit)
        {
            out.Fault=PhysicalWeatherInsightFault::MixedSourceEpoch;
            return out;
        }
        double duration=0.0;
        if(n>0u)
        {
            if(current.PreviousDigest!=previous.Digest ||
               current.NativeFrame<=previous.NativeFrame)
            {
                out.Fault=PhysicalWeatherInsightFault::CorruptDigest;
                return out;
            }
            duration=current.NativeClockSeconds-
                previous.NativeClockSeconds;
            if(!std::isfinite(duration) ||
               duration<settings.MinimumFrameGapSeconds ||
               duration>settings.MaximumFrameGapSeconds)
            {
                out.Fault=PhysicalWeatherInsightFault::NonmonotonicClock;
                return out;
            }
        }
        for(std::size_t car=0u;car<PhysicalWeatherEventCars;++car)
        {
            const auto& input=current.Cars[car];
            auto& result=out.Cars[car];
            if(input.Actor!=result.Actor)
            {
                out.Fault=PhysicalWeatherInsightFault::IdentityChanged;
                return out;
            }
            if(n>0u)
            {
                const auto& prior=previous.Cars[car];
                const double change=std::abs(
                    input.RiskRatio-prior.RiskRatio);
                result.LargestObservedRatioChange=std::max(
                    result.LargestObservedRatioChange,change);
                result.TimeIntegratedRiskRatio+=duration*
                    (input.RiskRatio+prior.RiskRatio)*0.5;
                result.MeasuredExposureSeconds+=duration;
                if(input.RiskRatio>=settings.SightCautionRatio ||
                   prior.RiskRatio>=settings.SightCautionRatio)
                {
                    result.CautionExposureSeconds+=duration;
                    out.TotalCautionVehicleSeconds+=duration;
                }
                if(input.RiskRatio>=settings.SightCriticalRatio ||
                   prior.RiskRatio>=settings.SightCriticalRatio)
                {
                    result.CriticalExposureSeconds+=duration;
                    out.TotalCriticalVehicleSeconds+=duration;
                }
                if(input.RoadWaterFilmMm>=
                       settings.MinimumWetSectionFilmMm ||
                   prior.RoadWaterFilmMm>=
                       settings.MinimumWetSectionFilmMm)
                {
                    result.WetRoadExposureSeconds+=duration;
                    out.TotalWetRoadVehicleSeconds+=duration;
                }
                if(input.TrackLux<=settings.LowTrackLuxThreshold ||
                   prior.TrackLux<=settings.LowTrackLuxThreshold)
                {
                    result.LowLightExposureSeconds+=duration;
                    out.TotalLowLightVehicleSeconds+=duration;
                }
            }
            ++result.QualifiedSamples;
            result.LastRiskRatio=input.RiskRatio;
            result.LastSafety=input.Safety;
            result.MaximumRiskRatio=std::max(
                result.MaximumRiskRatio,input.RiskRatio);
            result.MinimumSightReserveCm=std::min(
                result.MinimumSightReserveCm,input.SightMarginCm);
            result.MaximumWitnessedFilmMm=std::max(
                result.MaximumWitnessedFilmMm,input.RoadWaterFilmMm);
            result.MinimumWitnessedTrackLux=std::min(
                result.MinimumWitnessedTrackLux,input.TrackLux);
            if(input.RiskRatio>=settings.SightCautionRatio)
                ++result.CautionSamples;
            if(input.RiskRatio>=settings.SightCriticalRatio)
                ++result.CriticalSamples;
            if(input.RoadWaterFilmMm>=settings.MinimumWetSectionFilmMm)
                ++result.WetRoadSamples;
            if(input.TrackLux<=settings.LowTrackLuxThreshold)
                ++result.LowLightSamples;
        }
        out.LowestSightReserveCm=std::min(
            out.LowestSightReserveCm,
            current.MinimumSourceSightMarginCm);
        out.MaximumObservedWaterMm=std::max(
            out.MaximumObservedWaterMm,current.MaximumSourceFilmMm);
        for(const auto& car:current.Cars)
            out.MinimumMeasuredLux=std::min(
                out.MinimumMeasuredLux,car.TrackLux);
        if(current.Kind==PhysicalWeatherEventKind::CriticalSight)
            ++out.CriticalEventCount;
        if(current.Kind==PhysicalWeatherEventKind::PitWaitForTraffic)
            ++out.PitWaitEventCount;
        if(current.Kind==PhysicalWeatherEventKind::PitSourceSafetyCheck)
            ++out.PitSafetyReviewEventCount;
        if(current.Kind==PhysicalWeatherEventKind::SightRecovered)
            ++out.RecoveringEventCount;
        if(current.Kind==PhysicalWeatherEventKind::ObserveStable)
            ++out.UnchangedEventCount;
        ++out.QualifiedEvents;
        previous=current;
    }
    out.EndingSourceClockSeconds=previous.NativeClockSeconds;
    out.LastNativeFrame=previous.NativeFrame;
    out.LastVerifiedDigest=previous.Digest;
    out.TotalObservedSeconds=out.EndingSourceClockSeconds-
        out.BeginningSourceClockSeconds;
    if(!std::isfinite(out.TotalObservedSeconds) ||
       out.TotalObservedSeconds<=0.0 ||
       out.QualifiedEvents!=count ||
       out.LastVerifiedDigest!=journal.CurrentDigest())
    {
        out.Fault=PhysicalWeatherInsightFault::InvalidPhysicalMeasure;
        return out;
    }
    double sumMeans=0.0;
    for(auto& result:out.Cars)
    {
        if(result.MeasuredExposureSeconds<=0.0 ||
           result.QualifiedSamples!=count)
        {
            out.Fault=PhysicalWeatherInsightFault::InvalidPhysicalMeasure;
            return out;
        }
        result.MeanRiskRatio=result.TimeIntegratedRiskRatio/
            result.MeasuredExposureSeconds;
        result.Trend=PhysicalWeatherRiskTrend(
            result.EarlyRiskRatio,result.LastRiskRatio,
            settings.RiskTrendDeadband);
        result.Valid=std::isfinite(result.MeanRiskRatio) &&
            result.MeanRiskRatio>=0.0;
        if(!result.Valid)
        {
            out.Fault=PhysicalWeatherInsightFault::InvalidPhysicalMeasure;
            return out;
        }
        ++out.QualifiedCars;
        sumMeans+=result.MeanRiskRatio;
        out.PeakFleetRiskRatio=std::max(
            out.PeakFleetRiskRatio,result.MaximumRiskRatio);
    }
    out.MeanFleetRiskRatio=sumMeans/
        static_cast<double>(PhysicalWeatherEventCars);
    out.ProportionCarsCautioned=
        out.TotalCautionVehicleSeconds/
        (out.TotalObservedSeconds*PhysicalWeatherEventCars);
    out.Valid=out.QualifiedCars==PhysicalWeatherEventCars &&
        std::isfinite(out.MeanFleetRiskRatio) &&
        std::isfinite(out.ProportionCarsCautioned) &&
        out.ProportionCarsCautioned>=0.0 &&
        out.ProportionCarsCautioned<=1.0+1.0e-9;
    out.Fault=out.Valid?
        PhysicalWeatherInsightFault::None:
        PhysicalWeatherInsightFault::InvalidPhysicalMeasure;
    return out;
}

inline PhysicalWeatherInsightComparison ComparePhysicalWeatherInsights(
    const PhysicalWeatherRaceInsight& before,
    const PhysicalWeatherRaceInsight& after,
    PhysicalWeatherInsightSettings settings={}) noexcept
{
    PhysicalWeatherInsightComparison out;
    if(!ValidPhysicalWeatherInsightSettings(settings) ||
       !before.Valid ||
       !after.Valid ||
       before.Circuit!=after.Circuit ||
       before.Session!=after.Session ||
       before.Revision!=after.Revision ||
       before.LastNativeFrame>=after.LastNativeFrame ||
       before.EndingSourceClockSeconds>=after.EndingSourceClockSeconds ||
       !before.NoOfficialResultIssued ||
       !after.NoOfficialResultIssued)
        return out;
    out.Session=before.Session;
    out.EarlierLastFrame=before.LastNativeFrame;
    out.LaterLastFrame=after.LastNativeFrame;
    out.MeanFleetRiskChange=after.MeanFleetRiskRatio-
        before.MeanFleetRiskRatio;
    out.PeakFleetRiskChange=after.PeakFleetRiskRatio-
        before.PeakFleetRiskRatio;
    out.SourceWetnessChangeMm=after.MaximumObservedWaterMm-
        before.MaximumObservedWaterMm;
    out.MinimumSightReserveChangeCm=after.LowestSightReserveCm-
        before.LowestSightReserveCm;
    out.CautionSecondsChange=after.TotalCautionVehicleSeconds-
        before.TotalCautionVehicleSeconds;
    for(std::size_t car=0u;car<PhysicalWeatherEventCars;++car)
    {
        if(before.Cars[car].Actor!=after.Cars[car].Actor ||
           !before.Cars[car].Valid ||
           !after.Cars[car].Valid)
            return {};
        const auto trend=PhysicalWeatherRiskTrend(
            before.Cars[car].MeanRiskRatio,
            after.Cars[car].MeanRiskRatio,
            settings.RiskTrendDeadband);
        if(trend==PhysicalWeatherInsightTrend::SightImproving)
            ++out.ImprovingCars;
        else if(trend==PhysicalWeatherInsightTrend::SightDeteriorating)
            ++out.DeterioratingCars;
        else
            ++out.UnchangedCars;
    }
    if(out.ImprovingCars>0u && out.DeterioratingCars>0u)
        out.Trend=PhysicalWeatherInsightTrend::MixedRisk;
    else if(out.ImprovingCars>0u)
        out.Trend=PhysicalWeatherInsightTrend::SightImproving;
    else if(out.DeterioratingCars>0u)
        out.Trend=PhysicalWeatherInsightTrend::SightDeteriorating;
    else
        out.Trend=PhysicalWeatherInsightTrend::Unchanged;
    out.Valid=std::isfinite(out.MeanFleetRiskChange) &&
        std::isfinite(out.PeakFleetRiskChange) &&
        std::isfinite(out.MinimumSightReserveChangeCm) &&
        out.ImprovingCars+out.DeterioratingCars+
            out.UnchangedCars==PhysicalWeatherEventCars;
    return out;
}
} // namespace latebrake
