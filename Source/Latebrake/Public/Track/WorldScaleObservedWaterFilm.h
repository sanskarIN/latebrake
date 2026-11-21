#pragma once
#include "Track/WorldScaleBankedRoadPlan.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Portable source-only conservative water-film calculation for six planned
// circuits. No sensor is fabricated; a future trusted UE adapter must supply
// actual physical observations. No gameplay friction or scoring authority.
inline constexpr std::size_t ObservedWaterCellCount=256u;
inline constexpr double ObservedWaterMaximumMm=20.0;
inline constexpr double ObservedWaterMaximumRainMmH=200.0;
enum class ObservedWaterFault : std::uint8_t {
    None, InvalidSettings, InvalidCircuit, MissingSource,
    EpochMismatch, ClockRollback, ImpossiblePhysics, ConservationError
};
struct ObservedWaterSettings final {
    double MaximumFilmMm=ObservedWaterMaximumMm;
    double MaximumRainMmH=ObservedWaterMaximumRainMmH;
    double MaximumObservationAgeSeconds=2.0;
    double MinimumConfidence=0.85;
    double SurfaceDrainagePerSecond=0.06;
    double EvaporationMmPerSecond=0.0006;
    double AlongRoadFlowPerSecond=0.07;
    double MinimumStepSeconds=0.001;
    double MaximumStepSeconds=0.2;
    bool RequirePhysicalRoadSource=true;
    bool RequireRealMeasuredRain=true;
    bool RequireRealMeasuredFilm=true;
    bool NeverChangeChaosFriction=true;
    bool NeverAwardRaceResults=true;
    bool NeverSpawnWeatherActors=true;
};
struct ObservedWaterCell final {
    double StationCm=0.0;
    double RainMmH=0.0;
    double MeasuredFilmMm=0.0;
    double MeasurementClockSeconds=0.0;
    double Confidence=0.0;
    std::uint64_t RoadComponent=0u;
    bool OriginalCollisionSource=true;
    bool ActualRainMeasured=false;
    bool ActualFilmMeasured=false;
};
struct ObservedWaterFrame final {
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t SourceFrame=0u;
    double ClockSeconds=0.0;
    double DeltaSeconds=0.0;
    bool AllCellsMeasured=false;
    bool TrustedEngineWeatherSource=false;
    bool CollisionRoadUnchanged=true;
    std::array<ObservedWaterCell,ObservedWaterCellCount> Cells{};
};
struct ObservedWaterBudget final {
    double InitialFilmMm=0.0;
    double RainInMm=0.0;
    double EvaporationMm=0.0;
    double DrainedMm=0.0;
    double SpilledMm=0.0;
    double FinalFilmMm=0.0;
    double MassResidualMm=0.0;
    double MaximumFilmMm=0.0;
    double MinimumFilmMm=0.0;
    double MeanFilmMm=0.0;
    double MaximumExchangeMm=0.0;
    unsigned WetCells=0u;
    unsigned RainCells=0u;
    unsigned SaturatedCells=0u;
    bool NoActualPhysicsChange=true;
};
struct ObservedWaterResult final {
    bool Valid=false;
    ObservedWaterFault Fault=ObservedWaterFault::MissingSource;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t SourceFrame=0u;
    double ClockSeconds=0.0;
    double StepSeconds=0.0;
    double CellLengthCm=0.0;
    std::array<double,ObservedWaterCellCount> FilmMm{};
    ObservedWaterBudget Budget{};
    bool RenderAdvisoryOnly=true;
    bool OriginalRaceUnaffected=true;
};
inline bool ValidObservedWaterSettings(ObservedWaterSettings s) noexcept {
    return std::isfinite(s.MaximumFilmMm) &&
        s.MaximumFilmMm>=2.0 && s.MaximumFilmMm<=20.0 &&
        std::isfinite(s.MaximumRainMmH) &&
        s.MaximumRainMmH>=20.0 && s.MaximumRainMmH<=300.0 &&
        std::isfinite(s.MaximumObservationAgeSeconds) &&
        s.MaximumObservationAgeSeconds>0.0 &&
        s.MaximumObservationAgeSeconds<=10.0 &&
        std::isfinite(s.MinimumConfidence) &&
        s.MinimumConfidence>=0.50 && s.MinimumConfidence<=1.0 &&
        std::isfinite(s.SurfaceDrainagePerSecond) &&
        s.SurfaceDrainagePerSecond>=0.0 &&
        s.SurfaceDrainagePerSecond<=0.35 &&
        std::isfinite(s.EvaporationMmPerSecond) &&
        s.EvaporationMmPerSecond>=0.0 &&
        s.EvaporationMmPerSecond<=0.02 &&
        std::isfinite(s.AlongRoadFlowPerSecond) &&
        s.AlongRoadFlowPerSecond>=0.0 &&
        s.AlongRoadFlowPerSecond<=0.3 &&
        std::isfinite(s.MinimumStepSeconds) &&
        s.MinimumStepSeconds>=0.001 &&
        s.MinimumStepSeconds<=0.02 &&
        std::isfinite(s.MaximumStepSeconds) &&
        s.MaximumStepSeconds>=0.02 &&
        s.MaximumStepSeconds<=0.2 &&
        s.MaximumStepSeconds>s.MinimumStepSeconds &&
        s.RequirePhysicalRoadSource &&
        s.RequireRealMeasuredRain &&
        s.RequireRealMeasuredFilm &&
        s.NeverChangeChaosFriction &&
        s.NeverAwardRaceResults &&
        s.NeverSpawnWeatherActors;
}
inline bool ValidObservedWaterFrame(
    const WorldScaleVariantRecipe& circuit,
    const ObservedWaterFrame& frame,
    ObservedWaterSettings settings={}) noexcept
{
    if(!ValidObservedWaterSettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       frame.Circuit!=circuit.Id ||
       frame.Session==0u || frame.Revision==0u ||
       frame.SourceFrame==0u ||
       !std::isfinite(frame.ClockSeconds) ||
       frame.ClockSeconds<=0.0 ||
       frame.ClockSeconds>86400.0 ||
       !std::isfinite(frame.DeltaSeconds) ||
       frame.DeltaSeconds<settings.MinimumStepSeconds ||
       frame.DeltaSeconds>settings.MaximumStepSeconds ||
       !frame.AllCellsMeasured ||
       !frame.TrustedEngineWeatherSource ||
       !frame.CollisionRoadUnchanged)return false;
    const double length=circuit.LapLengthCm/
        static_cast<double>(ObservedWaterCellCount);
    for(std::size_t i=0u;i<ObservedWaterCellCount;++i){
        const auto& c=frame.Cells[i];
        const double age=frame.ClockSeconds-c.MeasurementClockSeconds;
        if(!std::isfinite(c.StationCm) ||
           !std::isfinite(c.RainMmH) ||
           !std::isfinite(c.MeasuredFilmMm) ||
           !std::isfinite(c.MeasurementClockSeconds) ||
           !std::isfinite(c.Confidence) ||
           !std::isfinite(age) ||
           std::abs(c.StationCm-i*length)>0.1 ||
           c.RainMmH<0.0 ||
           c.RainMmH>settings.MaximumRainMmH ||
           c.MeasuredFilmMm<0.0 ||
           c.MeasuredFilmMm>settings.MaximumFilmMm ||
           c.Confidence<settings.MinimumConfidence ||
           c.Confidence>1.0 ||
           age<0.0 ||
           age>settings.MaximumObservationAgeSeconds ||
           c.RoadComponent==0u ||
           !c.OriginalCollisionSource ||
           !c.ActualRainMeasured ||
           !c.ActualFilmMeasured) return false;
    }
    return true;
}
// Pairwise antisymmetric diffusion leaves the total initial+rain mass
// unchanged before explicit evaporation, drainage and spill sinks.
// Every transfer reads from one unchanged pre-flow snapshot.
inline ObservedWaterResult StepObservedWaterFilm(
    const WorldScaleVariantRecipe& circuit,
    const ObservedWaterFrame& frame,
    ObservedWaterSettings settings={}) noexcept
{
    ObservedWaterResult out;
    if(!ValidObservedWaterSettings(settings)){
        out.Fault=ObservedWaterFault::InvalidSettings;
        return out;
    }
    if(!ValidWorldScaleVariantRecipe(circuit) ||
       circuit.Id!=frame.Circuit){
        out.Fault=ObservedWaterFault::InvalidCircuit;
        return out;
    }
    if(!ValidObservedWaterFrame(circuit,frame,settings))
        return out;
    out.Circuit=circuit.Id;
    out.Session=frame.Session;
    out.Revision=frame.Revision;
    out.SourceFrame=frame.SourceFrame;
    out.ClockSeconds=frame.ClockSeconds;
    out.StepSeconds=frame.DeltaSeconds;
    out.CellLengthCm=circuit.LapLengthCm/
        static_cast<double>(ObservedWaterCellCount);
    const double dt=frame.DeltaSeconds;
    std::array<double,ObservedWaterCellCount> water{};
    std::array<double,ObservedWaterCellCount> exchange{};
    auto& b=out.Budget;
    b.MinimumFilmMm=std::numeric_limits<double>::infinity();
    for(std::size_t i=0u;i<ObservedWaterCellCount;++i){
        const auto& c=frame.Cells[i];
        const double rain=c.RainMmH*dt/3600.0;
        const double afterRain=c.MeasuredFilmMm+rain;
        const double evaporation=std::min(
            afterRain,settings.EvaporationMmPerSecond*dt);
        const double afterEvaporation=afterRain-evaporation;
        const double drainage=std::min(
            afterEvaporation,afterEvaporation*
                settings.SurfaceDrainagePerSecond*dt);
        water[i]=afterEvaporation-drainage;
        b.InitialFilmMm+=c.MeasuredFilmMm;
        b.RainInMm+=rain;
        b.EvaporationMm+=evaporation;
        b.DrainedMm+=drainage;
        if(c.RainMmH>0.0)++b.RainCells;
    }
    const double factor=settings.AlongRoadFlowPerSecond*dt;
    for(std::size_t i=0u;i<ObservedWaterCellCount;++i){
        const auto next=(i+1u)%ObservedWaterCellCount;
        const double amount=factor*(water[i]-water[next]);
        exchange[i]-=amount;
        exchange[next]+=amount;
        b.MaximumExchangeMm=std::max(
            b.MaximumExchangeMm,std::abs(amount));
    }
    for(std::size_t i=0u;i<ObservedWaterCellCount;++i){
        const double beforeCap=water[i]+exchange[i];
        if(!std::isfinite(beforeCap) || beforeCap< -1.0e-7){
            out.Fault=ObservedWaterFault::ImpossiblePhysics;
            return out;
        }
        const double nonnegative=std::max(0.0,beforeCap);
        const double film=std::min(
            settings.MaximumFilmMm,nonnegative);
        b.SpilledMm+=std::max(0.0,nonnegative-film);
        out.FilmMm[i]=film;
        b.FinalFilmMm+=film;
        b.MinimumFilmMm=std::min(b.MinimumFilmMm,film);
        b.MaximumFilmMm=std::max(b.MaximumFilmMm,film);
        if(film>0.05)++b.WetCells;
        if(film>=settings.MaximumFilmMm-1.0e-7)
            ++b.SaturatedCells;
    }
    b.MeanFilmMm=b.FinalFilmMm/
        static_cast<double>(ObservedWaterCellCount);
    const double expected=b.InitialFilmMm+b.RainInMm-
        b.EvaporationMm-b.DrainedMm-b.SpilledMm;
    b.MassResidualMm=b.FinalFilmMm-expected;
    if(!std::isfinite(b.MassResidualMm) ||
       std::abs(b.MassResidualMm)>1.0e-5*
           (1.0+b.InitialFilmMm*1e-3)){
        out.Fault=ObservedWaterFault::ConservationError;
        return out;
    }
    out.Fault=ObservedWaterFault::None;
    out.Valid=true;
    return out;
}
inline bool ObservedWaterContinuity(
    const ObservedWaterResult& a,
    const ObservedWaterResult& b) noexcept
{
    if(!a.Valid || !b.Valid ||
       a.Circuit!=b.Circuit ||
       a.Session!=b.Session ||
       a.Revision!=b.Revision ||
       b.SourceFrame<=a.SourceFrame ||
       b.SourceFrame-a.SourceFrame>8u ||
       b.ClockSeconds<=a.ClockSeconds ||
       !std::isfinite(a.ClockSeconds) ||
       !std::isfinite(b.ClockSeconds) ||
       !std::isfinite(b.StepSeconds))
        return false;
    const double dt=b.ClockSeconds-a.ClockSeconds;
    return dt>=0.001 && dt<=0.2 &&
        std::abs(dt-b.StepSeconds)<=0.03;
}
struct ObservedWaterStationSample final {
    bool Valid=false;
    double StationCm=0.0;
    double FilmMm=0.0;
    double Fraction=0.0;
    bool SourceOnly=true;
    bool NoChaosGripAuthority=true;
};
inline ObservedWaterStationSample SampleObservedWaterFilm(
    const WorldScaleVariantRecipe& circuit,
    const ObservedWaterResult& result,
    double stationCm) noexcept
{
    if(!result.Valid ||
       result.Circuit!=circuit.Id ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       !std::isfinite(stationCm) ||
       std::abs(stationCm)>1.0e9 ||
       result.CellLengthCm<=0.0)return {};
    const double wrapped=WorldScaleVariantWrapDistance(
        circuit,stationCm);
    if(!std::isfinite(wrapped))return {};
    const auto lower=std::min(
        static_cast<std::size_t>(wrapped/result.CellLengthCm),
        ObservedWaterCellCount-1u);
    const auto upper=(lower+1u)%ObservedWaterCellCount;
    const double fraction=std::clamp(
        (wrapped-lower*result.CellLengthCm)/
        result.CellLengthCm,0.0,1.0);
    const double film=(1.0-fraction)*result.FilmMm[lower]+
        fraction*result.FilmMm[upper];
    if(!std::isfinite(film) || film<0.0 ||
       film>ObservedWaterMaximumMm)return {};
    return {true,wrapped,film,fraction,true,true};
}
} // namespace latebrake
