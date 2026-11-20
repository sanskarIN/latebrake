#pragma once
#include "Racing/PhysicalSixCarReplayTimeline.h"
#include "Track/WorldScaleObservedWaterFilm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Six-car, four-wheel hydroplaning RISK ADVISORY from ACTUAL native tire
// sensor readings, measured water film and verified source identity.
// The thresholds are provisional research heuristics, NOT an approved tire
// physical force model. This code CANNOT change Chaos brake, grip, assist,
// tire contact, marshal penalties, official time or car steering.
inline constexpr unsigned WaterRiskWheelCount=4u;
inline constexpr unsigned WaterRiskCarCount=PhysicalReplayCarCount;
inline constexpr unsigned WaterRiskSampleCount=
    WaterRiskWheelCount*WaterRiskCarCount;
enum class WaterRiskBand : std::uint8_t {
    Unqualified, Dry, Low, Elevated, Severe
};
struct WaterRiskSettings final {
    double MinimumPressureKpa=120.0;
    double MaximumPressureKpa=450.0;
    double MinimumTreadDepthMm=0.4;
    double MaximumTreadDepthMm=10.0;
    double MinimumStaticWheelLoadN=600.0;
    double MaximumStaticWheelLoadN=15000.0;
    double MaximumFilmMm=ObservedWaterMaximumMm;
    double MaximumMeasuredSpeedCmps=PhysicalReplayMaximumSpeedCmps;
    double MaximumSourceClockSkewSeconds=0.05;
    double MinimumObservationConfidence=0.85;
    double WetRiskStartFilmMm=0.20;
    double WetRiskFullFilmMm=5.0;
    double PressureThresholdScale=165.0;
    double MinimumTreadMultiplier=0.55;
    double MaximumTreadMultiplier=1.25;
    double MaximumLoadMultiplier=1.2;
    bool AllWheelsMustHaveRealNativeContact=true;
    bool RequireActualPressureAndTreadSensors=true;
    bool OriginalRoadPhysicsMustRemain=true;
    bool NoActualTyreForceAuthority=true;
    bool NoAutomaticGameSteeringOrBraking=true;
};
struct WaterRiskWheelObservation final {
    std::uint64_t OwnerActor=0u;
    std::uint64_t NativeWheelComponent=0u;
    std::uint64_t SourceFrame=0u;
    double PressureKpa=0.0;
    double TreadDepthMm=0.0;
    double MeasuredLoadN=0.0;
    double ContactStationCm=0.0;
    double SourceClockSeconds=0.0;
    double Confidence=0.0;
    bool OriginalChaosWheelObserved=false;
    bool NativeRoadContactObserved=false;
    bool ActualPressureSensorObserved=false;
    bool ActualTreadReadingObserved=false;
};
struct WaterRiskWheelResult final {
    bool Valid=false;
    std::uint64_t OwnerActor=0u;
    std::uint64_t NativeWheelComponent=0u;
    unsigned WheelIndex=0u;
    double WaterFilmMm=0.0;
    double MeasuredSpeedCmps=0.0;
    double NominalHydroThresholdCmps=0.0;
    double WetSurfaceThresholdCmps=0.0;
    double SpeedThresholdFraction=0.0;
    double RiskFraction=0.0;
    WaterRiskBand Band=WaterRiskBand::Unqualified;
    bool SourceOnly=true;
    bool ActualWheelForceUnchanged=true;
};
struct WaterRiskCarResult final {
    bool Valid=false;
    std::uint64_t Actor=0u;
    std::array<WaterRiskWheelResult,WaterRiskWheelCount> Wheels{};
    double MaximumWheelRisk=0.0;
    double MeanWheelRisk=0.0;
    unsigned SevereWheelCount=0u;
    bool NoNativeDriveInputChanged=true;
};
struct WaterRiskFleetPlan final {
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    std::array<WaterRiskCarResult,WaterRiskCarCount> Cars{};
    double MaximumFleetRisk=0.0;
    double MeanFleetRisk=0.0;
    unsigned SeverelyAtRiskCars=0u;
    unsigned VerifiedWheelSources=0u;
    bool NoRealPhysicsForceChanged=true;
    bool NoRaceAuthorityChanged=true;
};
inline bool ValidWaterRiskSettings(WaterRiskSettings s) noexcept {
    return std::isfinite(s.MinimumPressureKpa) &&
        s.MinimumPressureKpa>=100.0 &&
        s.MinimumPressureKpa<=200.0 &&
        std::isfinite(s.MaximumPressureKpa) &&
        s.MaximumPressureKpa>=300.0 &&
        s.MaximumPressureKpa<=500.0 &&
        s.MaximumPressureKpa>s.MinimumPressureKpa &&
        std::isfinite(s.MinimumTreadDepthMm) &&
        s.MinimumTreadDepthMm>=0.2 &&
        s.MinimumTreadDepthMm<=1.0 &&
        std::isfinite(s.MaximumTreadDepthMm) &&
        s.MaximumTreadDepthMm>=7.0 &&
        s.MaximumTreadDepthMm<=12.0 &&
        std::isfinite(s.MinimumStaticWheelLoadN) &&
        s.MinimumStaticWheelLoadN>=300.0 &&
        s.MinimumStaticWheelLoadN<=1000.0 &&
        std::isfinite(s.MaximumStaticWheelLoadN) &&
        s.MaximumStaticWheelLoadN>=10000.0 &&
        s.MaximumStaticWheelLoadN<=20000.0 &&
        std::isfinite(s.MaximumFilmMm) &&
        s.MaximumFilmMm>=5.0 &&
        s.MaximumFilmMm<=ObservedWaterMaximumMm &&
        std::isfinite(s.MaximumMeasuredSpeedCmps) &&
        s.MaximumMeasuredSpeedCmps>=10000.0 &&
        s.MaximumMeasuredSpeedCmps<=15000.0 &&
        std::isfinite(s.MaximumSourceClockSkewSeconds) &&
        s.MaximumSourceClockSkewSeconds>=0.0 &&
        s.MaximumSourceClockSkewSeconds<=0.1 &&
        std::isfinite(s.MinimumObservationConfidence) &&
        s.MinimumObservationConfidence>=0.5 &&
        s.MinimumObservationConfidence<=1.0 &&
        std::isfinite(s.WetRiskStartFilmMm) &&
        s.WetRiskStartFilmMm>=0.05 &&
        s.WetRiskStartFilmMm<=0.5 &&
        std::isfinite(s.WetRiskFullFilmMm) &&
        s.WetRiskFullFilmMm>=2.0 &&
        s.WetRiskFullFilmMm<=8.0 &&
        s.WetRiskFullFilmMm>s.WetRiskStartFilmMm &&
        std::isfinite(s.PressureThresholdScale) &&
        s.PressureThresholdScale>=100.0 &&
        s.PressureThresholdScale<=220.0 &&
        std::isfinite(s.MinimumTreadMultiplier) &&
        s.MinimumTreadMultiplier>=0.3 &&
        s.MinimumTreadMultiplier<=0.8 &&
        std::isfinite(s.MaximumTreadMultiplier) &&
        s.MaximumTreadMultiplier>=1.0 &&
        s.MaximumTreadMultiplier<=1.5 &&
        std::isfinite(s.MaximumLoadMultiplier) &&
        s.MaximumLoadMultiplier>=1.0 &&
        s.MaximumLoadMultiplier<=1.5 &&
        s.AllWheelsMustHaveRealNativeContact &&
        s.RequireActualPressureAndTreadSensors &&
        s.OriginalRoadPhysicsMustRemain &&
        s.NoActualTyreForceAuthority &&
        s.NoAutomaticGameSteeringOrBraking;
}
inline bool ValidWaterRiskWheelObservation(
    const PhysicalReplayCarSample& car,
    WaterRiskWheelObservation wheel,
    const ObservedWaterResult& water,
    WaterRiskSettings settings) noexcept
{
    if(!ValidWaterRiskSettings(settings) ||
       !car.ActualChaosBodyObserved ||
       car.Actor==0u ||
       wheel.OwnerActor!=car.Actor ||
       wheel.NativeWheelComponent==0u ||
       wheel.SourceFrame!=car.SourceFrame ||
       !wheel.OriginalChaosWheelObserved ||
       !wheel.NativeRoadContactObserved ||
       !wheel.ActualPressureSensorObserved ||
       !wheel.ActualTreadReadingObserved ||
       !std::isfinite(wheel.PressureKpa) ||
       !std::isfinite(wheel.TreadDepthMm) ||
       !std::isfinite(wheel.MeasuredLoadN) ||
       !std::isfinite(wheel.ContactStationCm) ||
       !std::isfinite(wheel.SourceClockSeconds) ||
       !std::isfinite(wheel.Confidence) ||
       wheel.PressureKpa<settings.MinimumPressureKpa ||
       wheel.PressureKpa>settings.MaximumPressureKpa ||
       wheel.TreadDepthMm<settings.MinimumTreadDepthMm ||
       wheel.TreadDepthMm>settings.MaximumTreadDepthMm ||
       wheel.MeasuredLoadN<settings.MinimumStaticWheelLoadN ||
       wheel.MeasuredLoadN>settings.MaximumStaticWheelLoadN ||
       wheel.Confidence<settings.MinimumObservationConfidence ||
       wheel.Confidence>1.0 ||
       !std::isfinite(water.ClockSeconds) ||
       std::abs(wheel.SourceClockSeconds-water.ClockSeconds)>
           settings.MaximumSourceClockSkewSeconds)
        return false;
    return true;
}
// Threshold is intentionally an advisory, bounded heuristic calibrated
// against pressure, tread and wheel load. It is NOT a native force law.
inline WaterRiskWheelResult EstimateWaterWheelRisk(
    const WorldScaleVariantRecipe& circuit,
    const PhysicalReplayCarSample& car,
    const ObservedWaterResult& water,
    WaterRiskWheelObservation wheel,
    unsigned wheelIndex,
    WaterRiskSettings settings={}) noexcept
{
    WaterRiskWheelResult out;
    if(wheelIndex>=WaterRiskWheelCount ||
       !FinitePhysicalReplayCar(car) ||
       !water.Valid ||
       water.Circuit!=circuit.Id ||
       !water.RenderAdvisoryOnly ||
       !ValidWaterRiskWheelObservation(car,wheel,water,settings))
        return out;
    const auto film=SampleObservedWaterFilm(
        circuit,water,wheel.ContactStationCm);
    const double speed=PhysicalReplaySpeed(car);
    if(!film.Valid ||
       !std::isfinite(speed) ||
       speed>settings.MaximumMeasuredSpeedCmps)
        return out;
    const double treadFraction=std::clamp(
        wheel.TreadDepthMm/settings.MaximumTreadDepthMm,
        0.0,1.0);
    const double treadMultiplier=
        settings.MinimumTreadMultiplier+
        treadFraction*(settings.MaximumTreadMultiplier-
                       settings.MinimumTreadMultiplier);
    const double loadFraction=std::clamp(
        wheel.MeasuredLoadN/settings.MaximumStaticWheelLoadN,
        0.0,1.0);
    const double loadMultiplier=1.0+
        (settings.MaximumLoadMultiplier-1.0)*loadFraction;
    const double pressureSpeed=settings.PressureThresholdScale*
        std::sqrt(wheel.PressureKpa);
    const double dryThreshold=pressureSpeed*treadMultiplier*
        loadMultiplier;
    const double depthFraction=std::clamp(
        (film.FilmMm-settings.WetRiskStartFilmMm)/
        (settings.WetRiskFullFilmMm-settings.WetRiskStartFilmMm),
        0.0,1.0);
    const double wetThreshold=dryThreshold*
        (1.0-0.38*depthFraction);
    if(!std::isfinite(dryThreshold) ||
       !std::isfinite(wetThreshold) ||
       wetThreshold<=0.0)
        return out;
    // No wet film means NO claimed hydroplaning, even at high speed.
    const double speedRatio=speed/wetThreshold;
    const double excess=std::clamp(
        (speedRatio-0.65)/(1.4-0.65),0.0,1.0);
    const double risk=depthFraction*excess;
    out.Valid=true;
    out.OwnerActor=car.Actor;
    out.NativeWheelComponent=wheel.NativeWheelComponent;
    out.WheelIndex=wheelIndex;
    out.WaterFilmMm=film.FilmMm;
    out.MeasuredSpeedCmps=speed;
    out.NominalHydroThresholdCmps=dryThreshold;
    out.WetSurfaceThresholdCmps=wetThreshold;
    out.SpeedThresholdFraction=speedRatio;
    out.RiskFraction=risk;
    out.Band=depthFraction==0.0 ? WaterRiskBand::Dry :
        risk>=0.75 ? WaterRiskBand::Severe :
        risk>=0.35 ? WaterRiskBand::Elevated : WaterRiskBand::Low;
    return std::isfinite(risk) ? out : WaterRiskWheelResult{};
}
inline WaterRiskFleetPlan BuildObservedWaterRiskFleet(
    const WorldScaleVariantRecipe& circuit,
    const PhysicalReplayFrame& fleet,
    const ObservedWaterResult& water,
    const std::array<WaterRiskWheelObservation,
        WaterRiskSampleCount>& measurements,
    WaterRiskSettings settings={}) noexcept
{
    WaterRiskFleetPlan out;
    if(!ValidWaterRiskSettings(settings) ||
       ValidatePhysicalReplayFrame(fleet)!=PhysicalReplayReject::None ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       !water.Valid ||
       water.Circuit!=circuit.Id ||
       water.Session!=fleet.Session ||
       water.Revision!=fleet.SessionRevision ||
       water.SourceFrame!=fleet.SourceFrame ||
       !water.OriginalRaceUnaffected ||
       !water.RenderAdvisoryOnly ||
       !std::isfinite(water.ClockSeconds) ||
       std::abs(water.ClockSeconds-fleet.ActiveClockSeconds)>
           settings.MaximumSourceClockSkewSeconds)
        return out;
    out.Circuit=circuit.Id;
    out.Session=fleet.Session;
    out.Revision=fleet.SessionRevision;
    out.SourceFrame=fleet.SourceFrame;
    out.SourceClockSeconds=water.ClockSeconds;
    for(unsigned i=0u;i<WaterRiskCarCount;++i){
        WaterRiskCarResult car;
        car.Actor=fleet.Cars[i].Actor;
        for(unsigned wheel=0u;wheel<WaterRiskWheelCount;++wheel){
            const unsigned k=i*WaterRiskWheelCount+wheel;
            const auto& source=measurements[k];
            const auto risk=EstimateWaterWheelRisk(
                circuit,fleet.Cars[i],water,source,wheel,settings);
            if(!risk.Valid)return {};
            // A single native wheel component must not be duplicated
            // across this frame, regardless of car identity.
            for(unsigned previous=0u;previous<k;++previous)
                if(source.NativeWheelComponent==
                   measurements[previous].NativeWheelComponent)
                    return {};
            car.Wheels[wheel]=risk;
            car.MaximumWheelRisk=std::max(
                car.MaximumWheelRisk,risk.RiskFraction);
            car.MeanWheelRisk+=risk.RiskFraction;
            ++out.VerifiedWheelSources;
        }
        car.MeanWheelRisk/=static_cast<double>(WaterRiskWheelCount);
        for(const auto& w:car.Wheels)
            if(w.Band==WaterRiskBand::Severe)++car.SevereWheelCount;
        car.Valid=true;
        out.Cars[i]=car;
        out.MeanFleetRisk+=car.MeanWheelRisk;
        out.MaximumFleetRisk=std::max(
            out.MaximumFleetRisk,car.MaximumWheelRisk);
        if(car.SevereWheelCount>0u)++out.SeverelyAtRiskCars;
    }
    out.MeanFleetRisk/=static_cast<double>(WaterRiskCarCount);
    out.Valid=out.VerifiedWheelSources==WaterRiskSampleCount &&
        out.MaximumFleetRisk<=1.0 &&
        out.MeanFleetRisk>=0.0 &&
        std::isfinite(out.MeanFleetRisk);
    return out;
}
} // namespace latebrake
