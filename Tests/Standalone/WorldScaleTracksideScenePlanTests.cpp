#include "Track/WorldScaleTracksideScenePlan.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool yes,const char* message)
{
    if(!yes)
    {
        std::cerr<<"WorldScaleTracksideScene FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1.0e-7)
{
    return std::abs(a-b)<=tolerance;
}
}

int main()
{
    using namespace latebrake;
    TracksideSceneSettings settings;
    Require(ValidTracksideSceneSettings(settings),
            "default original noncolliding trackside settings valid");
    settings.NoRuntimeActorSpawn=false;
    Require(!ValidTracksideSceneSettings(settings),
            "source scene can never authorize new game actors");
    settings={};
    settings.VisualMeshesHaveNoCollision=false;
    Require(!ValidTracksideSceneSettings(settings),
            "unqualified static meshes cannot become game collision");
    settings={};
    settings.MaximumStructureHeightCm=100.0;
    Require(!ValidTracksideSceneSettings(settings),
            "nonphysical tiny height bound rejected");
    settings={};
    settings.MaximumProxyDrawDistanceCm=
        std::numeric_limits<double>::infinity();
    Require(!ValidTracksideSceneSettings(settings),
            "nonfinite maximum draw range rejected");
    settings={};

    const auto invalidRecipe=TracksideKindRecipe(
        static_cast<TracksideSceneKind>(250));
    Require(!ValidTracksideRecipe(invalidRecipe),
            "invalid trackside kind has no geometry permission");
    unsigned confirmedCircuits=0u;
    for(unsigned i=0u;i<WorldScaleVariantCount;++i)
    {
        const auto circuit=WorldScaleOriginalVariantRecipes[i];
        WorldScaleBankedRoadProfile profile;
        Require(ChooseWorldScaleBankedRoadProfile(
                    circuit.Id,profile),
                "real future-circuit road profile chosen");
        const auto scene=BuildTracksideScenePlan(
            circuit,profile);
        Require(scene.Valid &&
                scene.Circuit==circuit.Id &&
                scene.QualifiedObjects==TracksideSceneObjectCount &&
                scene.OriginalCollisionUnchanged &&
                scene.SourceOnly &&
                scene.MinimumObjectRoadClearanceCm>=175.0 &&
                scene.TallestObjectCm<=1600.0 &&
                scene.SmallestForwardObjectSpacingCm>=1700.0,
                "128 original noncolliding placements per planned circuit");
        const auto again=BuildTracksideScenePlan(
            circuit,profile);
        Require(again.Valid &&
                again.QualifiedObjects==scene.QualifiedObjects &&
                Near(again.MinimumObjectRoadClearanceCm,
                     scene.MinimumObjectRoadClearanceCm),
                "original scene reproducible without random/time dependence");
        std::array<unsigned,7> byKind{};
        for(unsigned s=0u;s<2u;++s)
        {
            double previousStation=-1.0;
            for(unsigned k=0u;k<TracksideSceneStationsPerSide;++k)
            {
                const auto id=s*TracksideSceneStationsPerSide+k;
                const auto& anchor=scene.Objects[id];
                const auto& repeat=again.Objects[id];
                Require(anchor.Valid &&
                        anchor.Circuit==circuit.Id &&
                        anchor.EveryBaseCornerOutsideRoad &&
                        !anchor.UnrealActorSpawned &&
                        !anchor.DrivableCollisionAdded &&
                        anchor.DeterministicId!=0 &&
                        anchor.MaterialIndex>0u &&
                        anchor.MaterialIndex<=7u,
                        "each source-only structure has stable authentic identity");
                Require(anchor.StationCm>previousStation &&
                        anchor.StationCm<circuit.LapLengthCm &&
                        std::abs(anchor.SignedOffsetCm)==2400.0 &&
                        anchor.HeightCm>=120.0 &&
                        anchor.HeightCm<=1600.0,
                        "ordered trackside placement uses real road distance");
                Require(Near(anchor.Xcm,repeat.Xcm) &&
                        Near(anchor.Ycm,repeat.Ycm) &&
                        Near(anchor.Zcm,repeat.Zcm) &&
                        anchor.DeterministicId==repeat.DeterministicId,
                        "same original scene input produces identical XYZ/IDs");
                Require(!IsOnWorldScaleVariantRoad(
                    circuit,{anchor.Xcm,anchor.Ycm}),
                    "structure centroid outside every original asphalt piece");
                Require(TracksideFootprintEntirelyOffRoad(
                    circuit,{anchor.Xcm,anchor.Ycm},
                    anchor.YawRadians,
                    anchor.HalfWidthCm,anchor.HalfLengthCm),
                    "full nine-point footprint cannot overlap live road");
                const auto recipe=TracksideKindRecipe(anchor.Kind);
                Require(ValidTracksideRecipe(recipe) &&
                        Near(recipe.HeightCm,anchor.HeightCm) &&
                        Near(recipe.HalfWidthCm,anchor.HalfWidthCm),
                        "source kind recipe matches emitted geometry");
                ++byKind[static_cast<std::size_t>(anchor.Kind)];
                previousStation=anchor.StationCm;
                if(k>0)
                {
                    const auto& prior=scene.Objects[id-1u];
                    Require(anchor.StationCm-prior.StationCm>
                                settings.MinimumAdjacentStationGapCm,
                            "same-side nearby installations remain separated");
                }
            }
        }
        for(unsigned count:byKind)
            Require(count>0u,"all seven classes exist on every circuit");
        const auto raw=BuildTracksideProxyMesh(scene);
        Require(raw.Valid &&
                raw.Circuit==circuit.Id &&
                raw.QualifiedObjects==TracksideSceneObjectCount &&
                !raw.HasCollisionBodies &&
                !raw.InstalledInUnreal &&
                raw.Vertices.size()>=TracksideSceneObjectCount*8u &&
                raw.Triangles.size()==raw.TriangleMaterial.size(),
                "bounded source-only original 3D silhouettes constructed");
        for(std::size_t k=0;k<raw.Triangles.size();++k)
        {
            const auto& t=raw.Triangles[k].Indices;
            Require(t[0]<raw.Vertices.size() &&
                    t[1]<raw.Vertices.size() &&
                    t[2]<raw.Vertices.size() &&
                    t[0]!=t[1] && t[0]!=t[2] && t[1]!=t[2],
                    "proxy mesh has valid nondegenerate triangle indices");
            Require(raw.TriangleMaterial[k]>=1u &&
                    raw.TriangleMaterial[k]<=7u,
                    "visual-only triangles preserve material category");
        }
        for(const auto& vertex:raw.Vertices)
        {
            Require(IsFiniteRoundedSmoothVertex(vertex) &&
                    !IsOnWorldScaleVariantRoad(
                        circuit,{vertex.X,vertex.Y}),
                    "all original proxy art vertices remain off physics road");
        }
        settings.MinimumAdjacentStationGapCm=4000.0;
        Require(!BuildTracksideScenePlan(
                    circuit,profile,{},settings).Valid,
                "unrealistic object-density contract fails closed");
        settings={};
        auto invalidScene=scene;
        invalidScene.Valid=false;
        Require(!BuildTracksideProxyMesh(
                    invalidScene).Valid,
                "unqualified layout cannot emit partial fake scene meshes");
        ++confirmedCircuits;
    }
    Require(confirmedCircuits==WorldScaleVariantCount,
            "all six independent roads have original trackside scene QA");
    const auto original=WorldScaleOriginalVariantRecipes[0];
    WorldScaleBankedRoadProfile profile;
    Require(ChooseWorldScaleBankedRoadProfile(
                original.Id,profile),
            "choose initial track");
    Require(!PlaceTracksideObject(
                original,profile,{},settings,
                TracksideSceneStationsPerSide,0u).Valid,
            "out-of-range station slot rejected");
    Require(!PlaceTracksideObject(
                original,profile,{},settings,0u,2u).Valid,
            "out-of-range side rejected");
    Require(!TracksideFootprintEntirelyOffRoad(
                original,{0.0,-original.HalfExtentYcm},
                0.0,100.0,100.0),
            "actual asphalt centroid cannot be reclassified as art");
    std::cout<<"WorldScaleTracksideScene: original 128-prop six-circuit QA passed\n";
    return EXIT_SUCCESS;
}
