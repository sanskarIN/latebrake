#pragma once
#include "Track/WorldScaleTracksideScenePlan.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Deterministic original six-circuit trackside NIGHT illumination proposal.
// This is a finite source-only photometric estimate from authored positions;
// it does not create native Unreal lights, shadows, ray tracing, or true
// nighttime visibility and never changes driving, collisions or lap records.
inline constexpr unsigned NightRaceMaximumFixtures=
    TracksideSceneObjectCount;
inline constexpr unsigned NightRaceSurveyStations=256u;
inline constexpr double NightRacePi=
    3.1415926535897932384626433832795;
enum class NightRaceFixtureKind : std::uint8_t {
    Floodlight, Grandstand, MarshalPost, Rescue, Wayfinding, CameraTower
};
struct NightRaceLightingSettings final {
    double MaximumFixtureHeightCm=1800.0;
    double MinimumFixtureHeightCm=140.0;
    double FloodlightBeamHalfDegrees=62.0;
    double MinimumBeamHalfDegrees=35.0;
    double MaximumBeamHalfDegrees=85.0;
    double MaximumFixtureRangeCm=8000.0;
    double MaximumFogDensityPerKm=0.20;
    double MinimumVisibleIlluminanceLux=1.0;
    double TargetIlluminanceLux=25.0;
    double MaximumIlluminanceLux=800.0;
    double MinimumFixturePowerWatts=70.0;
    double MaximumFixturePowerWatts=2400.0;
    double MaximumTotalPowerWatts=90000.0;
    double MaximumSunElevationDegrees=12.0;
    double MinimumSunElevationDegrees=-35.0;
    unsigned MaximumPoweredFixtures=64u;
    unsigned SurveyStationCount=NightRaceSurveyStations;
    bool RequireNonCollidingScene=true;
    bool NeverSpawnUnrealLightActors=true;
    bool NeverAlterPhysicsOrWeather=true;
    bool NoOfficialRaceProgressAuthority=true;
};
struct NightRaceAtmosphere final {
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    double SunElevationDegrees=-16.0;
    double FogDensityPerKm=0.015;
    double SourceConfidence=1.0;
    bool NativeTimeOfDayObserved=false;
    bool ActualFogObserved=false;
    bool OriginalRoadCollisionUnchanged=true;
};
struct NightRaceFixture final {
    bool Valid=false;
    NightRaceFixtureKind Kind=NightRaceFixtureKind::Floodlight;
    std::uint64_t SceneObjectId=0u;
    double Xcm=0.0;
    double Ycm=0.0;
    double Zcm=0.0;
    double YawRadians=0.0;
    double BeamHalfAngleRadians=0.0;
    double NominalLumens=0.0;
    double ConsumedPowerWatts=0.0;
    double MaximumInfluenceCm=0.0;
    unsigned MaterialIndex=0u;
    bool SourceOnly=true;
    bool UnrealLightActorCreated=false;
};
struct NightRaceLightProbe final {
    bool Valid=false;
    double StationCm=0.0;
    double RoadXcm=0.0;
    double RoadYcm=0.0;
    double RoadZcm=0.0;
    double EstimatedIlluminanceLux=0.0;
    double EstimatedFogTransmittance=1.0;
    double HighestFixtureContributionLux=0.0;
    unsigned ContributingFixtures=0u;
    bool ActualNativeShadowTested=false;
    bool NoDriverVisibilityAuthority=true;
};
struct NightRaceLightingPlan final {
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    double TotalEstimatedPowerWatts=0.0;
    double MeanIlluminanceLux=0.0;
    double MaximumIlluminanceLux=0.0;
    double MinimumIlluminanceLux=0.0;
    double MaximumSurveyFogAttenuation=0.0;
    unsigned QualifiedFixtures=0u;
    unsigned PoweredFixtures=0u;
    unsigned QualifiedSurveyStations=0u;
    unsigned StationsAboveMinimumLux=0u;
    unsigned BlackoutSurveyStations=0u;
    std::array<NightRaceFixture,NightRaceMaximumFixtures> Fixtures{};
    std::array<NightRaceLightProbe,NightRaceSurveyStations> Probes{};
    bool UnrealRenderingUnchanged=true;
    bool OriginalPhysicalRoadUnchanged=true;
    bool OfficialRaceStateUnchanged=true;
};
inline bool ValidNightRaceLightingSettings(
    NightRaceLightingSettings s) noexcept
{
    return std::isfinite(s.MaximumFixtureHeightCm) &&
        s.MaximumFixtureHeightCm>=1000.0 &&
        s.MaximumFixtureHeightCm<=2500.0 &&
        std::isfinite(s.MinimumFixtureHeightCm) &&
        s.MinimumFixtureHeightCm>=90.0 &&
        s.MinimumFixtureHeightCm<=300.0 &&
        std::isfinite(s.FloodlightBeamHalfDegrees) &&
        s.FloodlightBeamHalfDegrees>=45.0 &&
        s.FloodlightBeamHalfDegrees<=80.0 &&
        std::isfinite(s.MinimumBeamHalfDegrees) &&
        s.MinimumBeamHalfDegrees>=20.0 &&
        s.MinimumBeamHalfDegrees<=50.0 &&
        std::isfinite(s.MaximumBeamHalfDegrees) &&
        s.MaximumBeamHalfDegrees>=75.0 &&
        s.MaximumBeamHalfDegrees<=90.0 &&
        s.MaximumBeamHalfDegrees>s.MinimumBeamHalfDegrees &&
        std::isfinite(s.MaximumFixtureRangeCm) &&
        s.MaximumFixtureRangeCm>=4000.0 &&
        s.MaximumFixtureRangeCm<=12000.0 &&
        std::isfinite(s.MaximumFogDensityPerKm) &&
        s.MaximumFogDensityPerKm>=0.05 &&
        s.MaximumFogDensityPerKm<=0.5 &&
        std::isfinite(s.MinimumVisibleIlluminanceLux) &&
        s.MinimumVisibleIlluminanceLux>=0.1 &&
        s.MinimumVisibleIlluminanceLux<=5.0 &&
        std::isfinite(s.TargetIlluminanceLux) &&
        s.TargetIlluminanceLux>=10.0 &&
        s.TargetIlluminanceLux<=80.0 &&
        std::isfinite(s.MaximumIlluminanceLux) &&
        s.MaximumIlluminanceLux>=400.0 &&
        s.MaximumIlluminanceLux<=1200.0 &&
        std::isfinite(s.MinimumFixturePowerWatts) &&
        s.MinimumFixturePowerWatts>=40.0 &&
        s.MinimumFixturePowerWatts<=150.0 &&
        std::isfinite(s.MaximumFixturePowerWatts) &&
        s.MaximumFixturePowerWatts>=1500.0 &&
        s.MaximumFixturePowerWatts<=3000.0 &&
        std::isfinite(s.MaximumTotalPowerWatts) &&
        s.MaximumTotalPowerWatts>=30000.0 &&
        s.MaximumTotalPowerWatts<=120000.0 &&
        std::isfinite(s.MaximumSunElevationDegrees) &&
        s.MaximumSunElevationDegrees>=0.0 &&
        s.MaximumSunElevationDegrees<=20.0 &&
        std::isfinite(s.MinimumSunElevationDegrees) &&
        s.MinimumSunElevationDegrees>=-50.0 &&
        s.MinimumSunElevationDegrees<=-15.0 &&
        s.MinimumSunElevationDegrees<s.MaximumSunElevationDegrees &&
        s.MaximumPoweredFixtures>=8u &&
        s.MaximumPoweredFixtures<=NightRaceMaximumFixtures &&
        s.SurveyStationCount==NightRaceSurveyStations &&
        s.RequireNonCollidingScene &&
        s.NeverSpawnUnrealLightActors &&
        s.NeverAlterPhysicsOrWeather &&
        s.NoOfficialRaceProgressAuthority;
}
inline bool ValidNightRaceAtmosphere(
    NightRaceAtmosphere atmosphere,
    NightRaceLightingSettings settings={}) noexcept
{
    return ValidNightRaceLightingSettings(settings) &&
        atmosphere.SourceSession!=0u &&
        atmosphere.SourceFrame!=0u &&
        atmosphere.NativeTimeOfDayObserved &&
        atmosphere.ActualFogObserved &&
        atmosphere.OriginalRoadCollisionUnchanged &&
        std::isfinite(atmosphere.SourceClockSeconds) &&
        atmosphere.SourceClockSeconds>0.0 &&
        atmosphere.SourceClockSeconds<=86400.0 &&
        std::isfinite(atmosphere.SunElevationDegrees) &&
        atmosphere.SunElevationDegrees>=
            settings.MinimumSunElevationDegrees &&
        atmosphere.SunElevationDegrees<=
            settings.MaximumSunElevationDegrees &&
        std::isfinite(atmosphere.FogDensityPerKm) &&
        atmosphere.FogDensityPerKm>=0.0 &&
        atmosphere.FogDensityPerKm<=
            settings.MaximumFogDensityPerKm &&
        std::isfinite(atmosphere.SourceConfidence) &&
        atmosphere.SourceConfidence>=0.8 &&
        atmosphere.SourceConfidence<=1.0;
}
inline NightRaceFixtureKind NightRaceMapFixtureKind(
    TracksideSceneKind kind) noexcept
{
    switch(kind) {
    case TracksideSceneKind::Floodlight:
        return NightRaceFixtureKind::Floodlight;
    case TracksideSceneKind::Grandstand:
        return NightRaceFixtureKind::Grandstand;
    case TracksideSceneKind::MarshalPost:
        return NightRaceFixtureKind::MarshalPost;
    case TracksideSceneKind::RescueAccess:
        return NightRaceFixtureKind::Rescue;
    case TracksideSceneKind::WayfindingSign:
        return NightRaceFixtureKind::Wayfinding;
    default:
        return NightRaceFixtureKind::CameraTower;
    }
}
inline bool NightRaceIsPoweredFixture(TracksideSceneKind kind) noexcept
{
    return kind!=TracksideSceneKind::WoodlandCanopy;
}
inline double NightRaceFixtureWattage(
    NightRaceFixtureKind kind) noexcept
{
    switch(kind) {
    case NightRaceFixtureKind::Floodlight:return 1800.0;
    case NightRaceFixtureKind::Grandstand:return 1200.0;
    case NightRaceFixtureKind::MarshalPost:return 250.0;
    case NightRaceFixtureKind::Rescue:return 420.0;
    case NightRaceFixtureKind::Wayfinding:return 120.0;
    case NightRaceFixtureKind::CameraTower:return 270.0;
    }
    return 0.0;
}
inline double NightRaceFixtureLumens(
    NightRaceFixtureKind kind) noexcept
{
    const double power=NightRaceFixtureWattage(kind);
    return power*115.0; // proposed luminous efficacy, not real measured IES
}
inline NightRaceFixture BuildNightRaceFixture(
    const TracksideSceneObject& scene,
    NightRaceLightingSettings settings={}) noexcept
{
    NightRaceFixture out;
    if(!ValidNightRaceLightingSettings(settings) ||
       !scene.Valid ||
       !scene.EveryBaseCornerOutsideRoad ||
       scene.UnrealActorSpawned ||
       scene.DrivableCollisionAdded ||
       scene.DeterministicId==0u ||
       !std::isfinite(scene.Xcm) ||
       !std::isfinite(scene.Ycm) ||
       !std::isfinite(scene.Zcm) ||
       !std::isfinite(scene.YawRadians) ||
       !std::isfinite(scene.HeightCm) ||
       scene.HeightCm<settings.MinimumFixtureHeightCm ||
       scene.HeightCm>settings.MaximumFixtureHeightCm ||
       !NightRaceIsPoweredFixture(scene.Kind))
        return out;
    out.Kind=NightRaceMapFixtureKind(scene.Kind);
    out.SceneObjectId=scene.DeterministicId;
    out.Xcm=scene.Xcm;
    out.Ycm=scene.Ycm;
    out.Zcm=scene.Zcm+scene.HeightCm;
    out.YawRadians=scene.YawRadians;
    out.BeamHalfAngleRadians=std::clamp(
        settings.FloodlightBeamHalfDegrees*NightRacePi/180.0,
        settings.MinimumBeamHalfDegrees*NightRacePi/180.0,
        settings.MaximumBeamHalfDegrees*NightRacePi/180.0);
    out.ConsumedPowerWatts=NightRaceFixtureWattage(out.Kind);
    out.NominalLumens=NightRaceFixtureLumens(out.Kind);
    out.MaximumInfluenceCm=settings.MaximumFixtureRangeCm;
    out.MaterialIndex=scene.MaterialIndex;
    out.Valid=std::isfinite(out.Zcm) &&
        std::isfinite(out.NominalLumens) &&
        out.NominalLumens>0.0 &&
        out.ConsumedPowerWatts>=settings.MinimumFixturePowerWatts &&
        out.ConsumedPowerWatts<=settings.MaximumFixturePowerWatts;
    return out;
}
// One point fixture estimates light on road with inverse-square distance,
// explicit aim, incidence and fog transmission. Neither shadow occlusion
// nor actual GPU output is known until an Unreal scene is run.
inline double EstimateNightRaceFixtureLux(
    NightRaceFixture fixture,
    CircuitPoint road,
    double roadHeightCm,
    double fogDensityPerKm,
    NightRaceLightingSettings settings={}) noexcept
{
    if(!fixture.Valid ||
       !ValidNightRaceLightingSettings(settings) ||
       !std::isfinite(road.X) || !std::isfinite(road.Y) ||
       !std::isfinite(roadHeightCm) ||
       !std::isfinite(fogDensityPerKm) ||
       fogDensityPerKm<0.0 ||
       fogDensityPerKm>settings.MaximumFogDensityPerKm)
        return 0.0;
    const double dx=road.X-fixture.Xcm;
    const double dy=road.Y-fixture.Ycm;
    const double dz=roadHeightCm-fixture.Zcm;
    const double range=std::hypot(std::hypot(dx,dy),dz);
    if(!std::isfinite(range) ||
       range<100.0 ||
       range>fixture.MaximumInfluenceCm)
        return 0.0;
    // Use a down-tilted 35 degree beam with source road orientation.
    const double yaw=fixture.YawRadians;
    const double beamX=0.574*std::cos(yaw);
    const double beamY=0.574*std::sin(yaw);
    const double beamZ=-0.819;
    const double cosine=(beamX*dx+beamY*dy+beamZ*dz)/range;
    if(!std::isfinite(cosine) ||
       cosine<std::cos(fixture.BeamHalfAngleRadians))
        return 0.0;
    const double verticalIncidence=std::max(0.0,-dz/range);
    const double attenuation=std::exp(
        -fogDensityPerKm*(range/100000.0));
    const double meters=std::max(1.0,range/100.0);
    const double steradians=2.0*NightRacePi*
        (1.0-std::cos(fixture.BeamHalfAngleRadians));
    if(steradians<=0.0)return 0.0;
    const double luminousIntensity=
        fixture.NominalLumens/steradians;
    const double angularWeight=std::pow(
        std::max(0.0,cosine),2.0);
    const double lux=luminousIntensity/
        (meters*meters)*verticalIncidence*
        angularWeight*attenuation;
    return std::isfinite(lux) ?
        std::clamp(lux,0.0,settings.MaximumIlluminanceLux):0.0;
}
inline NightRaceLightingPlan BuildNightRaceLightingPlan(
    const WorldScaleVariantRecipe& circuit,
    const TracksideScenePlan& scene,
    WorldScaleBankedRoadProfile road,
    NightRaceAtmosphere atmosphere,
    NightRaceLightingSettings settings={}) noexcept
{
    NightRaceLightingPlan out;
    if(!ValidWorldScaleVariantRecipe(circuit) ||
       !ValidWorldScaleBankedRoadProfile(circuit,road) ||
       !ValidNightRaceLightingSettings(settings) ||
       !ValidNightRaceAtmosphere(atmosphere,settings) ||
       !scene.Valid ||
       scene.Circuit!=circuit.Id ||
       scene.QualifiedObjects!=TracksideSceneObjectCount ||
       !scene.SourceOnly ||
       !scene.OriginalCollisionUnchanged)
        return out;
    out.Circuit=circuit.Id;
    out.SourceSession=atmosphere.SourceSession;
    out.SourceFrame=atmosphere.SourceFrame;
    out.SourceClockSeconds=atmosphere.SourceClockSeconds;
    for(const auto& object:scene.Objects) {
        if(!object.Valid || object.UnrealActorSpawned ||
           object.DrivableCollisionAdded)
            return {};
        if(!NightRaceIsPoweredFixture(object.Kind))
            continue;
        const auto light=BuildNightRaceFixture(object,settings);
        if(!light.Valid ||
           out.QualifiedFixtures>=NightRaceMaximumFixtures)
            return {};
        if(out.PoweredFixtures<settings.MaximumPoweredFixtures &&
           out.TotalEstimatedPowerWatts+
             light.ConsumedPowerWatts<=
             settings.MaximumTotalPowerWatts)
        {
            out.Fixtures[out.QualifiedFixtures++]=light;
            out.TotalEstimatedPowerWatts+=light.ConsumedPowerWatts;
            ++out.PoweredFixtures;
        }
    }
    if(out.QualifiedFixtures==0u)return {};
    out.MinimumIlluminanceLux=
        std::numeric_limits<double>::infinity();
    for(unsigned i=0u;i<NightRaceSurveyStations;++i) {
        const double station=circuit.LapLengthCm*i/
            static_cast<double>(NightRaceSurveyStations);
        const auto roadSample=SampleWorldScaleBankedRoad(
            circuit,road,station,0.0);
        if(!roadSample.Valid)return {};
        NightRaceLightProbe probe;
        probe.StationCm=station;
        probe.RoadXcm=roadSample.WorldXcm;
        probe.RoadYcm=roadSample.WorldYcm;
        probe.RoadZcm=roadSample.WorldZcm;
        probe.EstimatedFogTransmittance=1.0;
        for(unsigned f=0u;f<out.QualifiedFixtures;++f) {
            const double lux=EstimateNightRaceFixtureLux(
                out.Fixtures[f],
                {probe.RoadXcm,probe.RoadYcm},
                probe.RoadZcm,atmosphere.FogDensityPerKm,settings);
            if(lux>0.0){
                probe.EstimatedIlluminanceLux+=lux;
                probe.HighestFixtureContributionLux=std::max(
                    probe.HighestFixtureContributionLux,lux);
                ++probe.ContributingFixtures;
            }
        }
        probe.EstimatedIlluminanceLux=std::min(
            settings.MaximumIlluminanceLux,
            probe.EstimatedIlluminanceLux);
        if(!std::isfinite(probe.EstimatedIlluminanceLux))
            return {};
        probe.Valid=true;
        out.Probes[i]=probe;
        out.MeanIlluminanceLux+=probe.EstimatedIlluminanceLux;
        out.MaximumIlluminanceLux=std::max(
            out.MaximumIlluminanceLux,
            probe.EstimatedIlluminanceLux);
        out.MinimumIlluminanceLux=std::min(
            out.MinimumIlluminanceLux,
            probe.EstimatedIlluminanceLux);
        if(probe.EstimatedIlluminanceLux>=
           settings.MinimumVisibleIlluminanceLux)
            ++out.StationsAboveMinimumLux;
        else
            ++out.BlackoutSurveyStations;
        ++out.QualifiedSurveyStations;
    }
    out.MeanIlluminanceLux/=static_cast<double>(
        NightRaceSurveyStations);
    out.Valid=out.QualifiedSurveyStations==
        NightRaceSurveyStations &&
        out.StationsAboveMinimumLux+
        out.BlackoutSurveyStations==NightRaceSurveyStations &&
        out.QualifiedFixtures<=settings.MaximumPoweredFixtures &&
        out.TotalEstimatedPowerWatts<=
            settings.MaximumTotalPowerWatts &&
        std::isfinite(out.MeanIlluminanceLux);
    return out;
}
} // namespace latebrake
