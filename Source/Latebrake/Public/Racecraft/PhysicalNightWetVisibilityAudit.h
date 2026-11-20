#pragma once
#include "Racing/PhysicalSixCarReplayTimeline.h"
#include "Track/WorldScaleNightRaceLightingPlan.h"
#include "Track/WorldScaleObservedWaterFilm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Source-only race-night safety diagnostics. Trackside lux probes,
// actual Chaos chassis telemetry and original road water must agree.
// Estimating sight distance from light intensity is a conservative
// planning proxy, NOT a real shadow/occlusion or fog-driving visibility
// validation. It cannot steer, brake, flag, score or alter native lights.
inline constexpr unsigned NightWetVehicleCount=PhysicalReplayCarCount;
enum class NightWetSafetyBand : std::uint8_t {
    Unqualified, Comfortable, Caution, LimitedSight, Critical
};
enum class NightWetFault : std::uint8_t {
    None, InvalidPolicy, BadNightScene, MissingWater,
    MissingSource, MixedEpoch, InvalidProjection, UnsafeEstimate
};
struct NightWetSettings final {
    double MinimumRequiredSceneLux=0.5;
    double MaximumSourceTrackLux=800.0;
    double MinimumNativeHeadlampLux=0.0;
    double MaximumNativeHeadlampLux=250.0;
    double MinimumObservedBrakingCmps2=350.0;
    double MaximumObservedBrakingCmps2=2500.0;
    double MinimumDriverReactionSeconds=0.5;
    double MaximumDriverReactionSeconds=1.7;
    double MaximumFogLossFraction=0.85;
    double MaximumFilmMm=ObservedWaterMaximumMm;
    double MaximumSourceClockSkewSeconds=0.05;
    double MaximumVisibilityDistanceCm=12000.0;
    double MinimumVisibilityDistanceCm=300.0;
    double MinimumContrastLux=2.0;
    double WetReflectionContrastPenalty=0.55;
    double CautionReserveMultiplier=1.1;
    double CriticalReserveMultiplier=1.9;
    bool RequireActualRoadStation=true;
    bool RequireOriginalChassisAndLamp=true;
    bool DemandRealPhysicalBrakingWitness=true;
    bool NeverClaimRealOcclusion=true;
    bool NeverOperateChaosPhysics=true;
    bool NeverIssueRaceControlWarning=true;
};
struct NightWetVehicleSource final {
    std::uint64_t Actor=0u;
    std::uint64_t NativeLampComponent=0u;
    std::uint64_t SourceFrame=0u;
    double RoadStationCm=0.0;
    double MeasuredLowBeamLux=0.0;
    double VerifiedBrakeDecelCmps2=0.0;
    double SourceClockSeconds=0.0;
    double SensorConfidence=0.0;
    bool RealLampObserved=false;
    bool RealRoadProjectionObserved=false;
    bool OriginalChaosBrakeWitnessed=false;
    bool NoRaceAuthorityChange=true;
};
struct NightWetCarReadout final {
    bool Valid=false;
    std::uint64_t Actor=0u;
    double WrappedStationCm=0.0;
    double RoadWaterFilmMm=0.0;
    double SourceTrackLux=0.0;
    double SourceLampLux=0.0;
    double RoadFogTransmittance=1.0;
    double EstimatedSightDistanceCm=0.0;
    double MeasuredStoppingDistanceCm=0.0;
    double BrakingDistanceReserveCm=0.0;
    double ConservativeDistanceRatio=0.0;
    NightWetSafetyBand Band=NightWetSafetyBand::Unqualified;
    bool RealOcclusionUnverified=true;
    bool NoPhysicalBrakingApplied=true;
};
struct NightWetSafetyFleet final {
    bool Valid=false;
    NightWetFault Fault=NightWetFault::MissingSource;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    std::array<NightWetCarReadout,NightWetVehicleCount> Cars{};
    unsigned SourceQualifiedCars=0u;
    unsigned CautionCars=0u;
    unsigned CriticalCars=0u;
    double MinimumSightReserveCm=0.0;
    double MaximumObservedWaterMm=0.0;
    double MinimumWitnessedLux=0.0;
    bool OriginalRaceAuthorityUnchanged=true;
    bool OriginalCarCollisionUnchanged=true;
    bool SourceOnly=true;
};
inline bool ValidNightWetSettings(NightWetSettings s) noexcept {
    const auto finite=[](double value) noexcept {
        return std::isfinite(value);
    };
    return finite(s.MinimumRequiredSceneLux) &&
        s.MinimumRequiredSceneLux>=0.1 &&
        s.MinimumRequiredSceneLux<=3.0 &&
        finite(s.MaximumSourceTrackLux) &&
        s.MaximumSourceTrackLux>=500.0 &&
        s.MaximumSourceTrackLux<=1000.0 &&
        finite(s.MinimumNativeHeadlampLux) &&
        s.MinimumNativeHeadlampLux>=0.0 &&
        s.MinimumNativeHeadlampLux<=5.0 &&
        finite(s.MaximumNativeHeadlampLux) &&
        s.MaximumNativeHeadlampLux>=150.0 &&
        s.MaximumNativeHeadlampLux<=400.0 &&
        finite(s.MinimumObservedBrakingCmps2) &&
        s.MinimumObservedBrakingCmps2>=200.0 &&
        s.MinimumObservedBrakingCmps2<=700.0 &&
        finite(s.MaximumObservedBrakingCmps2) &&
        s.MaximumObservedBrakingCmps2>=1800.0 &&
        s.MaximumObservedBrakingCmps2<=3500.0 &&
        finite(s.MinimumDriverReactionSeconds) &&
        s.MinimumDriverReactionSeconds>=0.3 &&
        s.MinimumDriverReactionSeconds<=0.8 &&
        finite(s.MaximumDriverReactionSeconds) &&
        s.MaximumDriverReactionSeconds>=1.4 &&
        s.MaximumDriverReactionSeconds<=2.5 &&
        finite(s.MaximumFogLossFraction) &&
        s.MaximumFogLossFraction>=0.5 &&
        s.MaximumFogLossFraction<=0.95 &&
        finite(s.MaximumFilmMm) &&
        s.MaximumFilmMm>=5.0 &&
        s.MaximumFilmMm<=ObservedWaterMaximumMm &&
        finite(s.MaximumSourceClockSkewSeconds) &&
        s.MaximumSourceClockSkewSeconds>=0.0 &&
        s.MaximumSourceClockSkewSeconds<=0.1 &&
        finite(s.MaximumVisibilityDistanceCm) &&
        s.MaximumVisibilityDistanceCm>=5000.0 &&
        s.MaximumVisibilityDistanceCm<=15000.0 &&
        finite(s.MinimumVisibilityDistanceCm) &&
        s.MinimumVisibilityDistanceCm>=150.0 &&
        s.MinimumVisibilityDistanceCm<=600.0 &&
        finite(s.MinimumContrastLux) &&
        s.MinimumContrastLux>=1.0 &&
        s.MinimumContrastLux<=5.0 &&
        finite(s.WetReflectionContrastPenalty) &&
        s.WetReflectionContrastPenalty>=0.2 &&
        s.WetReflectionContrastPenalty<=0.75 &&
        finite(s.CautionReserveMultiplier) &&
        s.CautionReserveMultiplier>=1.0 &&
        s.CautionReserveMultiplier<=1.4 &&
        finite(s.CriticalReserveMultiplier) &&
        s.CriticalReserveMultiplier>=1.5 &&
        s.CriticalReserveMultiplier<=2.5 &&
        s.RequireActualRoadStation &&
        s.RequireOriginalChassisAndLamp &&
        s.DemandRealPhysicalBrakingWitness &&
        s.NeverClaimRealOcclusion &&
        s.NeverOperateChaosPhysics &&
        s.NeverIssueRaceControlWarning;
}
inline bool ValidNightWetVehicleSource(
    const PhysicalReplayCarSample& car,
    NightWetVehicleSource witness,
    NightWetSettings s) noexcept
{
    if(!FinitePhysicalReplayCar(car) ||
       witness.Actor!=car.Actor ||
       witness.NativeLampComponent==0u ||
       witness.SourceFrame!=car.SourceFrame ||
       !std::isfinite(witness.RoadStationCm) ||
       std::abs(witness.RoadStationCm)>1e9 ||
       !std::isfinite(witness.MeasuredLowBeamLux) ||
       witness.MeasuredLowBeamLux<s.MinimumNativeHeadlampLux ||
       witness.MeasuredLowBeamLux>s.MaximumNativeHeadlampLux ||
       !std::isfinite(witness.VerifiedBrakeDecelCmps2) ||
       witness.VerifiedBrakeDecelCmps2<
           s.MinimumObservedBrakingCmps2 ||
       witness.VerifiedBrakeDecelCmps2>
           s.MaximumObservedBrakingCmps2 ||
       !std::isfinite(witness.SourceClockSeconds) ||
       !std::isfinite(witness.SensorConfidence) ||
       witness.SensorConfidence<0.85 ||
       witness.SensorConfidence>1.0 ||
       !witness.RealLampObserved ||
       !witness.RealRoadProjectionObserved ||
       !witness.OriginalChaosBrakeWitnessed ||
       !witness.NoRaceAuthorityChange)
        return false;
    return true;
}
inline NightWetCarReadout EvaluateNightWetCar(
    const WorldScaleVariantRecipe& circuit,
    const NightRaceLightingPlan& lights,
    const ObservedWaterResult& water,
    const PhysicalReplayCarSample& car,
    NightWetVehicleSource source,
    NightWetSettings settings={}) noexcept
{
    NightWetCarReadout out;
    if(!ValidNightWetSettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       !lights.Valid || lights.Circuit!=circuit.Id ||
       !water.Valid || water.Circuit!=circuit.Id ||
       !ValidNightWetVehicleSource(car,source,settings))
        return out;
    const auto wet=SampleObservedWaterFilm(
        circuit,water,source.RoadStationCm);
    const double station=WorldScaleVariantWrapDistance(
        circuit,source.RoadStationCm);
    if(!wet.Valid || !std::isfinite(station))return out;
    const auto index=std::min(
        static_cast<std::size_t>(
            station/circuit.LapLengthCm*
                NightRaceSurveyStations),
        static_cast<std::size_t>(NightRaceSurveyStations-1u));
    const auto& probe=lights.Probes[index];
    if(!probe.Valid ||
       !probe.NoDriverVisibilityAuthority ||
       probe.ActualNativeShadowTested ||
       !std::isfinite(probe.EstimatedIlluminanceLux) ||
       !std::isfinite(probe.EstimatedFogTransmittance) ||
       probe.EstimatedIlluminanceLux<0.0 ||
       probe.EstimatedIlluminanceLux>
           settings.MaximumSourceTrackLux ||
       probe.EstimatedFogTransmittance<0.0 ||
       probe.EstimatedFogTransmittance>1.0)
        return out;
    const double measuredSpeed=PhysicalReplaySpeed(car);
    const double safeBraking=source.VerifiedBrakeDecelCmps2;
    const double reaction=settings.MaximumDriverReactionSeconds;
    const double requiredStopping=measuredSpeed*reaction+
        measuredSpeed*measuredSpeed/(2.0*safeBraking);
    // A strictly conservative optical heuristic based on a measured
    // lamp and already qualified source-proposal track lux. It does
    // not assert a geometric line-of-sight, nor ray cast the engine.
    const double lux=std::max(
        0.0,probe.EstimatedIlluminanceLux+
            source.MeasuredLowBeamLux);
    const double wetFraction=std::clamp(
        wet.FilmMm/settings.MaximumFilmMm,0.0,1.0);
    const double contrast=1.0-
        settings.WetReflectionContrastPenalty*wetFraction;
    const double fog=std::clamp(
        probe.EstimatedFogTransmittance,0.0,1.0);
    const double distance=settings.MinimumVisibilityDistanceCm+
        900.0*std::sqrt(lux/settings.MinimumContrastLux)*
        contrast*fog;
    const double bounded=std::clamp(
        distance,settings.MinimumVisibilityDistanceCm,
        settings.MaximumVisibilityDistanceCm);
    if(!std::isfinite(requiredStopping) ||
       !std::isfinite(bounded) ||
       requiredStopping<0.0)return out;
    const double ratio=requiredStopping/
        std::max(1.0,bounded);
    out.Valid=true;
    out.Actor=car.Actor;
    out.WrappedStationCm=station;
    out.RoadWaterFilmMm=wet.FilmMm;
    out.SourceTrackLux=probe.EstimatedIlluminanceLux;
    out.SourceLampLux=source.MeasuredLowBeamLux;
    out.RoadFogTransmittance=fog;
    out.EstimatedSightDistanceCm=bounded;
    out.MeasuredStoppingDistanceCm=requiredStopping;
    out.BrakingDistanceReserveCm=bounded-requiredStopping;
    out.ConservativeDistanceRatio=ratio;
    out.Band=ratio>=settings.CriticalReserveMultiplier ?
        NightWetSafetyBand::Critical:
        ratio>=settings.CautionReserveMultiplier ?
            NightWetSafetyBand::LimitedSight:
            ratio>=0.85?NightWetSafetyBand::Caution:
            NightWetSafetyBand::Comfortable;
    return std::isfinite(ratio) ? out:NightWetCarReadout{};
}
inline NightWetSafetyFleet AuditNightWetSixCarSafety(
    const WorldScaleVariantRecipe& circuit,
    const NightRaceLightingPlan& lighting,
    const ObservedWaterResult& water,
    const PhysicalReplayFrame& fleet,
    const std::array<NightWetVehicleSource,
        NightWetVehicleCount>& observations,
    NightWetSettings settings={}) noexcept
{
    NightWetSafetyFleet out;
    out.Fault=NightWetFault::MissingSource;
    if(!ValidNightWetSettings(settings)){
        out.Fault=NightWetFault::InvalidPolicy;
        return out;
    }
    if(!ValidWorldScaleVariantRecipe(circuit) ||
       !lighting.Valid || lighting.Circuit!=circuit.Id ||
       !lighting.UnrealRenderingUnchanged ||
       !lighting.OriginalPhysicalRoadUnchanged ||
       !lighting.OfficialRaceStateUnchanged ||
       lighting.QualifiedSurveyStations!=NightRaceSurveyStations){
        out.Fault=NightWetFault::BadNightScene;
        return out;
    }
    if(!water.Valid || water.Circuit!=circuit.Id ||
       !water.RenderAdvisoryOnly ||
       !water.OriginalRaceUnaffected){
        out.Fault=NightWetFault::MissingWater;
        return out;
    }
    if(ValidatePhysicalReplayFrame(fleet)!=PhysicalReplayReject::None)
        return out;
    if(lighting.SourceSession!=fleet.Session ||
       lighting.SourceFrame!=fleet.SourceFrame ||
       water.Session!=fleet.Session ||
       water.Revision!=fleet.SessionRevision ||
       water.SourceFrame!=fleet.SourceFrame ||
       !std::isfinite(lighting.SourceClockSeconds) ||
       !std::isfinite(water.ClockSeconds) ||
       std::abs(lighting.SourceClockSeconds-
           fleet.ActiveClockSeconds)>
           settings.MaximumSourceClockSkewSeconds ||
       std::abs(water.ClockSeconds-fleet.ActiveClockSeconds)>
           settings.MaximumSourceClockSkewSeconds){
        out.Fault=NightWetFault::MixedEpoch;
        return out;
    }
    out.Circuit=circuit.Id;
    out.Session=fleet.Session;
    out.Revision=fleet.SessionRevision;
    out.SourceFrame=fleet.SourceFrame;
    out.SourceClockSeconds=fleet.ActiveClockSeconds;
    out.MinimumSightReserveCm=
        std::numeric_limits<double>::infinity();
    out.MinimumWitnessedLux=
        std::numeric_limits<double>::infinity();
    for(unsigned i=0u;i<NightWetVehicleCount;++i){
        const auto& source=observations[i];
        if(!ValidNightWetVehicleSource(fleet.Cars[i],source,settings) ||
           std::abs(source.SourceClockSeconds-
               fleet.ActiveClockSeconds)>
               settings.MaximumSourceClockSkewSeconds){
            out.Fault=NightWetFault::InvalidProjection;
            return out;
        }
        for(unsigned previous=0u;previous<i;++previous)
            if(source.NativeLampComponent==
               observations[previous].NativeLampComponent){
                out.Fault=NightWetFault::InvalidProjection;
                return out;
            }
        auto car=EvaluateNightWetCar(
            circuit,lighting,water,fleet.Cars[i],source,settings);
        if(!car.Valid){
            out.Fault=NightWetFault::UnsafeEstimate;
            return out;
        }
        out.Cars[i]=car;
        ++out.SourceQualifiedCars;
        out.MinimumSightReserveCm=std::min(
            out.MinimumSightReserveCm,
            car.BrakingDistanceReserveCm);
        out.MaximumObservedWaterMm=std::max(
            out.MaximumObservedWaterMm,car.RoadWaterFilmMm);
        out.MinimumWitnessedLux=std::min(
            out.MinimumWitnessedLux,car.SourceTrackLux);
        if(car.Band==NightWetSafetyBand::Critical)
            ++out.CriticalCars;
        if(car.Band==NightWetSafetyBand::Caution ||
           car.Band==NightWetSafetyBand::LimitedSight ||
           car.Band==NightWetSafetyBand::Critical)
            ++out.CautionCars;
    }
    out.Valid=out.SourceQualifiedCars==NightWetVehicleCount &&
        std::isfinite(out.MinimumSightReserveCm) &&
        std::isfinite(out.MinimumWitnessedLux);
    out.Fault=out.Valid?NightWetFault::None:
        NightWetFault::UnsafeEstimate;
    return out;
}
} // namespace latebrake
