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
// Portable visual-only spray planning for SIX real physical chassis.
// It does not apply drag, hydroplaning, damage, tire force or camera
// occlusion; no synthetic visibility metric can overrule native gameplay.
inline constexpr unsigned WetSprayFleetSize=6u;
inline constexpr unsigned WetSprayPairCount=30u;
inline constexpr double WetSprayRadiansPerDegree=
    0.01745329251994329576923690768489;
struct WetSpraySettings final {
    double MinimumWetFilmMm=0.06;
    double MaximumWetFilmMm=15.0;
    double MinimumSpraySpeedCmps=400.0;
    double FullSpraySpeedCmps=5500.0;
    double MaximumMeasuredSpeedCmps=15000.0;
    double MaximumSourceClockSkewSeconds=0.05;
    double MaximumSprayRangeCm=3500.0;
    double MaximumLateralSprayCm=700.0;
    double SprayConeHalfAngleDegrees=25.0;
    double MaximumParticleIntensity=1.0;
    double MinimumEffectiveIntensity=0.02;
    unsigned MaximumParticlesPerVehicle=900u;
    unsigned MaximumTotalParticles=2500u;
    bool RequireRealChaosSource=true;
    bool RequireMeasuredNativeRoadWater=true;
    bool NeverModifyCollision=true;
    bool NeverClaimRealCameraOcclusion=true;
    bool NeverChangeRaceClassification=true;
};
struct WetSprayVehicleObservation final {
    std::uint64_t Actor=0u;
    double MeasuredStationCm=0.0;
    double ClockSeconds=0.0;
    bool NativeRoadProjectionObserved=false;
    bool PhysicalTireContactObserved=false;
    bool OriginalRaceClockUnchanged=true;
};
struct WetSprayEmitter final {
    bool Valid=false;
    std::uint64_t Actor=0u;
    double Xcm=0.0;
    double Ycm=0.0;
    double Zcm=0.0;
    double HeadingRadians=0.0;
    double SourceFilmMm=0.0;
    double ActualSpeedCmps=0.0;
    double Intensity=0.0;
    double RecommendedRangeCm=0.0;
    unsigned ProposedParticleBudget=0u;
    bool SourceOnly=true;
    bool UnrealParticleSystemCreated=false;
};
struct WetSprayExposure final {
    bool Valid=false;
    std::uint64_t EmitterActor=0u;
    std::uint64_t FollowingActor=0u;
    double AlongSprayCm=0.0;
    double LateralSprayCm=0.0;
    double EuclideanGapCm=0.0;
    double ObservedIntensity=0.0;
    double OcclusionAdvisoryFraction=0.0;
    bool NoVisibilityGameplayAuthority=true;
};
struct WetSprayFramePlan final {
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Frame=0u;
    double ClockSeconds=0.0;
    std::array<WetSprayEmitter,WetSprayFleetSize> Emitters{};
    std::array<WetSprayExposure,WetSprayPairCount> Exposures{};
    unsigned VerifiedEmitterCount=0u;
    unsigned ActiveEmitterCount=0u;
    unsigned ActiveExposureCount=0u;
    unsigned ProposedParticleCount=0u;
    double PeakExposure=0.0;
    double TotalIntensity=0.0;
    bool ActualOcclusionUnverified=true;
    bool NoPhysicsOrRaceChanges=true;
};
inline bool ValidWetSpraySettings(WetSpraySettings p) noexcept {
    return std::isfinite(p.MinimumWetFilmMm) &&
        p.MinimumWetFilmMm>=0.01 && p.MinimumWetFilmMm<=0.5 &&
        std::isfinite(p.MaximumWetFilmMm) &&
        p.MaximumWetFilmMm>=p.MinimumWetFilmMm+1.0 &&
        p.MaximumWetFilmMm<=ObservedWaterMaximumMm &&
        std::isfinite(p.MinimumSpraySpeedCmps) &&
        p.MinimumSpraySpeedCmps>=100.0 &&
        p.MinimumSpraySpeedCmps<=1200.0 &&
        std::isfinite(p.FullSpraySpeedCmps) &&
        p.FullSpraySpeedCmps>=p.MinimumSpraySpeedCmps+1000.0 &&
        p.FullSpraySpeedCmps<=8000.0 &&
        std::isfinite(p.MaximumMeasuredSpeedCmps) &&
        p.MaximumMeasuredSpeedCmps>=10000.0 &&
        p.MaximumMeasuredSpeedCmps<=15000.0 &&
        std::isfinite(p.MaximumSourceClockSkewSeconds) &&
        p.MaximumSourceClockSkewSeconds>=0.0 &&
        p.MaximumSourceClockSkewSeconds<=0.1 &&
        std::isfinite(p.MaximumSprayRangeCm) &&
        p.MaximumSprayRangeCm>=1000.0 &&
        p.MaximumSprayRangeCm<=5000.0 &&
        std::isfinite(p.MaximumLateralSprayCm) &&
        p.MaximumLateralSprayCm>=300.0 &&
        p.MaximumLateralSprayCm<=1000.0 &&
        std::isfinite(p.SprayConeHalfAngleDegrees) &&
        p.SprayConeHalfAngleDegrees>=10.0 &&
        p.SprayConeHalfAngleDegrees<=50.0 &&
        std::isfinite(p.MaximumParticleIntensity) &&
        p.MaximumParticleIntensity>=0.1 &&
        p.MaximumParticleIntensity<=1.0 &&
        std::isfinite(p.MinimumEffectiveIntensity) &&
        p.MinimumEffectiveIntensity>=0.001 &&
        p.MinimumEffectiveIntensity<=0.1 &&
        p.MaximumParticlesPerVehicle>=100u &&
        p.MaximumParticlesPerVehicle<=2000u &&
        p.MaximumTotalParticles>=p.MaximumParticlesPerVehicle &&
        p.MaximumTotalParticles<=5000u &&
        p.RequireRealChaosSource &&
        p.RequireMeasuredNativeRoadWater &&
        p.NeverModifyCollision &&
        p.NeverClaimRealCameraOcclusion &&
        p.NeverChangeRaceClassification;
}
inline bool ValidateWetSprayProvenance(
    const WorldScaleVariantRecipe& circuit,
    const PhysicalReplayFrame& fleet,
    const ObservedWaterResult& water,
    const std::array<WetSprayVehicleObservation,
       WetSprayFleetSize>& sources,
    WetSpraySettings settings) noexcept
{
    if(!ValidWetSpraySettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       ValidatePhysicalReplayFrame(fleet)!=PhysicalReplayReject::None ||
       !water.Valid || water.Circuit!=circuit.Id ||
       !water.RenderAdvisoryOnly || !water.OriginalRaceUnaffected ||
       water.Session!=fleet.Session ||
       water.Revision!=fleet.SessionRevision ||
       water.SourceFrame!=fleet.SourceFrame ||
       !std::isfinite(water.ClockSeconds) ||
       std::abs(water.ClockSeconds-fleet.ActiveClockSeconds)>
           settings.MaximumSourceClockSkewSeconds)
        return false;
    for(unsigned i=0u;i<WetSprayFleetSize;++i) {
        const auto& car=fleet.Cars[i];
        const auto& obs=sources[i];
        if(obs.Actor!=car.Actor ||
           !obs.NativeRoadProjectionObserved ||
           !obs.PhysicalTireContactObserved ||
           !obs.OriginalRaceClockUnchanged ||
           !std::isfinite(obs.MeasuredStationCm) ||
           !std::isfinite(obs.ClockSeconds) ||
           std::abs(obs.MeasuredStationCm)>1.0e9 ||
           std::abs(obs.ClockSeconds-fleet.ActiveClockSeconds)>
               settings.MaximumSourceClockSkewSeconds ||
           !SampleObservedWaterFilm(
               circuit,water,obs.MeasuredStationCm).Valid)
            return false;
    }
    return true;
}
inline WetSprayEmitter BuildWetSprayEmitter(
    const WorldScaleVariantRecipe& circuit,
    const PhysicalReplayCarSample& car,
    const ObservedWaterResult& water,
    WetSprayVehicleObservation obs,
    WetSpraySettings settings) noexcept
{
    WetSprayEmitter out;
    if(!FinitePhysicalReplayCar(car) ||
       car.Actor!=obs.Actor ||
       !obs.NativeRoadProjectionObserved ||
       !obs.PhysicalTireContactObserved ||
       !ValidWetSpraySettings(settings))
        return out;
    const auto witness=SampleObservedWaterFilm(
        circuit,water,obs.MeasuredStationCm);
    if(!witness.Valid)return out;
    const double speed=PhysicalReplaySpeed(car);
    if(!std::isfinite(speed) ||
       speed>settings.MaximumMeasuredSpeedCmps)
        return out;
    const double wet=std::clamp(
        (witness.FilmMm-settings.MinimumWetFilmMm)/
        (settings.MaximumWetFilmMm-settings.MinimumWetFilmMm),
        0.0,1.0);
    const double velocity=std::clamp(
        (speed-settings.MinimumSpraySpeedCmps)/
        (settings.FullSpraySpeedCmps-settings.MinimumSpraySpeedCmps),
        0.0,1.0);
    out.Valid=true;
    out.Actor=car.Actor;
    out.Xcm=car.WorldXcm;
    out.Ycm=car.WorldYcm;
    out.Zcm=car.WorldZcm;
    out.HeadingRadians=car.HeadingRadians;
    out.SourceFilmMm=witness.FilmMm;
    out.ActualSpeedCmps=speed;
    out.Intensity=settings.MaximumParticleIntensity*
        wet*velocity;
    out.RecommendedRangeCm=
        settings.MaximumSprayRangeCm*std::sqrt(out.Intensity);
    out.ProposedParticleBudget=static_cast<unsigned>(
        settings.MaximumParticlesPerVehicle*out.Intensity);
    return std::isfinite(out.Intensity) &&
        std::isfinite(out.RecommendedRangeCm) ? out:
        WetSprayEmitter{};
}
inline WetSprayExposure EvaluateWetSprayExposure(
    const WetSprayEmitter& emitter,
    const PhysicalReplayCarSample& following,
    WetSpraySettings settings={}) noexcept
{
    WetSprayExposure out;
    if(!emitter.Valid ||
       !FinitePhysicalReplayCar(following) ||
       !ValidWetSpraySettings(settings) ||
       emitter.Actor==following.Actor)return out;
    const double dx=following.WorldXcm-emitter.Xcm;
    const double dy=following.WorldYcm-emitter.Ycm;
    const double forwardX=std::cos(emitter.HeadingRadians);
    const double forwardY=std::sin(emitter.HeadingRadians);
    const double along=-(dx*forwardX+dy*forwardY);
    const double sideways=dx*(-forwardY)+dy*forwardX;
    const double gap=std::hypot(dx,dy);
    if(!std::isfinite(along) ||
       !std::isfinite(sideways) ||
       !std::isfinite(gap))return out;
    out.Valid=true;
    out.EmitterActor=emitter.Actor;
    out.FollowingActor=following.Actor;
    out.AlongSprayCm=along;
    out.LateralSprayCm=sideways;
    out.EuclideanGapCm=gap;
    if(emitter.Intensity>=settings.MinimumEffectiveIntensity &&
       along>0.0 && along<emitter.RecommendedRangeCm &&
       std::abs(sideways)<settings.MaximumLateralSprayCm)
    {
        const double cone=std::tan(
            settings.SprayConeHalfAngleDegrees*
            WetSprayRadiansPerDegree)*along+125.0;
        if(std::abs(sideways)<=cone){
            const double fade=1.0-
                along/emitter.RecommendedRangeCm;
            const double lateral=1.0-
                std::abs(sideways)/std::max(1.0,cone);
            out.ObservedIntensity=emitter.Intensity*
                fade*lateral;
            out.OcclusionAdvisoryFraction=std::clamp(
                out.ObservedIntensity,0.0,1.0);
        }
    }
    if(!std::isfinite(out.ObservedIntensity))return {};
    return out;
}
inline WetSprayFramePlan BuildObservedWetSpray(
    const WorldScaleVariantRecipe& circuit,
    const PhysicalReplayFrame& fleet,
    const ObservedWaterResult& water,
    const std::array<WetSprayVehicleObservation,
        WetSprayFleetSize>& observations,
    WetSpraySettings settings={}) noexcept
{
    WetSprayFramePlan out;
    if(!ValidateWetSprayProvenance(
        circuit,fleet,water,observations,settings))
        return out;
    out.Circuit=circuit.Id;
    out.Session=fleet.Session;
    out.Frame=fleet.SourceFrame;
    out.ClockSeconds=fleet.ActiveClockSeconds;
    for(unsigned i=0u;i<WetSprayFleetSize;++i){
        auto emitter=BuildWetSprayEmitter(
            circuit,fleet.Cars[i],water,observations[i],settings);
        if(!emitter.Valid)return {};
        out.Emitters[i]=emitter;
        ++out.VerifiedEmitterCount;
        if(emitter.Intensity>=settings.MinimumEffectiveIntensity)
            ++out.ActiveEmitterCount;
        out.TotalIntensity+=emitter.Intensity;
        // Never exceed a deterministic global particle budget,
        // even if all six physical cars are on wet tarmac.
        const unsigned remaining=settings.MaximumTotalParticles-
            out.ProposedParticleCount;
        const unsigned accepted=std::min(
            remaining,emitter.ProposedParticleBudget);
        out.Emitters[i].ProposedParticleBudget=accepted;
        out.ProposedParticleCount+=accepted;
    }
    unsigned pair=0u;
    for(unsigned i=0u;i<WetSprayFleetSize;++i)
        for(unsigned j=0u;j<WetSprayFleetSize;++j){
            if(i==j)continue;
            const auto exposure=EvaluateWetSprayExposure(
                out.Emitters[i],fleet.Cars[j],settings);
            if(!exposure.Valid)return {};
            out.Exposures[pair++]=exposure;
            out.PeakExposure=std::max(
                out.PeakExposure,exposure.ObservedIntensity);
            if(exposure.ObservedIntensity>=
               settings.MinimumEffectiveIntensity)
                ++out.ActiveExposureCount;
        }
    out.Valid=pair==WetSprayPairCount &&
        out.VerifiedEmitterCount==WetSprayFleetSize &&
        out.ProposedParticleCount<=settings.MaximumTotalParticles &&
        std::isfinite(out.PeakExposure) &&
        std::isfinite(out.TotalIntensity);
    return out;
}
} // namespace latebrake
