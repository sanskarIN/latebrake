#include "Track/WorldScaleTracksideStreamingPlan.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition,const char* message)
{
    if(!condition)
    {
        std::cerr<<"WorldScaleTracksideStreaming FAIL: "
                 <<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1.0e-6)
{
    return std::abs(a-b)<=tolerance;
}
latebrake::TracksideStreamingObservation Observation(
    latebrake::WorldScaleVariantId circuit,
    double station,double speed=2000.0,
    std::uint64_t frame=1u,double clock=1.0)
{
    latebrake::TracksideStreamingObservation input;
    input.Circuit=circuit;
    input.Session=31u;
    input.SessionRevision=4u;
    input.NativeVehicleToken=82u;
    input.SourceFrame=frame;
    input.EngineClockSeconds=clock;
    input.ObservedStationCm=station;
    input.MeasuredSpeedCmps=speed;
    input.ActualEngineRouteProjectionObserved=true;
    input.OriginalWorldStaticRoadVerified=true;
    input.OriginalPhysicalVehicleObserved=true;
    return input;
}
}

int main()
{
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=TracksideStreamingSettings{};
    Check(ValidTracksideStreamingSettings(settings),
          "default original 16-sector streaming policy valid");
    settings.MaximumResidentObjects=8u;
    Check(!ValidTracksideStreamingSettings(settings),
          "cannot admit fewer resident objects than source inventory");
    settings={};
    settings.MaximumPrefetchSectorsBudget=2u;
    Check(!ValidTracksideStreamingSettings(settings),
          "cannot prefetch more sectors than fixed memory budget");
    settings={};
    settings.NeverCallUnrealStreamingAPI=false;
    Check(!ValidTracksideStreamingSettings(settings),
          "source-only plan never claims actual engine streaming API");
    settings={};
    settings.LookAheadSeconds=nan;
    Check(!ValidTracksideStreamingSettings(settings),
          "nonfinite predictive horizon rejected");
    settings={};
    settings.KeepAllCollisionAssetsResident=false;
    Check(!ValidTracksideStreamingSettings(settings),
          "original physical road assets never streamed away");
    settings={};
    unsigned independentlyChecked=0u;
    for(unsigned route=0u;route<WorldScaleVariantCount;++route)
    {
        const auto circuit=WorldScaleOriginalVariantRecipes[route];
        WorldScaleBankedRoadProfile road;
        Check(ChooseWorldScaleBankedRoadProfile(
            circuit.Id,road),
            "six original source track profiles required");
        const auto scene=BuildTracksideScenePlan(
            circuit,road);
        Check(scene.Valid &&
              scene.QualifiedObjects==TracksideSceneObjectCount,
              "each route has exactly 128 original source-only props");
        const auto catalog=BuildTracksideStreamingCatalog(
            circuit,scene);
        Check(catalog.Valid &&
              catalog.Circuit==circuit.Id &&
              catalog.QualifiedObjects==TracksideSceneObjectCount &&
              catalog.QualifiedSectors==TracksideStreamingSectorCount &&
              Near(catalog.SectorLengthCm*
                   TracksideStreamingSectorCount,circuit.LapLengthCm),
              "authored source props partition into 16 equal route sectors");
        for(unsigned sector=0u;
            sector<TracksideStreamingSectorCount;++sector)
        {
            const auto& chunk=catalog.Sectors[sector];
            Check(chunk.Valid &&
                  chunk.Index==sector &&
                  chunk.SourceObjects==TracksideStreamingPropsPerSector &&
                  Near(chunk.StartStationCm,
                       sector*catalog.SectorLengthCm) &&
                  Near(chunk.EndStationCm,
                       (sector+1u)*catalog.SectorLengthCm) &&
                  chunk.MinimumStructureHeightCm>0.0 &&
                  chunk.MaximumStructureHeightCm>=
                      chunk.MinimumStructureHeightCm &&
                  chunk.SourceOnly,
                  "each sector contains eight unique physical-road-excluded props");
            for(unsigned slot=0u;slot<chunk.SourceObjects;++slot)
            {
                Check(chunk.ObjectIds[slot]!=0u &&
                      chunk.SourceObjectIndex[slot]<scene.Objects.size() &&
                      chunk.ObjectIds[slot]==scene.Objects[
                          chunk.SourceObjectIndex[slot]].DeterministicId,
                      "sector object inventory exactly matches input scene");
                for(unsigned other=0u;other<slot;++other)
                    Check(chunk.ObjectIds[slot]!=chunk.ObjectIds[other],
                          "no duplicate ID inside a streaming section");
            }
        }
        Check(TracksideSectorOfStation(catalog,0.0)==0u &&
              TracksideSectorOfStation(
                  catalog,circuit.LapLengthCm)==0u &&
              TracksideSectorOfStation(
                  catalog,-1.0)==TracksideStreamingSectorCount-1u &&
              TracksideSectorOfStation(
                  catalog,nan)==TracksideStreamingSectorCount,
              "station lookup closes signed lap seam and rejects NaN");

        const auto source=Observation(circuit.Id,100.0,0.0);
        Check(ValidTracksideStreamingObservation(
            catalog,source,settings),
            "real engine-reported speed and lane projection source accepted");
        const auto stationary=PlanTracksideStreaming(
            catalog,source);
        Check(stationary.Valid &&
              stationary.Fault==TracksideStreamingFault::None &&
              stationary.Circuit==circuit.Id &&
              stationary.CurrentSector==0u &&
              stationary.LookAheadSectors==2u &&
              stationary.ResidentSectorCount==3u &&
              stationary.PrefetchSectorCount==2u &&
              stationary.ColdSectorCount==11u &&
              stationary.ResidentObjectCount==24u &&
              stationary.PrefetchObjectCount==16u &&
              stationary.NoAssetsLoadedOrRemoved &&
              stationary.CollisionRemainUnchanged &&
              stationary.NoOfficialRaceImpact,
              "stationary scene retains 3 resident and 2 warm sectors");
        Check(stationary.Sectors[0].Intent==
                  TracksideStreamingIntent::Resident &&
              stationary.Sectors[1].Intent==
                  TracksideStreamingIntent::Resident &&
              stationary.Sectors[15].Intent==
                  TracksideStreamingIntent::Resident &&
              stationary.Sectors[2].Intent==
                  TracksideStreamingIntent::Prefetch &&
              stationary.Sectors[3].Intent==
                  TracksideStreamingIntent::Prefetch &&
              stationary.Sectors[8].Intent==
                  TracksideStreamingIntent::Cold,
              "streaming safely crosses sector-15 -> sector-0 seam");
        for(const auto& sec:stationary.Sectors)
            Check(!sec.MayLoadRealUnrealAssets &&
                  !sec.MayUnloadSourceCollision &&
                  !sec.MayChangeOfficialGameState &&
                  sec.SourceObjectCount==TracksideStreamingPropsPerSector,
                  "no native engine load/unload or race authority emitted");

        auto race=source;
        race.MeasuredSpeedCmps=12000.0;
        const auto fast=PlanTracksideStreaming(
            catalog,race);
        Check(fast.Valid &&
              fast.PrefetchSectorCount>=stationary.PrefetchSectorCount &&
              fast.LookAheadSectors>=stationary.LookAheadSectors &&
              fast.PrefetchSectorCount<=6u &&
              fast.PrefetchObjectCount<=48u &&
              fast.ResidentObjectCount<=24u,
              "high speed requests bounded source prefetch horizon");
        const auto again=PlanTracksideStreaming(
            catalog,race);
        Check(again.Valid &&
              again.CurrentSector==fast.CurrentSector &&
              again.PrefetchSectorCount==fast.PrefetchSectorCount,
              "same source observation gives deterministic streaming window");

        auto next=source;
        next.SourceFrame=2u;
        next.EngineClockSeconds=1.05;
        next.ObservedStationCm=150.0;
        next.MeasuredSpeedCmps=2000.0;
        const auto continuous=PlanTracksideStreaming(
            catalog,next);
        Check(continuous.Valid &&
              TracksideStreamingContinuity(stationary,continuous),
              "observed small chassis movement preserves same session window");
        next.ObservedStationCm=catalog.SectorLengthCm+250.0;
        const auto teleport=PlanTracksideStreaming(
            catalog,next);
        Check(teleport.Valid &&
              !TracksideStreamingContinuity(stationary,teleport),
              "single sector hop without physical travel cannot be trusted");
        next.ObservedStationCm=150.0;
        next.SessionRevision=5u;
        const auto restarted=PlanTracksideStreaming(catalog,next);
        Check(restarted.Valid &&
              !TracksideStreamingContinuity(stationary,restarted),
              "race restart invalidates temporal prefetch witness");

        auto end=Observation(circuit.Id,
            circuit.LapLengthCm-75.0,3000.0,5u,2.0);
        const auto nearFinish=PlanTracksideStreaming(
            catalog,end);
        end.SourceFrame=6u;
        end.EngineClockSeconds=2.05;
        end.ObservedStationCm=50.0;
        const auto justAfterStart=PlanTracksideStreaming(
            catalog,end);
        Check(nearFinish.Valid && justAfterStart.Valid &&
              nearFinish.CurrentSector==
                  TracksideStreamingSectorCount-1u &&
              justAfterStart.CurrentSector==0u &&
              TracksideStreamingContinuity(
                  nearFinish,justAfterStart),
              "real measured seam crossing is admissible in either lap");
        auto negative=Observation(
            circuit.Id,-75.0,3000.0);
        const auto negativeStart=PlanTracksideStreaming(
            catalog,negative);
        Check(negativeStart.Valid &&
              negativeStart.CurrentSector==
                  TracksideStreamingSectorCount-1u,
              "negative source station wraps into last visual sector");

        auto missing=source;
        missing.ActualEngineRouteProjectionObserved=false;
        Check(!PlanTracksideStreaming(
            catalog,missing).Valid,
            "invented route projection cannot qualify actual streaming");
        missing=source;
        missing.NativeVehicleToken=0u;
        Check(!PlanTracksideStreaming(
            catalog,missing).Valid,
            "missing original Chaos actor token rejected");
        missing=source;
        missing.MeasuredSpeedCmps=nan;
        Check(!PlanTracksideStreaming(
            catalog,missing).Valid,
            "nonfinite measured speed rejected");
        missing=source;
        missing.Circuit=WorldScaleOriginalVariantRecipes[
            (route+1u)%WorldScaleVariantCount].Id;
        const auto foreign=PlanTracksideStreaming(
            catalog,missing);
        Check(!foreign.Valid &&
              foreign.Fault==TracksideStreamingFault::SourceEpochMismatch,
              "foreign route cannot reuse a different track sector catalog");
        auto tampered=scene;
        tampered.Objects[5].DeterministicId=
            tampered.Objects[4].DeterministicId;
        Check(!BuildTracksideStreamingCatalog(
            circuit,tampered).Valid,
            "duplicate scene source ID invalidates catalog");
        tampered=scene;
        tampered.Objects[4].StationCm=
            circuit.LapLengthCm+1.0;
        Check(!BuildTracksideStreamingCatalog(
            circuit,tampered).Valid,
            "out-of-lap geometry must not be silently wrapped");
        ++independentlyChecked;
    }
    Check(independentlyChecked==WorldScaleVariantCount,
          "all six original track layouts audited independently");
    std::cout<<"WorldScaleTracksideStreaming: six-circuit 16-sector QA passed\n";
    return EXIT_SUCCESS;
}
