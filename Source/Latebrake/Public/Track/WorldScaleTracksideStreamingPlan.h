#pragma once

#include "Track/WorldScaleTracksideScenePlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Deterministic streaming *proposals* for the six original source roads.
// The active Unreal world is never unloaded, altered, given collision,
// granted memory availability or told that an asset was actually loaded.
inline constexpr unsigned TracksideStreamingSectorCount=16u;
inline constexpr unsigned TracksideStreamingPropsPerSector=
    TracksideSceneObjectCount/TracksideStreamingSectorCount;
static_assert(TracksideSceneObjectCount%
    TracksideStreamingSectorCount==0u);
static_assert(TracksideStreamingPropsPerSector==8u);

enum class TracksideStreamingIntent : std::uint8_t
{
    Cold,
    Prefetch,
    Resident
};

enum class TracksideStreamingFault : std::uint8_t
{
    None,
    BadRoad,
    BadCatalog,
    BadCamera,
    BadPolicy,
    SourceEpochMismatch,
    ImpossibleResourceBudget
};

struct TracksideStreamingSettings final
{
    unsigned ResidentPreviousSectors=1u;
    unsigned ResidentNextSectors=1u;
    unsigned MinimumPrefetchSectors=2u;
    unsigned MaximumPrefetchSectors=6u;
    unsigned MaximumResidentSectors=3u;
    unsigned MaximumPrefetchSectorsBudget=6u;
    unsigned MaximumResidentObjects=24u;
    unsigned MaximumPrefetchedObjects=48u;
    double LookAheadSeconds=2.5;
    double MaximumMeasuredSpeedCmps=12000.0;
    double MaximumCameraClockSeconds=86400.0;
    double MaximumSourceStationCm=1000000.0;
    bool KeepSourceRoadPhysicallyPresent=true;
    bool RequireRealRouteProjection=true;
    bool KeepAllCollisionAssetsResident=true;
    bool NeverCallUnrealStreamingAPI=true;
    bool NeverModifyRaceState=true;
};

struct TracksideStreamingSector final
{
    bool Valid=false;
    unsigned Index=0u;
    double StartStationCm=0.0;
    double EndStationCm=0.0;
    std::array<std::size_t,TracksideStreamingPropsPerSector>
        SourceObjectIndex{};
    std::array<std::uint64_t,TracksideStreamingPropsPerSector>
        ObjectIds{};
    unsigned SourceObjects=0u;
    double MinimumStructureHeightCm=0.0;
    double MaximumStructureHeightCm=0.0;
    bool SourceOnly=true;
};

struct TracksideStreamingCatalog final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::array<TracksideStreamingSector,
        TracksideStreamingSectorCount> Sectors{};
    double LapLengthCm=0.0;
    double SectorLengthCm=0.0;
    unsigned QualifiedObjects=0u;
    unsigned QualifiedSectors=0u;
    bool OriginalWorldStaticRoadUnchanged=true;
};

struct TracksideStreamingObservation final
{
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t SessionRevision=0u;
    std::uint64_t NativeVehicleToken=0u;
    std::uint64_t SourceFrame=0u;
    double EngineClockSeconds=0.0;
    double ObservedStationCm=0.0;
    double MeasuredSpeedCmps=0.0;
    bool ActualEngineRouteProjectionObserved=false;
    bool OriginalWorldStaticRoadVerified=false;
    bool OriginalPhysicalVehicleObserved=false;
};

struct TracksideStreamingSectorAction final
{
    unsigned SectorIndex=0u;
    TracksideStreamingIntent Intent=TracksideStreamingIntent::Cold;
    unsigned SourceObjectCount=0u;
    double StartStationCm=0.0;
    double EndStationCm=0.0;
    bool MayLoadRealUnrealAssets=false;
    bool MayUnloadSourceCollision=false;
    bool MayChangeOfficialGameState=false;
};

struct TracksideStreamingPlan final
{
    bool Valid=false;
    TracksideStreamingFault Fault=TracksideStreamingFault::BadCatalog;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceRevision=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    double WrappedStationCm=0.0;
    double SourceLapLengthCm=0.0;
    double MeasuredSpeedCmps=0.0;
    unsigned CurrentSector=0u;
    unsigned LookAheadSectors=0u;
    unsigned ResidentSectorCount=0u;
    unsigned PrefetchSectorCount=0u;
    unsigned ColdSectorCount=0u;
    unsigned ResidentObjectCount=0u;
    unsigned PrefetchObjectCount=0u;
    std::array<TracksideStreamingSectorAction,
        TracksideStreamingSectorCount> Sectors{};
    bool NoAssetsLoadedOrRemoved=true;
    bool CollisionRemainUnchanged=true;
    bool NoOfficialRaceImpact=true;
};

inline bool ValidTracksideStreamingSettings(
    TracksideStreamingSettings p) noexcept
{
    return p.ResidentPreviousSectors<=2u &&
        p.ResidentNextSectors>=1u &&
        p.ResidentNextSectors<=2u &&
        p.ResidentPreviousSectors+
            p.ResidentNextSectors+1u<=
                p.MaximumResidentSectors &&
        p.MaximumResidentSectors>=3u &&
        p.MaximumResidentSectors<=5u &&
        p.MinimumPrefetchSectors>=1u &&
        p.MinimumPrefetchSectors<=4u &&
        p.MaximumPrefetchSectors>=
            p.MinimumPrefetchSectors &&
        p.MaximumPrefetchSectors<=7u &&
        p.MaximumPrefetchSectorsBudget>=
            p.MaximumPrefetchSectors &&
        p.MaximumPrefetchSectorsBudget<=8u &&
        p.MaximumResidentObjects>=
            p.MaximumResidentSectors*
                TracksideStreamingPropsPerSector &&
        p.MaximumResidentObjects<=
            TracksideSceneObjectCount &&
        p.MaximumPrefetchedObjects>=
            p.MaximumPrefetchSectorsBudget*
                TracksideStreamingPropsPerSector &&
        p.MaximumPrefetchedObjects<=
            TracksideSceneObjectCount &&
        std::isfinite(p.LookAheadSeconds) &&
        p.LookAheadSeconds>=0.5 &&
        p.LookAheadSeconds<=8.0 &&
        std::isfinite(p.MaximumMeasuredSpeedCmps) &&
        p.MaximumMeasuredSpeedCmps>=2000.0 &&
        p.MaximumMeasuredSpeedCmps<=15000.0 &&
        std::isfinite(p.MaximumCameraClockSeconds) &&
        p.MaximumCameraClockSeconds>=60.0 &&
        p.MaximumCameraClockSeconds<=86400.0 &&
        std::isfinite(p.MaximumSourceStationCm) &&
        p.MaximumSourceStationCm>=200000.0 &&
        p.MaximumSourceStationCm<=1000000.0 &&
        p.KeepSourceRoadPhysicallyPresent &&
        p.RequireRealRouteProjection &&
        p.KeepAllCollisionAssetsResident &&
        p.NeverCallUnrealStreamingAPI &&
        p.NeverModifyRaceState;
}

inline TracksideStreamingCatalog BuildTracksideStreamingCatalog(
    const WorldScaleVariantRecipe& circuit,
    const TracksideScenePlan& scene) noexcept
{
    TracksideStreamingCatalog out;
    if(!ValidWorldScaleVariantRecipe(circuit) ||
       !scene.Valid ||
       !scene.SourceOnly ||
       !scene.OriginalCollisionUnchanged ||
       scene.Circuit!=circuit.Id ||
       scene.QualifiedObjects!=TracksideSceneObjectCount)
        return out;
    out.Circuit=circuit.Id;
    out.LapLengthCm=circuit.LapLengthCm;
    out.SectorLengthCm=circuit.LapLengthCm/
        static_cast<double>(TracksideStreamingSectorCount);
    if(!std::isfinite(out.SectorLengthCm) ||
       out.SectorLengthCm<=1000.0)
        return {};
    for(unsigned i=0u;i<TracksideStreamingSectorCount;++i)
    {
        auto& sector=out.Sectors[i];
        sector.Index=i;
        sector.StartStationCm=i*out.SectorLengthCm;
        sector.EndStationCm=(i+1u)*out.SectorLengthCm;
        sector.MinimumStructureHeightCm=
            std::numeric_limits<double>::infinity();
        sector.MaximumStructureHeightCm=0.0;
    }
    for(std::size_t i=0u;i<scene.Objects.size();++i)
    {
        const auto& object=scene.Objects[i];
        if(!object.Valid ||
           object.Circuit!=circuit.Id ||
           object.DeterministicId==0u ||
           !object.EveryBaseCornerOutsideRoad ||
           object.UnrealActorSpawned ||
           object.DrivableCollisionAdded ||
           !std::isfinite(object.StationCm) ||
           !std::isfinite(object.HeightCm) ||
           object.HeightCm<=0.0 ||
           object.StationCm<0.0 ||
           object.StationCm>=circuit.LapLengthCm)
            return {};
        for(std::size_t j=0;j<i;++j)
            if(object.DeterministicId==
               scene.Objects[j].DeterministicId)
                return {};
        // Source art is authored in two ordered 64-station strips.
        // Use their exact source slots instead of a floating-point floor
        // at a sector seam (which can mis-bucket perfectly valid XYZ).
        // Still verify the station itself to reject forged placement.
        constexpr unsigned stationsPerSector=
            TracksideSceneStationsPerSide/
                TracksideStreamingSectorCount;
        static_assert(TracksideSceneStationsPerSide%
            TracksideStreamingSectorCount==0u);
        const unsigned originalStation=static_cast<unsigned>(
            i%TracksideSceneStationsPerSide);
        const double expectedStation=circuit.LapLengthCm*
            static_cast<double>(originalStation)/
                TracksideSceneStationsPerSide;
        if(std::abs(object.StationCm-expectedStation)>1e-5)
            return {};
        const unsigned sectorIndex=originalStation/
            stationsPerSector;
        auto& sector=out.Sectors[sectorIndex];
        if(sector.SourceObjects>=TracksideStreamingPropsPerSector ||
           object.StationCm+1e-7<sector.StartStationCm ||
           object.StationCm>=sector.EndStationCm+1e-7)
            return {};
        const unsigned slot=sector.SourceObjects++;
        sector.SourceObjectIndex[slot]=i;
        sector.ObjectIds[slot]=object.DeterministicId;
        sector.MinimumStructureHeightCm=std::min(
            sector.MinimumStructureHeightCm,
            object.HeightCm);
        sector.MaximumStructureHeightCm=std::max(
            sector.MaximumStructureHeightCm,
            object.HeightCm);
        ++out.QualifiedObjects;
    }
    for(auto& sector:out.Sectors)
    {
        if(sector.SourceObjects!=TracksideStreamingPropsPerSector ||
           !std::isfinite(sector.MinimumStructureHeightCm) ||
           sector.MinimumStructureHeightCm<=0.0 ||
           sector.MinimumStructureHeightCm>
               sector.MaximumStructureHeightCm)
            return {};
        sector.Valid=true;
        ++out.QualifiedSectors;
    }
    out.Valid=out.QualifiedObjects==TracksideSceneObjectCount &&
        out.QualifiedSectors==TracksideStreamingSectorCount;
    return out;
}

inline bool ValidTracksideStreamingObservation(
    const TracksideStreamingCatalog& catalog,
    TracksideStreamingObservation o,
    TracksideStreamingSettings p) noexcept
{
    return catalog.Valid &&
        catalog.Circuit==o.Circuit &&
        o.Session!=0u &&
        o.SessionRevision!=0u &&
        o.NativeVehicleToken!=0u &&
        o.SourceFrame!=0u &&
        o.ActualEngineRouteProjectionObserved &&
        o.OriginalWorldStaticRoadVerified &&
        o.OriginalPhysicalVehicleObserved &&
        std::isfinite(o.EngineClockSeconds) &&
        o.EngineClockSeconds>0.0 &&
        o.EngineClockSeconds<=p.MaximumCameraClockSeconds &&
        std::isfinite(o.ObservedStationCm) &&
        std::abs(o.ObservedStationCm)<=
            p.MaximumSourceStationCm &&
        std::isfinite(o.MeasuredSpeedCmps) &&
        o.MeasuredSpeedCmps>=0.0 &&
        o.MeasuredSpeedCmps<=p.MaximumMeasuredSpeedCmps;
}

inline unsigned TracksideSectorOfStation(
    const TracksideStreamingCatalog& catalog,
    double stationCm) noexcept
{
    if(!catalog.Valid ||
       !std::isfinite(stationCm) ||
       catalog.LapLengthCm<=0.0 ||
       catalog.SectorLengthCm<=0.0)
        return TracksideStreamingSectorCount;
    double wrapped=std::fmod(
        stationCm,catalog.LapLengthCm);
    if(wrapped<0.0)wrapped+=catalog.LapLengthCm;
    return std::min(
        static_cast<unsigned>(wrapped/catalog.SectorLengthCm),
        TracksideStreamingSectorCount-1u);
}

inline TracksideStreamingPlan PlanTracksideStreaming(
    const TracksideStreamingCatalog& catalog,
    TracksideStreamingObservation observation,
    TracksideStreamingSettings settings={}) noexcept
{
    TracksideStreamingPlan out;
    if(!ValidTracksideStreamingSettings(settings))
    {
        out.Fault=TracksideStreamingFault::BadPolicy;
        return out;
    }
    if(!catalog.Valid ||
       catalog.QualifiedSectors!=TracksideStreamingSectorCount ||
       catalog.QualifiedObjects!=TracksideSceneObjectCount)
    {
        out.Fault=TracksideStreamingFault::BadCatalog;
        return out;
    }
    if(!ValidTracksideStreamingObservation(
        catalog,observation,settings))
    {
        out.Fault=observation.Circuit!=catalog.Circuit ?
            TracksideStreamingFault::SourceEpochMismatch :
            TracksideStreamingFault::BadCamera;
        return out;
    }
    const unsigned current=TracksideSectorOfStation(
        catalog,observation.ObservedStationCm);
    if(current>=TracksideStreamingSectorCount)
    {
        out.Fault=TracksideStreamingFault::BadRoad;
        return out;
    }
    const double distance=observation.MeasuredSpeedCmps*
        settings.LookAheadSeconds;
    const unsigned predictive=static_cast<unsigned>(std::ceil(
        distance/catalog.SectorLengthCm));
    const unsigned warmAhead=std::clamp(
        predictive,settings.MinimumPrefetchSectors,
        settings.MaximumPrefetchSectors);
    if(warmAhead>settings.MaximumPrefetchSectorsBudget)
    {
        out.Fault=TracksideStreamingFault::ImpossibleResourceBudget;
        return out;
    }
    out.Circuit=catalog.Circuit;
    out.SourceSession=observation.Session;
    out.SourceRevision=observation.SessionRevision;
    out.SourceFrame=observation.SourceFrame;
    out.SourceClockSeconds=observation.EngineClockSeconds;
    out.WrappedStationCm=std::fmod(
        observation.ObservedStationCm,catalog.LapLengthCm);
    if(out.WrappedStationCm<0.0)
        out.WrappedStationCm+=catalog.LapLengthCm;
    out.CurrentSector=current;
    out.SourceLapLengthCm=catalog.LapLengthCm;
    out.MeasuredSpeedCmps=observation.MeasuredSpeedCmps;
    out.LookAheadSectors=warmAhead;
    for(unsigned index=0u;index<TracksideStreamingSectorCount;++index)
    {
        const auto& source=catalog.Sectors[index];
        if(!source.Valid ||
           source.Index!=index ||
           source.SourceObjects!=TracksideStreamingPropsPerSector)
        {
            out.Fault=TracksideStreamingFault::BadCatalog;
            return out;
        }
        const unsigned ahead=(index+TracksideStreamingSectorCount-
            current)%TracksideStreamingSectorCount;
        const unsigned behind=(current+TracksideStreamingSectorCount-
            index)%TracksideStreamingSectorCount;
        TracksideStreamingIntent intent=TracksideStreamingIntent::Cold;
        if(ahead<=settings.ResidentNextSectors ||
           behind<=settings.ResidentPreviousSectors)
        {
            intent=TracksideStreamingIntent::Resident;
            ++out.ResidentSectorCount;
            out.ResidentObjectCount+=source.SourceObjects;
        }
        else if(ahead>settings.ResidentNextSectors &&
                ahead<=settings.ResidentNextSectors+warmAhead)
        {
            intent=TracksideStreamingIntent::Prefetch;
            ++out.PrefetchSectorCount;
            out.PrefetchObjectCount+=source.SourceObjects;
        }
        else
            ++out.ColdSectorCount;
        out.Sectors[index]={
            index,intent,source.SourceObjects,
            source.StartStationCm,source.EndStationCm,
            false,false,false};
    }
    if(out.ResidentSectorCount>settings.MaximumResidentSectors ||
       out.PrefetchSectorCount>
           settings.MaximumPrefetchSectorsBudget ||
       out.ResidentObjectCount>settings.MaximumResidentObjects ||
       out.PrefetchObjectCount>settings.MaximumPrefetchedObjects ||
       out.ResidentSectorCount+out.PrefetchSectorCount+
           out.ColdSectorCount!=TracksideStreamingSectorCount)
    {
        out.Fault=TracksideStreamingFault::ImpossibleResourceBudget;
        return out;
    }
    out.Fault=TracksideStreamingFault::None;
    out.Valid=true;
    return out;
}

inline bool TracksideStreamingContinuity(
    const TracksideStreamingPlan& previous,
    const TracksideStreamingPlan& next) noexcept
{
    if(!previous.Valid || !next.Valid ||
       previous.Circuit!=next.Circuit ||
       previous.SourceSession!=next.SourceSession ||
       previous.SourceRevision!=next.SourceRevision ||
       next.SourceFrame<=previous.SourceFrame ||
       !std::isfinite(previous.SourceClockSeconds) ||
       !std::isfinite(next.SourceClockSeconds) ||
       next.SourceClockSeconds<=previous.SourceClockSeconds ||
       next.SourceClockSeconds-
           previous.SourceClockSeconds>0.25)
        return false;
    if(!std::isfinite(previous.WrappedStationCm) ||
       !std::isfinite(next.WrappedStationCm) ||
       !std::isfinite(previous.SourceLapLengthCm) ||
       !std::isfinite(next.SourceLapLengthCm) ||
       previous.SourceLapLengthCm<=0.0 ||
       next.SourceLapLengthCm!=previous.SourceLapLengthCm ||
       !std::isfinite(previous.MeasuredSpeedCmps) ||
       !std::isfinite(next.MeasuredSpeedCmps) ||
       previous.MeasuredSpeedCmps<0.0 ||
       next.MeasuredSpeedCmps<0.0)
        return false;
    const double lap=previous.SourceLapLengthCm;
    const double rawDistance=std::abs(
        next.WrappedStationCm-previous.WrappedStationCm);
    if(rawDistance>lap)return false;
    const double shortest=std::min(
        rawDistance,lap-rawDistance);
    const double elapsed=next.SourceClockSeconds-
        previous.SourceClockSeconds;
    const double maximumTravel=std::max(
        previous.MeasuredSpeedCmps,next.MeasuredSpeedCmps)*
        elapsed+200.0; // sampled projection and contact tolerance
    if(!std::isfinite(shortest) ||
       !std::isfinite(maximumTravel) ||
       shortest>maximumTravel)
        return false;
    const unsigned ahead=(next.CurrentSector+
        TracksideStreamingSectorCount-
        previous.CurrentSector)%TracksideStreamingSectorCount;
    const unsigned behind=(previous.CurrentSector+
        TracksideStreamingSectorCount-
        next.CurrentSector)%TracksideStreamingSectorCount;
    return std::min(ahead,behind)<=1u;
}
} // namespace latebrake
