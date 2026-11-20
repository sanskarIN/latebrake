#include "Track/WorldScaleRunoffTerrainPlan.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition,const char* label)
{
    if(!condition)
    {
        std::cerr<<"WorldScaleRunoffTerrain FAIL: "<<label<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double epsilon=1e-7)
{
    return std::abs(a-b)<=epsilon;
}
}

int main()
{
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    unsigned passingCircuits=0u;
    for(unsigned i=0u;i<WorldScaleVariantCount;++i)
    {
        const auto circuit=WorldScaleOriginalVariantRecipes[i];
        WorldScaleBankedRoadProfile banked;
        Require(ChooseWorldScaleBankedRoadProfile(
            circuit.Id,banked),"authentic road bank profile required");
        WorldScaleRunoffParameters settings;
        Require(ValidWorldScaleRunoffParameters(circuit,settings),
                "default runoff safety policy valid on six original roads");
        Require(WorldScaleClassifyRunoff(
                    circuit.RoadHalfWidthCm,settings)==
                    WorldScaleRunoffSurface::Unqualified,
                "road-driving corridor cannot become off-road terrain");
        Require(WorldScaleClassifyRunoff(
                    settings.InnerBandStartCm,settings)==
                    WorldScaleRunoffSurface::GravelTrap,
                "gravel starts outside collision-bearing asphalt");
        Require(WorldScaleClassifyRunoff(
                    settings.DrainageBandEndCm,settings)==
                    WorldScaleRunoffSurface::EarthBerm,
                "outer berm remains an explicit source-only surface");

        const auto section=SampleWorldScaleRunoffSection(
            circuit,banked,settings,0.125*circuit.LapLengthCm);
        Require(section.Valid &&
                section.Circuit==circuit.Id &&
                section.MinimumRoadSeparationCm>=100.0 &&
                section.MaximumCrossSlope<=0.3 &&
                section.LiveCollisionUnchanged,
                "sample cross section never overlaps original road");
        for(unsigned side=0;side<WorldScaleRunoffSideCount;++side)
        {
            double previousOffset=0.0;
            for(unsigned col=0;col<WorldScaleRunoffColumnsPerSide;++col)
            {
                const auto& point=section.Points[
                    side*WorldScaleRunoffColumnsPerSide+col];
                Require(point.Valid &&
                        point.OutsideRoadCollision &&
                        !point.ContactAuthorityIssued &&
                        point.SourceCircuit==circuit.Id,
                        "verified gravel/ditch/berm point cannot issue contact");
                Require(std::abs(point.SignedSideOffsetCm)>
                            previousOffset,
                        "successive terrain bands stay outside their predecessor");
                Require(!IsOnWorldScaleVariantRoad(
                    circuit,{point.Xcm,point.Ycm}),
                    "no terrain point is embedded in authentic WorldStatic road");
                previousOffset=std::abs(point.SignedSideOffsetCm);
            }
        }

        const auto start=SampleWorldScaleRunoffSection(
            circuit,banked,settings,0.0);
        const auto finish=SampleWorldScaleRunoffSection(
            circuit,banked,settings,circuit.LapLengthCm);
        const auto reverse=SampleWorldScaleRunoffSection(
            circuit,banked,settings,-circuit.LapLengthCm);
        Require(start.Valid && finish.Valid && reverse.Valid,
                "periodic original 3D terrain survives signed laps");
        for(unsigned k=0;k<WorldScaleRunoffVerticesPerSection;++k)
        {
            Require(Near(start.Points[k].Zcm,finish.Points[k].Zcm) &&
                    Near(start.Points[k].Zcm,reverse.Points[k].Zcm),
                    "terrain seam must repeat height without crack");
            Require(Near(start.Points[k].Xcm,finish.Points[k].Xcm) &&
                    Near(start.Points[k].Ycm,finish.Points[k].Ycm),
                    "roadside shoulder seam XY fully closes");
        }
        for(double distance : {
            0.0,
            circuit.LapLengthCm/6.0,
            circuit.LapLengthCm*0.47,
            circuit.LapLengthCm-1.0})
        {
            const auto left=SampleWorldScaleRunoff(
                circuit,banked,settings,distance,
                settings.GravelBandEndCm);
            const auto right=SampleWorldScaleRunoff(
                circuit,banked,settings,distance,
                -settings.GravelBandEndCm);
            Require(left.Valid && right.Valid &&
                    left.Surface==WorldScaleRunoffSurface::DrainageSwale &&
                    right.Surface==WorldScaleRunoffSurface::DrainageSwale &&
                    left.OutsideRoadCollision &&
                    right.OutsideRoadCollision,
                    "symmetric drainage boundary is off-road on either side");
            Require(left.DrainageDepthCm>0.0 &&
                    right.DrainageDepthCm>0.0 &&
                    Near(left.DrainageDepthCm,right.DrainageDepthCm),
                    "drainage trough has consistent designed depth");
        }

        const auto plan=BuildWorldScaleRunoffMesh(
            circuit,banked,settings);
        Require(plan.Valid &&
                plan.Circuit==circuit.Id &&
                plan.QualifiedSections==WorldScaleRunoffSections &&
                plan.QualifiedVertices==
                    WorldScaleRunoffSections*
                    WorldScaleRunoffVerticesPerSection &&
                plan.Visual.Triangles.size()==
                    WorldScaleRunoffMeshTriangles &&
                plan.Visual.Vertices.size()==
                    WorldScaleRunoffSections*
                    WorldScaleRunoffVerticesPerSection &&
                plan.CannotBeDrivenOn &&
                plan.NoUnrealMeshInstalled &&
                plan.OriginalPhysicalRoadUnchanged,
                "closed-lap off-road ribbon has exact original face budget");
        Require(std::isfinite(plan.MinimumHeightCm) &&
                std::isfinite(plan.MaximumHeightCm) &&
                plan.MinimumHeightCm<=plan.MaximumHeightCm &&
                plan.MinimumSourceRoadGapCm>=100.0,
                "terrain survey proves safe mesh source bounds");
        for(const auto& triangle:plan.Visual.Triangles)
        {
            const auto& indices=triangle.Indices;
            Require(indices[0]<plan.Visual.Vertices.size() &&
                    indices[1]<plan.Visual.Vertices.size() &&
                    indices[2]<plan.Visual.Vertices.size(),
                    "no out-of-range terrain triangle index");
            Require(indices[0]!=indices[1] &&
                    indices[0]!=indices[2] &&
                    indices[1]!=indices[2],
                    "all terrain triangles use independent vertices");
        }
        for(unsigned sectionIndex=0;sectionIndex<WorldScaleRunoffSections;
            ++sectionIndex)
        {
            for(unsigned col=0;col<WorldScaleRunoffVerticesPerSection;++col)
            {
                const auto& v=plan.Visual.Vertices[
                    sectionIndex*WorldScaleRunoffVerticesPerSection+col];
                Require(!IsOnWorldScaleVariantRoad(
                    circuit,{v.X,v.Y}),
                    "entire 3072-vertex visual terrain stays non-colliding");
            }
        }

        settings.RequirePhysicalRoadSeparation=false;
        Require(!BuildWorldScaleRunoffMesh(
                    circuit,banked,settings).Valid,
                "cannot disable original asphalt separation proof");
        settings.RequirePhysicalRoadSeparation=true;
        settings.PreserveOriginalRoadCollision=false;
        Require(!BuildWorldScaleRunoffMesh(
                    circuit,banked,settings).Valid,
                "cannot replace original collision with speculative terrain");
        settings.PreserveOriginalRoadCollision=true;
        settings.InnerBandStartCm=circuit.RoadHalfWidthCm;
        Require(!SampleWorldScaleRunoff(
                    circuit,banked,settings,0.0,650.0).Valid,
                "policy does not admit a new drivable terrain within asphalt");
        ++passingCircuits;
    }
    Require(passingCircuits==WorldScaleVariantCount,
            "six independent original layouts qualified");
    const auto circuit=WorldScaleOriginalVariantRecipes[0];
    WorldScaleBankedRoadProfile banked;
    Require(ChooseWorldScaleBankedRoadProfile(
        circuit.Id,banked),"choose source-only road");
    Require(!SampleWorldScaleRunoff(
                circuit,banked,{},nan,650.0).Valid,
            "nonfinite station rejected");
    Require(!SampleWorldScaleRunoff(
                circuit,banked,{},0.0,nan).Valid,
            "nonfinite offset rejected");
    Require(!SampleWorldScaleRunoff(
                circuit,banked,{},0.0,0.0).Valid,
            "center racing lane never becomes gravel");
    std::cout<<"WorldScaleRunoffTerrain: six 3D circuit visual plans passed\n";
    return EXIT_SUCCESS;
}
