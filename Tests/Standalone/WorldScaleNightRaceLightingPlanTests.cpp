#include "Track/WorldScaleNightRaceLightingPlan.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Require(bool value,const char* what) {
    if(!value) {
        std::cerr<<"NightRaceLighting FAIL: "<<what<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1e-6) {
    return std::abs(a-b)<=tolerance;
}
latebrake::NightRaceAtmosphere SourceNight() {
    latebrake::NightRaceAtmosphere value;
    value.SourceSession=911u;
    value.SourceFrame=1001u;
    value.SourceClockSeconds=81.0;
    value.SunElevationDegrees=-16.0;
    value.FogDensityPerKm=0.015;
    value.SourceConfidence=1.0;
    value.NativeTimeOfDayObserved=true;
    value.ActualFogObserved=true;
    value.OriginalRoadCollisionUnchanged=true;
    return value;
}
}
int main() {
    using namespace latebrake;
    auto settings=NightRaceLightingSettings{};
    Require(ValidNightRaceLightingSettings(settings),
            "default visual-only floodlight physics bounded");
    settings.NeverSpawnUnrealLightActors=false;
    Require(!ValidNightRaceLightingSettings(settings),
            "source-only preview never spawns native lights");
    settings={};
    settings.MaximumTotalPowerWatts=
        std::numeric_limits<double>::quiet_NaN();
    Require(!ValidNightRaceLightingSettings(settings),
            "nonfinite power budget disallowed");
    settings={};
    Require(!ValidNightRaceAtmosphere(
                {},settings),
            "unwitnessed time-of-day cannot create fictional night");
    auto atmosphere=SourceNight();
    Require(ValidNightRaceAtmosphere(
                atmosphere,settings),
            "actually measured night and fog source trusted");
    atmosphere.NativeTimeOfDayObserved=false;
    Require(!ValidNightRaceAtmosphere(
                atmosphere,settings),
            "missing engine sun pose fails closed");
    atmosphere=SourceNight();
    unsigned checked=0u;
    for(unsigned index=0u;index<WorldScaleVariantCount;++index) {
        const auto circuit=WorldScaleOriginalVariantRecipes[index];
        WorldScaleBankedRoadProfile road;
        Require(ChooseWorldScaleBankedRoadProfile(
                    circuit.Id,road),
                "original six future roads have bank profiles");
        const auto scene=BuildTracksideScenePlan(
            circuit,road);
        Require(scene.Valid &&
                scene.QualifiedObjects==TracksideSceneObjectCount,
                "authored 128 original off-road props per circuit");
        const auto plan=BuildNightRaceLightingPlan(
            circuit,scene,road,atmosphere);
        Require(plan.Valid &&
                plan.Circuit==circuit.Id &&
                plan.SourceSession==911u &&
                plan.SourceFrame==1001u &&
                plan.QualifiedFixtures>0u &&
                plan.PoweredFixtures==plan.QualifiedFixtures &&
                plan.QualifiedFixtures<=settings.MaximumPoweredFixtures &&
                plan.TotalEstimatedPowerWatts>0.0 &&
                plan.TotalEstimatedPowerWatts<=
                    settings.MaximumTotalPowerWatts &&
                plan.QualifiedSurveyStations==NightRaceSurveyStations &&
                plan.StationsAboveMinimumLux+
                  plan.BlackoutSurveyStations==NightRaceSurveyStations &&
                plan.MeanIlluminanceLux>=0.0 &&
                plan.MaximumIlluminanceLux>=plan.MinimumIlluminanceLux &&
                plan.UnrealRenderingUnchanged &&
                plan.OriginalPhysicalRoadUnchanged &&
                plan.OfficialRaceStateUnchanged,
                "authentic-source night lighting stays within energy and survey constraints");
        for(unsigned fixture=0u;fixture<plan.QualifiedFixtures;++fixture) {
            const auto& lamp=plan.Fixtures[fixture];
            Require(lamp.Valid &&
                    lamp.SceneObjectId!=0u &&
                    lamp.NominalLumens>0.0 &&
                    lamp.ConsumedPowerWatts>0.0 &&
                    lamp.MaximumInfluenceCm>=4000.0 &&
                    lamp.SourceOnly &&
                    !lamp.UnrealLightActorCreated,
                    "powered photometry is not a real engine actor");
            for(unsigned earlier=0u;earlier<fixture;++earlier)
                Require(lamp.SceneObjectId!=
                        plan.Fixtures[earlier].SceneObjectId,
                        "no lamp may duplicate authored trackside object");
        }
        for(unsigned station=0u;station<NightRaceSurveyStations;++station) {
            const auto& sample=plan.Probes[station];
            Require(sample.Valid &&
                    sample.EstimatedIlluminanceLux>=0.0 &&
                    sample.EstimatedIlluminanceLux<=
                        settings.MaximumIlluminanceLux &&
                    sample.EstimatedFogTransmittance>=0.0 &&
                    sample.EstimatedFogTransmittance<=1.0 &&
                    !sample.ActualNativeShadowTested &&
                    sample.NoDriverVisibilityAuthority,
                    "all 256 road probes are only visual light estimates");
            Require(IsOnWorldScaleVariantRoad(
                circuit,{sample.RoadXcm,sample.RoadYcm}),
                "every illumination estimate uses original physical road XYZ");
            if(station>0u)
                Require(sample.StationCm>
                        plan.Probes[station-1u].StationCm,
                        "source survey station ordering is monotonic");
        }
        const auto repeat=BuildNightRaceLightingPlan(
            circuit,scene,road,atmosphere);
        Require(repeat.Valid &&
                repeat.QualifiedFixtures==plan.QualifiedFixtures &&
                Near(repeat.TotalEstimatedPowerWatts,
                     plan.TotalEstimatedPowerWatts) &&
                Near(repeat.MeanIlluminanceLux,
                     plan.MeanIlluminanceLux),
                "same original source road always has deterministic planned light");

        auto fog=atmosphere;
        fog.FogDensityPerKm=0.20;
        const auto reduced=BuildNightRaceLightingPlan(
            circuit,scene,road,fog);
        Require(reduced.Valid &&
                reduced.MeanIlluminanceLux<=
                    plan.MeanIlluminanceLux+1e-6,
                "more observed fog never increases estimated road illumination");
        auto invalid=atmosphere;
        invalid.FogDensityPerKm=
            std::numeric_limits<double>::quiet_NaN();
        Require(!BuildNightRaceLightingPlan(
            circuit,scene,road,invalid).Valid,
            "nonfinite weather cannot be treated as clear");
        invalid=atmosphere;
        invalid.SunElevationDegrees=50.0;
        Require(!BuildNightRaceLightingPlan(
            circuit,scene,road,invalid).Valid,
            "verified broad daylight is not a night lighting source");
        auto corruptScene=scene;
        corruptScene.Objects[0].UnrealActorSpawned=true;
        Require(!BuildNightRaceLightingPlan(
            circuit,corruptScene,road,atmosphere).Valid,
            "visual proposal cannot authorize actual actor creation");
        auto limited=settings;
        limited.MaximumPoweredFixtures=8u;
        limited.MaximumTotalPowerWatts=30000.0;
        const auto minimal=BuildNightRaceLightingPlan(
            circuit,scene,road,atmosphere,limited);
        Require(minimal.Valid &&
                minimal.QualifiedFixtures<=8u &&
                minimal.TotalEstimatedPowerWatts<=30000.0,
            "tight original electricity cap sheds visual-only fixtures");
        ++checked;
    }
    Require(checked==WorldScaleVariantCount,
            "all six independent original track layouts audited");
    std::cout<<"WorldScaleNightRaceLighting: six circuits qualified\n";
    return EXIT_SUCCESS;
}
