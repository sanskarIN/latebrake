#pragma once
#include "Track/WorldScaleObservedWaterFilm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
namespace latebrake {
// Source-only preview mesh for water measured on an original physical road.
// Reflective strips have NO collision, grip, telemetry, score or Chaos
// authority. Real puddle materials and visual quality require Unreal QA.
inline constexpr std::size_t PuddleVisualMaximumSections=ObservedWaterCellCount;
inline constexpr std::size_t PuddleVisualMaximumVertices=
    PuddleVisualMaximumSections*4u;
inline constexpr std::size_t PuddleVisualMaximumTriangles=
    PuddleVisualMaximumSections*2u;
enum class PuddleVisualBand : std::uint8_t {
    Dry, Sheen, Puddle, StandingWater
};
struct PuddleVisualSettings final {
    double MinimumSheenMm=0.03;
    double MinimumPuddleMm=0.50;
    double MinimumStandingWaterMm=3.0;
    double SmallestHalfWidthCm=45.0;
    double LargestHalfWidthCm=285.0;
    double MaximumFilmMm=ObservedWaterMaximumMm;
    double VisualLiftCm=0.65;
    double MaximumVisualLiftCm=1.0;
    double MaxStationGapCm=1000.0;
    std::size_t MaximumEmittedSections=PuddleVisualMaximumSections;
    bool RequireObservedWaterSource=true;
    bool StayWithinOriginalPhysicalRoad=true;
    bool NeverEnableVisualCollision=true;
    bool NoOriginalRaceModification=true;
};
struct PuddleVisualSection final {
    bool Valid=false;
    PuddleVisualBand Band=PuddleVisualBand::Dry;
    unsigned SourceCell=0u;
    double StartStationCm=0.0;
    double EndStationCm=0.0;
    double WidthCm=0.0;
    double FilmMm=0.0;
    double BrightnessHint=0.0;
    double MaterialRoughnessHint=1.0;
    double EstimatedSurfaceAreaSqCm=0.0;
    std::array<RoundedSmoothVertex,4> Corners{};
    bool NoRoadContactAuthority=true;
};
struct PuddleVisualPlan final {
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceFrame=0u;
    std::array<PuddleVisualSection,PuddleVisualMaximumSections> Sections{};
    unsigned QualifiedSections=0u;
    unsigned DrySections=0u;
    unsigned SheenSections=0u;
    unsigned PuddleSections=0u;
    unsigned StandingWaterSections=0u;
    double TotalVisibleAreaSqCm=0.0;
    double MaximumWitnessedFilmMm=0.0;
    double MinimumRoadSeparationCm=0.0;
    double MaximumPlannedHeightCm=0.0;
    bool CollisionRoadUnchanged=true;
    bool NoIndependentPhysicalSurface=true;
    bool NoOfficialLapAuthority=true;
};
struct PuddleVisualMesh final {
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::vector<RoundedSmoothVertex> Vertices;
    std::vector<RoundedSmoothTriangle> Triangles;
    std::vector<PuddleVisualBand> Materials;
    unsigned IncludedSections=0u;
    bool VisualOnly=true;
    bool PhysicsSurfaceCreated=false;
};
inline bool ValidPuddleVisualSettings(PuddleVisualSettings s) noexcept {
    return std::isfinite(s.MinimumSheenMm) &&
        s.MinimumSheenMm>=0.01 && s.MinimumSheenMm<=0.20 &&
        std::isfinite(s.MinimumPuddleMm) &&
        s.MinimumPuddleMm>=s.MinimumSheenMm+0.1 &&
        s.MinimumPuddleMm<=2.0 &&
        std::isfinite(s.MinimumStandingWaterMm) &&
        s.MinimumStandingWaterMm>=s.MinimumPuddleMm+0.5 &&
        s.MinimumStandingWaterMm<=7.0 &&
        std::isfinite(s.SmallestHalfWidthCm) &&
        s.SmallestHalfWidthCm>=25.0 &&
        s.SmallestHalfWidthCm<=75.0 &&
        std::isfinite(s.LargestHalfWidthCm) &&
        s.LargestHalfWidthCm>=s.SmallestHalfWidthCm+100.0 &&
        s.LargestHalfWidthCm<=320.0 &&
        std::isfinite(s.MaximumFilmMm) &&
        s.MaximumFilmMm>=s.MinimumStandingWaterMm &&
        s.MaximumFilmMm<=ObservedWaterMaximumMm &&
        std::isfinite(s.VisualLiftCm) &&
        s.VisualLiftCm>=0.1 &&
        s.VisualLiftCm<=s.MaximumVisualLiftCm &&
        std::isfinite(s.MaximumVisualLiftCm) &&
        s.MaximumVisualLiftCm>=0.5 &&
        s.MaximumVisualLiftCm<=1.5 &&
        std::isfinite(s.MaxStationGapCm) &&
        s.MaxStationGapCm>=300.0 &&
        s.MaxStationGapCm<=2000.0 &&
        s.MaximumEmittedSections>=1u &&
        s.MaximumEmittedSections<=PuddleVisualMaximumSections &&
        s.RequireObservedWaterSource &&
        s.StayWithinOriginalPhysicalRoad &&
        s.NeverEnableVisualCollision &&
        s.NoOriginalRaceModification;
}
inline PuddleVisualBand ClassifyPuddleVisualFilm(
    double filmMm,PuddleVisualSettings settings={}) noexcept
{
    if(!std::isfinite(filmMm) || filmMm<0.0 ||
       filmMm>settings.MaximumFilmMm)
        return PuddleVisualBand::Dry;
    if(filmMm>=settings.MinimumStandingWaterMm)
        return PuddleVisualBand::StandingWater;
    if(filmMm>=settings.MinimumPuddleMm)
        return PuddleVisualBand::Puddle;
    if(filmMm>=settings.MinimumSheenMm)
        return PuddleVisualBand::Sheen;
    return PuddleVisualBand::Dry;
}
inline double PuddleVisualHalfWidth(
    double film,PuddleVisualSettings s) noexcept
{
    const double fraction=std::clamp(
        film/s.MaximumFilmMm,0.0,1.0);
    return s.SmallestHalfWidthCm+
        std::sqrt(fraction)*
        (s.LargestHalfWidthCm-s.SmallestHalfWidthCm);
}
inline double PuddleVisualRoughness(
    double film,PuddleVisualSettings s) noexcept
{
    const double fraction=std::clamp(
        film/s.MaximumFilmMm,0.0,1.0);
    return std::clamp(0.38-0.30*fraction,0.05,1.0);
}
inline PuddleVisualSection SamplePuddleVisualSection(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile road,
    const ObservedWaterResult& source,
    PuddleVisualSettings settings,
    unsigned cell) noexcept
{
    PuddleVisualSection out;
    if(!ValidWorldScaleVariantRecipe(circuit) ||
       !ValidWorldScaleBankedRoadProfile(circuit,road) ||
       !ValidPuddleVisualSettings(settings) ||
       !source.Valid ||
       source.Circuit!=circuit.Id ||
       source.Session==0u ||
       source.SourceFrame==0u ||
       cell>=PuddleVisualMaximumSections ||
       !std::isfinite(source.CellLengthCm) ||
       source.CellLengthCm<=0.0 ||
       source.CellLengthCm>settings.MaxStationGapCm)
        return out;
    const double film=source.FilmMm[cell];
    const auto band=ClassifyPuddleVisualFilm(film,settings);
    out.SourceCell=cell;
    out.StartStationCm=cell*source.CellLengthCm;
    out.EndStationCm=(cell+1u)*source.CellLengthCm;
    out.FilmMm=film;
    out.Band=band;
    if(band==PuddleVisualBand::Dry){
        out.Valid=std::isfinite(film) && film>=0.0 &&
            film<=settings.MaximumFilmMm;
        return out;
    }
    const double halfWidth=PuddleVisualHalfWidth(film,settings);
    if(!std::isfinite(halfWidth) ||
       halfWidth>circuit.VisualHalfWidthCm)return out;
    const std::array<double,2> stations{{
        out.StartStationCm,out.EndStationCm
    }};
    unsigned filled=0u;
    for(const double station:stations){
        for(const double side:{-1.0,1.0}){
            const auto sample=SampleWorldScaleBankedRoad(
                circuit,road,station,side*halfWidth);
            if(!sample.Valid ||
               !sample.OriginalRoadXYQualified ||
               !sample.OriginalCollisionUnaffected)
                return {};
            const RoundedSmoothVertex point{
                sample.WorldXcm,
                sample.WorldYcm,
                sample.WorldZcm+settings.VisualLiftCm
            };
            if(!IsFiniteRoundedSmoothVertex(point) ||
               !IsOnWorldScaleVariantRoad(
                   circuit,{point.X,point.Y}))
                return {};
            out.Corners[filled++]=point;
        }
    }
    out.WidthCm=2.0*halfWidth;
    out.EstimatedSurfaceAreaSqCm=
        out.WidthCm*source.CellLengthCm;
    out.BrightnessHint=std::clamp(
        0.1+0.65*std::sqrt(film/settings.MaximumFilmMm),
        0.0,0.8);
    out.MaterialRoughnessHint=PuddleVisualRoughness(film,settings);
    out.Valid=filled==4u &&
        std::isfinite(out.EstimatedSurfaceAreaSqCm) &&
        out.EstimatedSurfaceAreaSqCm>0.0;
    return out;
}
inline PuddleVisualPlan BuildPuddleVisualPlan(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile road,
    const ObservedWaterResult& source,
    PuddleVisualSettings settings={}) noexcept
{
    PuddleVisualPlan out;
    if(!ValidPuddleVisualSettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       !ValidWorldScaleBankedRoadProfile(circuit,road) ||
       !source.Valid ||
       source.Circuit!=circuit.Id ||
       !source.RenderAdvisoryOnly ||
       !source.OriginalRaceUnaffected ||
       source.CellLengthCm<=0.0)
        return out;
    out.Circuit=circuit.Id;
    out.SourceSession=source.Session;
    out.SourceFrame=source.SourceFrame;
    out.MaximumPlannedHeightCm=
        -std::numeric_limits<double>::infinity();
    out.MinimumRoadSeparationCm=
        std::numeric_limits<double>::infinity();
    for(unsigned i=0u;i<PuddleVisualMaximumSections;++i){
        const auto sample=SamplePuddleVisualSection(
            circuit,road,source,settings,i);
        if(!sample.Valid)return {};
        out.Sections[i]=sample;
        out.MaximumWitnessedFilmMm=std::max(
            out.MaximumWitnessedFilmMm,sample.FilmMm);
        if(sample.Band==PuddleVisualBand::Dry){
            ++out.DrySections;
            continue;
        }
        if(out.QualifiedSections>=settings.MaximumEmittedSections)
            return {};
        ++out.QualifiedSections;
        if(sample.Band==PuddleVisualBand::Sheen)
            ++out.SheenSections;
        if(sample.Band==PuddleVisualBand::Puddle)
            ++out.PuddleSections;
        if(sample.Band==PuddleVisualBand::StandingWater)
            ++out.StandingWaterSections;
        out.TotalVisibleAreaSqCm+=
            sample.EstimatedSurfaceAreaSqCm;
        out.MinimumRoadSeparationCm=std::min(
            out.MinimumRoadSeparationCm,
            circuit.RoadHalfWidthCm-sample.WidthCm*0.5);
        for(const auto& corner:sample.Corners)
            out.MaximumPlannedHeightCm=std::max(
                out.MaximumPlannedHeightCm,corner.Z);
    }
    if(out.QualifiedSections==0u){
        out.MinimumRoadSeparationCm=circuit.RoadHalfWidthCm;
        out.MaximumPlannedHeightCm=road.RoadTopCm;
    }
    out.Valid=out.DrySections+out.QualifiedSections==
            PuddleVisualMaximumSections &&
        out.QualifiedSections==
            out.SheenSections+out.PuddleSections+
            out.StandingWaterSections &&
        out.MinimumRoadSeparationCm>=100.0 &&
        std::isfinite(out.MaximumPlannedHeightCm) &&
        std::isfinite(out.TotalVisibleAreaSqCm);
    return out;
}
inline PuddleVisualMesh BuildPuddleVisualMesh(
    const PuddleVisualPlan& plan)
{
    PuddleVisualMesh out;
    if(!plan.Valid || !plan.CollisionRoadUnchanged ||
       !plan.NoIndependentPhysicalSurface ||
       !plan.NoOfficialLapAuthority)return out;
    out.Circuit=plan.Circuit;
    out.Vertices.reserve(plan.QualifiedSections*4u);
    out.Triangles.reserve(plan.QualifiedSections*2u);
    out.Materials.reserve(plan.QualifiedSections*2u);
    for(const auto& section:plan.Sections){
        if(!section.Valid)return {};
        if(section.Band==PuddleVisualBand::Dry)continue;
        if(out.Vertices.size()+4u>PuddleVisualMaximumVertices ||
           out.Triangles.size()+2u>PuddleVisualMaximumTriangles)
            return {};
        const auto first=static_cast<std::uint32_t>(
            out.Vertices.size());
        for(const auto& vertex:section.Corners){
            if(!IsFiniteRoundedSmoothVertex(vertex))
                return {};
            out.Vertices.push_back(vertex);
        }
        out.Triangles.push_back({{{
            first,first+1u,first+2u}}});
        out.Triangles.push_back({{{
            first+1u,first+3u,first+2u}}});
        out.Materials.push_back(section.Band);
        out.Materials.push_back(section.Band);
        ++out.IncludedSections;
    }
    out.Valid=out.IncludedSections==plan.QualifiedSections &&
        out.Vertices.size()==out.IncludedSections*4u &&
        out.Triangles.size()==out.IncludedSections*2u &&
        out.Materials.size()==out.Triangles.size();
    return out;
}
} // namespace latebrake
