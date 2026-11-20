#pragma once
#include "Track/WorldScaleObservedWaterFilm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Six-circuit endurance/pit strategy derived exclusively from witnessed
// native telemetry. It returns a WHAT-IF estimate, never a pit request,
// tyre replacement, fuel change, credits, official race result or command.
// Weather scenarios are user-selected "if" assumptions, NOT forecasts.
inline constexpr unsigned WetPitCompoundCount=3u;
inline constexpr unsigned WetPitMaximumRemainingLaps=24u;
inline constexpr unsigned WetPitMaximumLapSamples=16u;
inline constexpr unsigned WetPitMaximumCandidates=
    WetPitMaximumRemainingLaps*WetPitCompoundCount;
enum class WetPitCompound : std::uint8_t {
    Dry, Intermediate, FullWet
};
enum class WetPitStrategyFault : std::uint8_t {
    None, InvalidPolicy, MissingPhysicalTelemetry, InvalidWeather,
    MixedSession, UnsafeInventory, UnqualifiedLapSamples,
    NoFeasibleScenario, InvalidPitEvidence
};
enum class WetPitAdvisory : std::uint8_t {
    Unqualified, RemainOnCurrentTyres,
    StopSoonForWear, EvaluateAlternativeCompound
};
struct WetPitSettings final {
    double MinimumTreadMm=1.5;
    double MaximumTreadMm=12.0;
    double MinimumReserveFuelLitres=2.0;
    double MaximumConsumptionLitresPerLap=8.0;
    double MaximumTyreWearMmPerLap=0.85;
    double MinimumPitDurationSeconds=5.0;
    double MaximumPitDurationSeconds=120.0;
    double MinimumObservedLapSeconds=30.0;
    double MaximumObservedLapSeconds=600.0;
    double MaximumLapTimeVariationFraction=0.40;
    double MaximumSensorClockSkewSeconds=0.05;
    double MinimumSensorConfidence=0.85;
    unsigned MinimumQualifiedLapSamples=3u;
    unsigned MaximumRecommendedStops=1u;
    bool RequireRealPhysicalTiming=true;
    bool RequireActualPitLossEvidence=true;
    bool RequirePhysicalCompoundInventory=true;
    bool NeverRefuelUnrealVehicle=true;
    bool NeverChangeNativeTyres=true;
    bool NeverGrantRaceRewards=true;
};
struct WetPitMeasuredCompound final {
    WetPitCompound Compound=WetPitCompound::Dry;
    bool AvailableInNativeGarage=false;
    bool ActualCarAndRoadLapTiming=true;
    double InstalledTreadMm=0.0;
    double VerifiedWearMmPerLap=0.0;
    double Confidence=0.0;
    unsigned QualifiedSamples=0u;
    std::array<double,WetPitMaximumLapSamples> LapSeconds{};
};
struct WetPitMeasuredRace final {
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t SessionRevision=0u;
    std::uint64_t ChassisActor=0u;
    std::uint64_t LastNativeFrame=0u;
    double EngineClockSeconds=0.0;
    unsigned RemainingOfficialLaps=0u;
    WetPitCompound CurrentCompound=WetPitCompound::Dry;
    double ActualCurrentTreadMm=0.0;
    double ActualFuelLitres=0.0;
    double MeasuredFuelLitresPerLap=0.0;
    double Confidence=0.0;
    double MeasuredPitTimeLossSeconds=0.0;
    double MeasuredPitLossConfidence=0.0;
    bool RealChaosCarMeasured=false;
    bool RealPitLaneTimeLossMeasured=false;
    bool OriginalOfficialLapClockVerified=false;
    bool OriginalRaceAndGarageUnmodified=true;
    std::array<WetPitMeasuredCompound,WetPitCompoundCount> Compounds{};
};
struct WetPitScenario final {
    bool Valid=false;
    unsigned ScenarioId=0u;
    unsigned RemainingLaps=0u;
    // Explicit prospective scenario, not a meteorological forecast.
    std::array<double,WetPitMaximumRemainingLaps> AssumedFilmMm{};
    bool ExternalScenarioAcknowledged=false;
    bool NotAnEngineWeatherForecast=true;
};
struct WetPitCandidate final {
    bool Valid=false;
    unsigned PlannedStopAfterLaps=0u;
    WetPitCompound SelectedCompound=WetPitCompound::Dry;
    double PredictedTotalSeconds=0.0;
    double ConservativePenaltySeconds=0.0;
    double MeanCurrentStintLapSeconds=0.0;
    double MeanSecondStintLapSeconds=0.0;
    double TyreMarginMm=0.0;
    double FuelMarginLitres=0.0;
    double AssumedMeanWaterMm=0.0;
    bool RequiresUnverifiedWeatherFuture=true;
    bool NoActualPitIssued=true;
    bool NoGarageStateChanged=true;
};
struct WetPitStrategyPlan final {
    bool Valid=false;
    WetPitStrategyFault Fault=WetPitStrategyFault::MissingPhysicalTelemetry;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t LastSourceFrame=0u;
    double SourceClockSeconds=0.0;
    unsigned RemainingLaps=0u;
    unsigned CandidateCount=0u;
    unsigned FeasibleCandidateCount=0u;
    double ConservativeNoStopSeconds=0.0;
    double BestCandidateSeconds=0.0;
    WetPitAdvisory Advisory=WetPitAdvisory::Unqualified;
    WetPitCandidate Best{};
    std::array<WetPitCandidate,WetPitMaximumCandidates> Candidates{};
    bool NoOfficialTimingAuthority=true;
    bool NoOriginalPhysicsChanges=true;
    bool SourceOnly=true;
};
inline unsigned WetPitCompoundIndex(WetPitCompound compound) noexcept {
    switch(compound){
    case WetPitCompound::Dry:return 0u;
    case WetPitCompound::Intermediate:return 1u;
    case WetPitCompound::FullWet:return 2u;
    }
    return WetPitCompoundCount;
}
inline bool ValidWetPitSettings(WetPitSettings s) noexcept {
    return std::isfinite(s.MinimumTreadMm) &&
        s.MinimumTreadMm>=0.5 && s.MinimumTreadMm<=3.0 &&
        std::isfinite(s.MaximumTreadMm) &&
        s.MaximumTreadMm>=8.0 && s.MaximumTreadMm<=15.0 &&
        s.MaximumTreadMm>s.MinimumTreadMm &&
        std::isfinite(s.MinimumReserveFuelLitres) &&
        s.MinimumReserveFuelLitres>=0.0 &&
        s.MinimumReserveFuelLitres<=10.0 &&
        std::isfinite(s.MaximumConsumptionLitresPerLap) &&
        s.MaximumConsumptionLitresPerLap>=1.0 &&
        s.MaximumConsumptionLitresPerLap<=15.0 &&
        std::isfinite(s.MaximumTyreWearMmPerLap) &&
        s.MaximumTyreWearMmPerLap>=0.25 &&
        s.MaximumTyreWearMmPerLap<=1.5 &&
        std::isfinite(s.MinimumPitDurationSeconds) &&
        s.MinimumPitDurationSeconds>=1.0 &&
        s.MinimumPitDurationSeconds<=15.0 &&
        std::isfinite(s.MaximumPitDurationSeconds) &&
        s.MaximumPitDurationSeconds>=60.0 &&
        s.MaximumPitDurationSeconds<=240.0 &&
        std::isfinite(s.MinimumObservedLapSeconds) &&
        s.MinimumObservedLapSeconds>=20.0 &&
        s.MinimumObservedLapSeconds<=45.0 &&
        std::isfinite(s.MaximumObservedLapSeconds) &&
        s.MaximumObservedLapSeconds>=450.0 &&
        s.MaximumObservedLapSeconds<=900.0 &&
        std::isfinite(s.MaximumLapTimeVariationFraction) &&
        s.MaximumLapTimeVariationFraction>=0.2 &&
        s.MaximumLapTimeVariationFraction<=0.7 &&
        std::isfinite(s.MaximumSensorClockSkewSeconds) &&
        s.MaximumSensorClockSkewSeconds>=0.0 &&
        s.MaximumSensorClockSkewSeconds<=0.1 &&
        std::isfinite(s.MinimumSensorConfidence) &&
        s.MinimumSensorConfidence>=0.5 &&
        s.MinimumSensorConfidence<=1.0 &&
        s.MinimumQualifiedLapSamples>=3u &&
        s.MinimumQualifiedLapSamples<=8u &&
        s.MaximumRecommendedStops==1u &&
        s.RequireRealPhysicalTiming &&
        s.RequireActualPitLossEvidence &&
        s.RequirePhysicalCompoundInventory &&
        s.NeverRefuelUnrealVehicle &&
        s.NeverChangeNativeTyres &&
        s.NeverGrantRaceRewards;
}
inline bool ValidWetPitMeasuredRace(
    const WorldScaleVariantRecipe& circuit,
    WetPitMeasuredRace race,
    WetPitSettings settings) noexcept
{
    if(!ValidWetPitSettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       race.Circuit!=circuit.Id ||
       race.Session==0u ||
       race.SessionRevision==0u ||
       race.ChassisActor==0u ||
       race.LastNativeFrame==0u ||
       race.RemainingOfficialLaps<1u ||
       race.RemainingOfficialLaps>WetPitMaximumRemainingLaps ||
       WetPitCompoundIndex(race.CurrentCompound)>=WetPitCompoundCount ||
       !std::isfinite(race.EngineClockSeconds) ||
       race.EngineClockSeconds<=0.0 ||
       !std::isfinite(race.ActualCurrentTreadMm) ||
       race.ActualCurrentTreadMm<settings.MinimumTreadMm ||
       race.ActualCurrentTreadMm>settings.MaximumTreadMm ||
       !std::isfinite(race.ActualFuelLitres) ||
       race.ActualFuelLitres<0.0 ||
       race.ActualFuelLitres>250.0 ||
       !std::isfinite(race.MeasuredFuelLitresPerLap) ||
       race.MeasuredFuelLitresPerLap<0.01 ||
       race.MeasuredFuelLitresPerLap>
           settings.MaximumConsumptionLitresPerLap ||
       !std::isfinite(race.Confidence) ||
       race.Confidence<settings.MinimumSensorConfidence ||
       race.Confidence>1.0 ||
       !std::isfinite(race.MeasuredPitTimeLossSeconds) ||
       race.MeasuredPitTimeLossSeconds<
           settings.MinimumPitDurationSeconds ||
       race.MeasuredPitTimeLossSeconds>
           settings.MaximumPitDurationSeconds ||
       !std::isfinite(race.MeasuredPitLossConfidence) ||
       race.MeasuredPitLossConfidence<settings.MinimumSensorConfidence ||
       race.MeasuredPitLossConfidence>1.0 ||
       !race.RealChaosCarMeasured ||
       !race.RealPitLaneTimeLossMeasured ||
       !race.OriginalOfficialLapClockVerified ||
       !race.OriginalRaceAndGarageUnmodified)
        return false;
    for(unsigned i=0u;i<WetPitCompoundCount;++i){
        const auto& c=race.Compounds[i];
        if(WetPitCompoundIndex(c.Compound)!=i ||
           !c.ActualCarAndRoadLapTiming ||
           !std::isfinite(c.Confidence) ||
           c.Confidence<settings.MinimumSensorConfidence ||
           c.Confidence>1.0 ||
           !std::isfinite(c.VerifiedWearMmPerLap) ||
           c.VerifiedWearMmPerLap<0.0 ||
           c.VerifiedWearMmPerLap>
               settings.MaximumTyreWearMmPerLap ||
           !std::isfinite(c.InstalledTreadMm) ||
           c.InstalledTreadMm<settings.MinimumTreadMm ||
           c.InstalledTreadMm>settings.MaximumTreadMm ||
           c.QualifiedSamples<settings.MinimumQualifiedLapSamples ||
           c.QualifiedSamples>WetPitMaximumLapSamples)
            return false;
        for(unsigned sample=0;sample<c.QualifiedSamples;++sample)
            if(!std::isfinite(c.LapSeconds[sample]) ||
               c.LapSeconds[sample]<settings.MinimumObservedLapSeconds ||
               c.LapSeconds[sample]>settings.MaximumObservedLapSeconds)
                return false;
    }
    return true;
}
inline bool ValidWetPitScenario(
    WetPitScenario s,WetPitMeasuredRace race,
    WetPitSettings settings) noexcept
{
    if(!s.Valid || !s.ExternalScenarioAcknowledged ||
       !s.NotAnEngineWeatherForecast || s.ScenarioId==0u ||
       s.RemainingLaps!=race.RemainingOfficialLaps ||
       !ValidWetPitSettings(settings))
        return false;
    for(unsigned i=0u;i<s.RemainingLaps;++i)
        if(!std::isfinite(s.AssumedFilmMm[i]) ||
           s.AssumedFilmMm[i]<0.0 ||
           s.AssumedFilmMm[i]>ObservedWaterMaximumMm)
            return false;
    return true;
}
struct WetPitLapEvidence final {
    bool Valid=false;
    double MeanSeconds=0.0;
    double WorstSeconds=0.0;
    double ConservativeSeconds=0.0;
    double SampleSpreadFraction=0.0;
};
inline WetPitLapEvidence SummarizeWetPitLapEvidence(
    const WetPitMeasuredCompound& compound,
    WetPitSettings settings={}) noexcept
{
    WetPitLapEvidence out;
    if(!ValidWetPitSettings(settings) ||
       compound.QualifiedSamples<settings.MinimumQualifiedLapSamples ||
       compound.QualifiedSamples>WetPitMaximumLapSamples)
        return out;
    double sum=0.0;
    double lowest=std::numeric_limits<double>::infinity();
    double highest=0.0;
    for(unsigned i=0u;i<compound.QualifiedSamples;++i){
        const double time=compound.LapSeconds[i];
        if(!std::isfinite(time) ||
           time<settings.MinimumObservedLapSeconds ||
           time>settings.MaximumObservedLapSeconds)
            return out;
        sum+=time;
        lowest=std::min(lowest,time);
        highest=std::max(highest,time);
    }
    const double mean=sum/compound.QualifiedSamples;
    const double variation=(highest-lowest)/mean;
    if(!std::isfinite(variation) ||
       variation>settings.MaximumLapTimeVariationFraction)
        return out;
    out.Valid=true;
    out.MeanSeconds=mean;
    out.WorstSeconds=highest;
    out.ConservativeSeconds=std::max(
        mean+0.35*(highest-mean),mean);
    out.SampleSpreadFraction=variation;
    return out;
}
inline double WetPitScenarioWaterSeverity(
    WetPitScenario scenario,unsigned start,unsigned laps) noexcept
{
    if(!scenario.Valid || laps==0u ||
       start+laps>scenario.RemainingLaps)return 0.0;
    double total=0.0;
    for(unsigned i=start;i<start+laps;++i)
        total+=std::clamp(scenario.AssumedFilmMm[i]/
            ObservedWaterMaximumMm,0.0,1.0);
    return total/laps;
}
inline double WetPitCompoundScenarioPenalty(
    WetPitCompound compound,double severity) noexcept
{
    const double wet=std::clamp(severity,0.0,1.0);
    switch(compound){
    case WetPitCompound::Dry:return 1.0+0.18*wet;
    case WetPitCompound::Intermediate:
        return 1.0+0.035*std::abs(wet-0.35);
    case WetPitCompound::FullWet:
        return 1.0+0.12*(1.0-wet);
    }
    return std::numeric_limits<double>::infinity();
}
inline WetPitCandidate EvaluateWetPitCandidate(
    const WetPitMeasuredRace& race,
    WetPitScenario scenario,
    unsigned stopAfterLaps,
    WetPitCompound nextCompound,
    WetPitSettings settings={}) noexcept
{
    WetPitCandidate out;
    if(!ValidWetPitSettings(settings) ||
       !ValidWetPitScenario(scenario,race,settings) ||
       stopAfterLaps>=race.RemainingOfficialLaps ||
       WetPitCompoundIndex(nextCompound)>=WetPitCompoundCount)
        return out;
    const auto& current=race.Compounds[
        WetPitCompoundIndex(race.CurrentCompound)];
    const auto& next=race.Compounds[
        WetPitCompoundIndex(nextCompound)];
    if(!next.AvailableInNativeGarage)
        return out;
    const auto first=SummarizeWetPitLapEvidence(current,settings);
    const auto second=SummarizeWetPitLapEvidence(next,settings);
    if(!first.Valid || !second.Valid)return out;
    const unsigned secondLaps=race.RemainingOfficialLaps-
        stopAfterLaps;
    const double tyreMarginCurrent=race.ActualCurrentTreadMm-
        current.VerifiedWearMmPerLap*stopAfterLaps-
        settings.MinimumTreadMm;
    const double tyreMarginNext=next.InstalledTreadMm-
        next.VerifiedWearMmPerLap*secondLaps-
        settings.MinimumTreadMm;
    const double fuelMargin=race.ActualFuelLitres-
        race.MeasuredFuelLitresPerLap*race.RemainingOfficialLaps-
        settings.MinimumReserveFuelLitres;
    if(tyreMarginCurrent<0.0 || tyreMarginNext<0.0 ||
       fuelMargin<0.0)return out;
    const double firstWet=WetPitScenarioWaterSeverity(
        scenario,0u,stopAfterLaps);
    const double secondWet=WetPitScenarioWaterSeverity(
        scenario,stopAfterLaps,secondLaps);
    const double firstTime=first.ConservativeSeconds*
        WetPitCompoundScenarioPenalty(race.CurrentCompound,firstWet);
    const double secondTime=second.ConservativeSeconds*
        WetPitCompoundScenarioPenalty(nextCompound,secondWet);
    const double pitLoss=race.MeasuredPitTimeLossSeconds/
        race.MeasuredPitLossConfidence;
    const double total=stopAfterLaps*firstTime+
        secondLaps*secondTime+pitLoss;
    if(!std::isfinite(total) ||
       total<=0.0 ||
       total>settings.MaximumObservedLapSeconds*
           race.RemainingOfficialLaps+settings.MaximumPitDurationSeconds)
        return out;
    out.Valid=true;
    out.PlannedStopAfterLaps=stopAfterLaps;
    out.SelectedCompound=nextCompound;
    out.PredictedTotalSeconds=total;
    out.ConservativePenaltySeconds=
        total-(stopAfterLaps*first.MeanSeconds+
               secondLaps*second.MeanSeconds);
    out.MeanCurrentStintLapSeconds=firstTime;
    out.MeanSecondStintLapSeconds=secondTime;
    out.TyreMarginMm=std::min(tyreMarginCurrent,tyreMarginNext);
    out.FuelMarginLitres=fuelMargin;
    out.AssumedMeanWaterMm=(firstWet*stopAfterLaps+
        secondWet*secondLaps)/
        race.RemainingOfficialLaps*ObservedWaterMaximumMm;
    return out;
}
inline WetPitStrategyPlan PlanPhysicalWetPitStrategy(
    const WorldScaleVariantRecipe& circuit,
    const WetPitMeasuredRace& race,
    WetPitScenario scenario,
    WetPitSettings settings={}) noexcept
{
    WetPitStrategyPlan out;
    if(!ValidWetPitSettings(settings)){
        out.Fault=WetPitStrategyFault::InvalidPolicy;
        return out;
    }
    if(!ValidWetPitMeasuredRace(circuit,race,settings)){
        out.Fault=WetPitStrategyFault::MissingPhysicalTelemetry;
        return out;
    }
    if(!ValidWetPitScenario(scenario,race,settings)){
        out.Fault=WetPitStrategyFault::InvalidWeather;
        return out;
    }
    out.Circuit=circuit.Id;
    out.Session=race.Session;
    out.Revision=race.SessionRevision;
    out.LastSourceFrame=race.LastNativeFrame;
    out.SourceClockSeconds=race.EngineClockSeconds;
    out.RemainingLaps=race.RemainingOfficialLaps;
    const auto currentEvidence=SummarizeWetPitLapEvidence(
        race.Compounds[WetPitCompoundIndex(
            race.CurrentCompound)],settings);
    if(!currentEvidence.Valid){
        out.Fault=WetPitStrategyFault::UnqualifiedLapSamples;
        return out;
    }
    const double currentWear=race.Compounds[
        WetPitCompoundIndex(race.CurrentCompound)]
        .VerifiedWearMmPerLap;
    const double currentMargin=race.ActualCurrentTreadMm-
        currentWear*race.RemainingOfficialLaps-
        settings.MinimumTreadMm;
    const double water=WetPitScenarioWaterSeverity(
        scenario,0u,race.RemainingOfficialLaps);
    out.ConservativeNoStopSeconds=currentMargin>=0.0 ?
        currentEvidence.ConservativeSeconds*
        WetPitCompoundScenarioPenalty(
            race.CurrentCompound,water)*
        race.RemainingOfficialLaps:
        std::numeric_limits<double>::infinity();
    out.BestCandidateSeconds=
        std::numeric_limits<double>::infinity();
    for(unsigned stop=1u;stop<race.RemainingOfficialLaps;++stop){
        for(unsigned tyre=0u;tyre<WetPitCompoundCount;++tyre){
            auto candidate=EvaluateWetPitCandidate(
                race,scenario,stop,static_cast<WetPitCompound>(tyre),
                settings);
            if(out.CandidateCount>=out.Candidates.size())break;
            out.Candidates[out.CandidateCount++]=candidate;
            if(!candidate.Valid)continue;
            ++out.FeasibleCandidateCount;
            if(candidate.PredictedTotalSeconds<
               out.BestCandidateSeconds){
                out.BestCandidateSeconds=
                    candidate.PredictedTotalSeconds;
                out.Best=candidate;
            }
        }
    }
    if(!std::isfinite(out.ConservativeNoStopSeconds) &&
       out.FeasibleCandidateCount==0u){
        out.Fault=WetPitStrategyFault::NoFeasibleScenario;
        return out;
    }
    if(out.FeasibleCandidateCount==0u ||
       (std::isfinite(out.ConservativeNoStopSeconds) &&
        out.ConservativeNoStopSeconds<=out.BestCandidateSeconds)){
        out.Advisory=WetPitAdvisory::RemainOnCurrentTyres;
    }else{
        out.Advisory=currentMargin<0.0 ?
            WetPitAdvisory::StopSoonForWear:
            WetPitAdvisory::EvaluateAlternativeCompound;
    }
    out.Fault=WetPitStrategyFault::None;
    out.Valid=true;
    return out;
}
} // namespace latebrake
