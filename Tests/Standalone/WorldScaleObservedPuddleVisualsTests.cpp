#include "Track/WorldScaleObservedPuddleVisuals.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool value,const char* what) {
    if(!value){std::cerr<<"PuddleVisuals FAIL: "<<what<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-6) {
    return std::abs(a-b)<=tolerance;
}
latebrake::ObservedWaterResult Water(
    const latebrake::WorldScaleVariantRecipe& road,double film)
{
    latebrake::ObservedWaterResult output;
    output.Valid=true;
    output.Fault=latebrake::ObservedWaterFault::None;
    output.Circuit=road.Id;
    output.Session=10u;
    output.Revision=1u;
    output.SourceFrame=20u;
    output.ClockSeconds=1.0;
    output.StepSeconds=0.05;
    output.CellLengthCm=road.LapLengthCm/
        static_cast<double>(latebrake::ObservedWaterCellCount);
    output.FilmMm.fill(film);
    output.RenderAdvisoryOnly=true;
    output.OriginalRaceUnaffected=true;
    return output;
}
}
int main() {
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=PuddleVisualSettings{};
    Check(ValidPuddleVisualSettings(settings),
        "conservative default visual thresholds");
    settings.NeverEnableVisualCollision=false;
    Check(!ValidPuddleVisualSettings(settings),
        "puddles cannot own physical collision");
    settings={};
    settings.MinimumPuddleMm=nan;
    Check(!ValidPuddleVisualSettings(settings),
        "invalid moisture threshold rejected");
    settings={};
    Check(ClassifyPuddleVisualFilm(0.0)==PuddleVisualBand::Dry &&
          ClassifyPuddleVisualFilm(0.08)==PuddleVisualBand::Sheen &&
          ClassifyPuddleVisualFilm(1.0)==PuddleVisualBand::Puddle &&
          ClassifyPuddleVisualFilm(5.0)==PuddleVisualBand::StandingWater &&
          ClassifyPuddleVisualFilm(nan)==PuddleVisualBand::Dry,
          "source water classes distinguish sheen and deep water");
    Check(PuddleVisualHalfWidth(10.0,settings)>
              PuddleVisualHalfWidth(1.0,settings) &&
          PuddleVisualRoughness(10.0,settings)<
              PuddleVisualRoughness(1.0,settings),
          "visually deeper measured water covers a wider smoother surface");
    unsigned verified=0u;
    for(unsigned id=0u;id<WorldScaleVariantCount;++id) {
        const auto circuit=WorldScaleOriginalVariantRecipes[id];
        WorldScaleBankedRoadProfile road;
        Check(ChooseWorldScaleBankedRoadProfile(circuit.Id,road),
              "source road has validated six-circuit bank/profile");
        auto measured=Water(circuit,0.0);
        auto dry=BuildPuddleVisualPlan(circuit,road,measured);
        Check(dry.Valid && dry.DrySections==ObservedWaterCellCount &&
              dry.QualifiedSections==0u &&
              Near(dry.TotalVisibleAreaSqCm,0.0) &&
              dry.CollisionRoadUnchanged &&
              dry.NoIndependentPhysicalSurface &&
              dry.NoOfficialLapAuthority,
              "fully dry measurements never fabricate a wet road");
        const auto dryMesh=BuildPuddleVisualMesh(dry);
        Check(dryMesh.Valid && dryMesh.Vertices.empty() &&
              dryMesh.Triangles.empty() &&
              !dryMesh.PhysicsSurfaceCreated,
              "no source wetness means no mesh geometry");
        measured=Water(circuit,0.0);
        measured.FilmMm[0]=2.0;
        measured.FilmMm[1]=0.10;
        measured.FilmMm[ObservedWaterCellCount-1u]=6.0;
        const auto preview=BuildPuddleVisualPlan(circuit,road,measured);
        Check(preview.Valid &&
              preview.DrySections==ObservedWaterCellCount-3u &&
              preview.PuddleSections==1u &&
              preview.SheenSections==1u &&
              preview.StandingWaterSections==1u &&
              preview.QualifiedSections==3u &&
              preview.TotalVisibleAreaSqCm>0.0 &&
              preview.MinimumRoadSeparationCm>=100.0 &&
              Near(preview.MaximumWitnessedFilmMm,6.0),
              "three measured puddle bands preserve exact source stations");
        Check(preview.Sections[0].Valid &&
              preview.Sections[0].SourceCell==0u &&
              preview.Sections[0].FilmMm==2.0 &&
              preview.Sections[0].Band==PuddleVisualBand::Puddle,
              "first wet fragment follows original start line");
        const auto section=preview.Sections[0];
        Check(Near(section.EndStationCm-section.StartStationCm,
                   measured.CellLengthCm) &&
              section.WidthCm>=settings.SmallestHalfWidthCm*2.0 &&
              section.WidthCm<=settings.LargestHalfWidthCm*2.0 &&
              section.BrightnessHint>0.0 &&
              section.MaterialRoughnessHint>0.0 &&
              section.EstimatedSurfaceAreaSqCm>0.0,
              "source film informs width and reflection material, not physics");
        for(const auto& point:section.Corners)
            Check(IsFiniteRoundedSmoothVertex(point) &&
                  IsOnWorldScaleVariantRoad(circuit,{point.X,point.Y}),
                  "all wet sheen visual vertices stay on authentic asphalt");
        const auto visual=BuildPuddleVisualMesh(preview);
        Check(visual.Valid &&
              visual.Vertices.size()==12u &&
              visual.Triangles.size()==6u &&
              visual.Materials.size()==6u &&
              visual.IncludedSections==3u &&
              visual.VisualOnly &&
              !visual.PhysicsSurfaceCreated,
              "precisely three original sections produce six triangles");
        for(const auto& tri:visual.Triangles) {
            const auto& indices=tri.Indices;
            Check(indices[0]<visual.Vertices.size() &&
                  indices[1]<visual.Vertices.size() &&
                  indices[2]<visual.Vertices.size() &&
                  indices[0]!=indices[1] &&
                  indices[0]!=indices[2] &&
                  indices[1]!=indices[2],
                  "future Unreal render sections have valid nondegenerate indices");
        }
        const auto repeat=BuildPuddleVisualPlan(circuit,road,measured);
        Check(repeat.Valid &&
              repeat.QualifiedSections==preview.QualifiedSections &&
              Near(repeat.TotalVisibleAreaSqCm,
                   preview.TotalVisibleAreaSqCm),
              "same measured water produces deterministic geometry");
        measured.FilmMm[0]=nan;
        Check(!BuildPuddleVisualPlan(circuit,road,measured).Valid,
              "unknown wetness fails closed even at one station");
        measured=Water(circuit,0.1);
        measured.Valid=false;
        Check(!BuildPuddleVisualPlan(circuit,road,measured).Valid,
              "no puddle preview from unverified water source");
        measured=Water(circuit,1.0);
        measured.RenderAdvisoryOnly=false;
        Check(!BuildPuddleVisualPlan(circuit,road,measured).Valid,
              "water from gameplay actuator cannot masquerade as visual source");
        measured=Water(circuit,5.0);
        settings.MaximumEmittedSections=5u;
        Check(!BuildPuddleVisualPlan(circuit,road,measured,settings).Valid,
              "wet scene cannot exceed fixed authored quad budget");
        settings={};
        ++verified;
    }
    Check(verified==WorldScaleVariantCount,
          "six genuinely distinct source circuits passed puddle QA");
    std::cout<<"ObservedPuddleVisuals: six circuits passed\n";
    return EXIT_SUCCESS;
}
