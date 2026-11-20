#include "Track/WorldScaleVariantRacePace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool yes,const char* why)
{
    if(!yes)
    {
        std::cerr<<"WorldScaleOriginalCircuits FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

double DiffOnLap(double a,double b,double lap)
{
    const double direct=std::abs(a-b);
    return std::min(direct,lap-direct);
}

void RoadAndPace(const latebrake::WorldScaleVariantRecipe& recipe)
{
    using namespace latebrake;
    Check(ValidWorldScaleVariantRecipe(recipe) &&
          recipe.RoadBlockCount==68 &&
          recipe.RoadHalfWidthCm==450.0 &&
          recipe.VisualHalfWidthCm==425.0 &&
          recipe.LapLengthCm>120000.0 &&
          recipe.LapLengthCm<260000.0,
          "six distinct source recipes have physically plausible dimensions");

    for(int index=0;index<recipe.RoadBlockCount;++index)
    {
        const auto slab=WorldScaleVariantPhysicalRoadPiece(
            recipe,index);
        Check(IsValidRoundedPhysicalRoadPiece(slab),
              "every one of 68 future circuit slabs is dimensionally valid");
    }
    Check(WorldScaleVariantContinuousRoadProof(recipe,1024u),
          "512 authored XY headings and 9m physically backed road sections");

    const auto visual=BuildWorldScaleVariantVisualMesh(recipe);
    Check(visual.Vertices.size()==4u*1024u &&
          visual.Triangles.size()==8u*1024u,
          "all six source circuits have a closed noncolliding visual ribbon");
    for(const auto& v:visual.Vertices)
    {
        Check(IsFiniteRoundedSmoothVertex(v) &&
              IsOnWorldScaleVariantRoad(recipe,{v.X,v.Y}) &&
              v.Z>=RoundedRoadTopCm &&
              v.Z<RoundedRoadTopCm+1.0,
              "visual ribbon must be backed by original collision pieces");
    }
    for(const auto& tri:visual.Triangles)
    {
        for(const auto index:tri.Indices)
        {
            Check(index<visual.Vertices.size(),
                  "original visual triangle refers to a valid source vertex");
        }
    }

    const auto plan=BuildWorldScaleVariantPace(recipe);
    Check(plan.Valid &&
          plan.Track.Id==recipe.Id &&
          plan.OriginalRaceResultsUnchanged &&
          plan.EstimatedLapSeconds>1.0 &&
          plan.RelaxationPasses>=1u &&
          plan.RelaxationPasses<=WorldScalePaceStations &&
          plan.StepCm>200.0 &&
          plan.StepCm<550.0,
          "each circuit has its own fully bounded 512-node pace model");
    const double brake=2.0*WorldScalePaceBrakeAccelCmps2*plan.StepCm;
    const double drive=2.0*WorldScalePaceDriveAccelCmps2*plan.StepCm;
    double lapTime=0.0;
    for(unsigned i=0;i<WorldScalePaceStations;++i)
    {
        const auto& a=plan.Nodes[i];
        const auto& b=plan.Nodes[(i+1u)%WorldScalePaceStations];
        const double v2=a.TargetCmps*a.TargetCmps;
        const double next2=b.TargetCmps*b.TargetCmps;
        Check(a.TargetCmps>=WorldScalePaceMinSpeedCmps-1e-6 &&
              a.TargetCmps<=WorldScalePaceMaxSpeedCmps+1e-6 &&
              a.TargetCmps<=a.LocalLimitCmps+1e-6 &&
              v2<=next2+brake+0.01 &&
              next2<=v2+drive+0.01 &&
              IsOnWorldScaleVariantRoad(recipe,a.Position),
              "all 512 routes and the closing seam satisfy same physical pace budget");
        lapTime+=2.0*plan.StepCm/(a.TargetCmps+b.TargetCmps);
    }
    Check(std::abs(plan.EstimatedLapSeconds-lapTime)<1e-6,
          "lap seconds come from real source distance and speed not a record");

    for(double distance : {-recipe.LapLengthCm,0.0,
                           recipe.LapLengthCm,
                           3.0*recipe.LapLengthCm})
    {
        const auto sampled=SampleWorldScaleVariantPace(plan,distance);
        Check(sampled.Valid && sampled.WrappedDistanceCm==0.0 &&
              IsOnWorldScaleVariantRoad(recipe,sampled.Pose.Position),
              "signed and repeated lap references share the start/finish geometry");
    }
    for(unsigned i=0;i<WorldScalePaceStations;i+=7u)
    {
        const double station=i*plan.StepCm;
        const auto pose=WorldScaleVariantCenterlineAt(recipe,station);
        const auto projected=ProjectWorldScaleVariantPace(
            plan,pose.Pose.Position);
        Check(pose.Valid && projected.Valid &&
              projected.InsidePhysicalCandidateRoad &&
              projected.CentrelineSeparationCm<=15.0 &&
              DiffOnLap(projected.LapDistanceCm,
                        station,recipe.LapLengthCm)<=20.0 &&
              projected.SuggestedSpeedCmps>
                  WorldScalePaceMinSpeedCmps-1e-6,
              "independent world XY projection cannot mix circuits' lap length");
    }
    Check(!ProjectWorldScaleVariantPace(plan,{0.0,0.0}).Valid,
          "far infield cannot become a drivable racing-line position");
    Check(!SampleWorldScaleVariantPace(
              plan,std::numeric_limits<double>::quiet_NaN()).Valid,
          "nonfinite station cannot claim a lap or speed");
}

void UniqueVariantsAndReferenceCompatibility()
{
    using namespace latebrake;
    std::array<double,WorldScaleVariantCount> lengths{};
    for(unsigned index=0;index<WorldScaleVariantCount;++index)
    {
        const auto recipe=WorldScaleVariantAt(index);
        Check(static_cast<unsigned>(recipe.Id)==index &&
              ValidWorldScaleVariantRecipe(recipe),
              "all six IDs map to distinct original road recipes");
        lengths[index]=recipe.LapLengthCm;
        for(unsigned previous=0;previous<index;++previous)
        {
            Check(std::abs(lengths[index]-lengths[previous])>500.0,
                  "six layouts have physically distinct lap lengths");
        }
        RoadAndPace(recipe);
    }

    const auto original=WorldScaleVariantAt(0);
    Check(std::abs(original.LapLengthCm-
                   WorldScaleCircuitLapCm)<1e-6,
          "first of six source maps exactly preserves existing 1.7km geometry");
    for(double distance : {0.0,250.0,12000.0,
                           0.5*WorldScaleCircuitLapCm,
                           WorldScaleCircuitLapCm-50.0})
    {
        const auto old=WorldScaleCenterlineAt(distance);
        const auto candidate=WorldScaleVariantCenterlineAt(
            original,distance);
        Check(old.Valid && candidate.Valid &&
              std::hypot(old.Pose.Position.X-
                             candidate.Pose.Position.X,
                         old.Pose.Position.Y-
                             candidate.Pose.Position.Y)<1e-6 &&
              std::abs(std::remainder(
                  old.Pose.HeadingDegrees-
                      candidate.Pose.HeadingDegrees,360.0))<1e-8,
              "original selected map matches physically authored historic centerline");
    }

    Check(!ValidWorldScaleVariantIndex(-1) &&
          !ValidWorldScaleVariantIndex(6) &&
          !ValidWorldScaleVariantRecipe(
              WorldScaleVariantByIndex(-1)) &&
          !ValidWorldScaleVariantRecipe(
              WorldScaleVariantByIndex(99)),
          "invalid menu IDs cannot silently fall back to a valid racing level");
}

void RejectMalformedGeometry()
{
    using namespace latebrake;
    auto recipe=WorldScaleVariantAt(0);
    recipe.CornerRadiusCm=30000.0;
    Check(!ValidWorldScaleVariantRecipe(recipe) &&
          !BuildWorldScaleVariantPace(recipe).Valid &&
          !WorldScaleVariantCenterlineAt(recipe,0.0).Valid &&
          !WorldScaleVariantPhysicalRoadPiece(recipe,0).Valid,
          "turn radius cannot exceed actual source extents");
    recipe=WorldScaleVariantAt(0);
    recipe.LapLengthCm+=10000.0;
    Check(!ValidWorldScaleVariantRecipe(recipe) &&
          !BuildWorldScaleVariantPace(recipe).Valid,
          "independent lap length is derived from physical source bounds");
    recipe=WorldScaleVariantAt(0);
    recipe.RoadHalfWidthCm=900.0;
    Check(!ValidWorldScaleVariantRecipe(recipe),
          "no track variant may silently double the 9m road width");
    recipe=WorldScaleVariantAt(0);
    recipe.VisualHalfWidthCm=700.0;
    Check(!ValidWorldScaleVariantRecipe(recipe),
          "non-colliding overlay cannot pretend to be drivable asphalt");
    recipe=WorldScaleVariantAt(0);
    recipe.HalfExtentYcm=
        std::numeric_limits<double>::quiet_NaN();
    Check(!ValidWorldScaleVariantRecipe(recipe) &&
          !WorldScaleVariantCenterlineAt(recipe,0.0).Valid,
          "nonfinite source geometry is invalid before any UE actor is spawned");
    const auto valid=WorldScaleVariantAt(0);
    Check(!WorldScaleVariantCenterlineAt(
              valid,std::numeric_limits<double>::infinity()).Valid &&
          !WorldScaleVariantContinuousRoadProof(valid,10u) &&
          !ProjectWorldScaleVariantPace(
              WorldScaleVariantPacePlan{},{}).Valid,
          "invalid sampling and road verification fail closed");
}
} // namespace

int main()
{
    UniqueVariantsAndReferenceCompatibility();
    RejectMalformedGeometry();
    std::cout<<"WorldScaleOriginalCircuits PASS: six distinct original "
                "rounded physical layouts, 68 source collision pieces each, "
                "closed visible meshes, 512-node speed and signed lap "
                "geometry (source-only, no UE5.8 map cooked)\n";
    return EXIT_SUCCESS;
}
