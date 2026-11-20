#include "Track/WorldScaleMeasuredWetGripPace.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool value,const char* why){
    if(!value){std::cerr<<"WetGripPace FAIL: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double x,double y,double tolerance=1.0e-6){
    return std::abs(x-y)<=tolerance;
}
latebrake::WetGripPaceInput Grip(
    const latebrake::WorldScaleVariantRecipe& circuit,
    double grip,double clock=10.0)
{
    using namespace latebrake;
    WetGripPaceInput out;
    out.Circuit=circuit.Id;
    out.Session=66u;
    out.Revision=3u;
    out.Frame=900u;
    out.ClockSeconds=clock;
    out.All256StationsMeasured=true;
    out.AuthenticRoadBodyVerified=true;
    for(std::size_t i=0u;i<WetGripProbeCount;++i){
        auto& p=out.Probes[i];
        p.StationCm=i*circuit.LapLengthCm/
            static_cast<double>(WetGripProbeCount);
        p.MeasuredGrip=grip;
        p.Confidence=1.0;
        p.ClockSeconds=clock;
        p.RoadBody=i+1u;
        p.NativeTireContactObserved=true;
        p.TrueFrictionMeasurement=true;
    }
    return out;
}
latebrake::ObservedWaterResult Water(
    const latebrake::WorldScaleVariantRecipe& circuit,
    double film=1.5)
{
    using namespace latebrake;
    ObservedWaterResult result;
    result.Valid=true;
    result.Circuit=circuit.Id;
    result.Session=66u;
    result.Revision=3u;
    result.SourceFrame=900u;
    result.ClockSeconds=10.0;
    result.CellLengthCm=circuit.LapLengthCm/
        static_cast<double>(ObservedWaterCellCount);
    result.RenderAdvisoryOnly=true;
    result.OriginalRaceUnaffected=true;
    result.FilmMm.fill(film);
    return result;
}
}
int main(){
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=WetGripPaceSettings{};
    Check(ValidWetGripPaceSettings(settings),
          "source-only real friction defaults valid");
    settings.NeverApplyNativeControls=false;
    Check(!ValidWetGripPaceSettings(settings),
          "wet advice never directly changes real throttle");
    settings={};
    settings.MinimumConfidence=nan;
    Check(!ValidWetGripPaceSettings(settings),
          "nonfinite physical observation confidence fails closed");
    unsigned routes=0u;
    for(unsigned index=0u;index<WorldScaleVariantCount;++index){
        const auto circuit=WorldScaleOriginalVariantRecipes[index];
        WorldScaleBankedRoadProfile road;
        Check(ChooseWorldScaleBankedRoadProfile(
            circuit.Id,road),"original banked source route exists");
        const auto dry=BuildWorldScaleBankedPace(circuit,road);
        Check(dry.Valid,"six source circuits have dry baseline");
        const auto water=Water(circuit);
        const auto measured=Grip(circuit,1.0);
        Check(ValidWetGripPaceSource(
            circuit,dry,water,measured),
            "256 matching actual contact measurements are required");
        const auto reference=BuildMeasuredWetGripPace(
            circuit,dry,water,measured);
        Check(reference.Valid &&
              reference.Fault==WetGripPaceFault::None &&
              reference.Session==66u &&
              reference.Revision==3u &&
              reference.SourceFrame==900u &&
              reference.MinimumGrip==1.0 &&
              reference.MaximumGrip==1.0 &&
              reference.OriginalDryPaceUntouched &&
              reference.NativePhysicsUntouched &&
              reference.RaceAuthorityUntouched &&
              Near(reference.EstimatedLapSeconds,
                   dry.EstimatedGeometricLapSeconds,1e-5),
              "all-grip source cannot fabricate an improved race pace");
        auto wetInput=Grip(circuit,0.65);
        const auto wet=BuildMeasuredWetGripPace(
            circuit,dry,water,wetInput);
        Check(wet.Valid &&
              wet.Fault==WetGripPaceFault::None &&
              wet.MinimumGrip==0.65 &&
              wet.MaximumGrip==0.65 &&
              wet.MaximumObservedFilmMm==1.5 &&
              wet.EstimatedLapSeconds>=
                  dry.EstimatedGeometricLapSeconds-1e-5 &&
              wet.ReducedNodes>0u &&
              wet.RelaxationPasses>0u &&
              wet.RelaxationPasses<=WetGripPaceNodeCount,
              "measured lower grip reduces only advisory speeds");
        for(std::size_t node=0;node<WetGripPaceNodeCount;++node){
            const auto& current=wet.Nodes[node];
            const auto& next=wet.Nodes[
                (node+1u)%WetGripPaceNodeCount];
            const double speed2=current.TargetCmps*current.TargetCmps;
            const double next2=next.TargetCmps*next.TargetCmps;
            const double braking=std::min(
                current.BrakingCmps2,next.BrakingCmps2);
            const double driving=std::min(
                current.DrivingCmps2,next.DrivingCmps2);
            Check(current.TargetCmps<=current.LocalCapCmps+1e-5 &&
                  current.TargetCmps<=
                      dry.Nodes[node].TargetSpeedCmps+1e-5 &&
                  current.NoNativeActuation &&
                  current.Grip==0.65 &&
                  current.ObservedFilmMm==1.5 &&
                  speed2<=next2+
                      2.0*wet.NodeLengthCm*braking+0.1 &&
                  next2<=speed2+
                      2.0*wet.NodeLengthCm*driving+0.1,
                  "every wet circuit edge conservatively brakes through seam");
        }
        for(const double station:{
            0.0,circuit.LapLengthCm*0.25,
            circuit.LapLengthCm-1.0})
        {
            const auto a=SampleMeasuredWetGripPace(
                circuit,wet,station);
            const auto b=SampleMeasuredWetGripPace(
                circuit,wet,station+circuit.LapLengthCm);
            Check(a.Valid && b.Valid &&
                  a.SourceOnly && a.NoRaceResultAuthority &&
                  Near(a.TargetSpeedCmps,b.TargetSpeedCmps) &&
                  Near(a.MinimumMeasuredGrip,
                       b.MinimumMeasuredGrip),
                  "signed whole-lap wet pace sampling preserves seam continuity");
        }
        const auto wrong=WorldScaleOriginalVariantRecipes[
            (index+1u)%WorldScaleVariantCount];
        Check(!SampleMeasuredWetGripPace(
            wrong,wet,0.0).Valid &&
            !SampleMeasuredWetGripPace(
            circuit,wet,nan).Valid,
            "unqualified route and nonfinite pace queries rejected");
        const auto dryWater=Water(circuit,0.0);
        const auto unchanged=BuildMeasuredWetGripPace(
            circuit,dry,dryWater,wetInput);
        Check(unchanged.Valid &&
              Near(unchanged.EstimatedLapSeconds,
                   wet.EstimatedLapSeconds,1e-5),
              "water film is never a fictional friction sensor");
        wetInput=Grip(circuit,0.65);
        wetInput.Probes[50].TrueFrictionMeasurement=false;
        Check(!BuildMeasuredWetGripPace(
            circuit,dry,water,wetInput).Valid,
            "one missing tire-force witness invalidates pace");
        wetInput=Grip(circuit,0.65);
        wetInput.Probes[25].MeasuredGrip=nan;
        Check(!BuildMeasuredWetGripPace(
            circuit,dry,water,wetInput).Valid,
            "nonfinite measured friction fails closed");
        wetInput=Grip(circuit,0.65);
        wetInput.Probes[75].StationCm+=5.0;
        Check(!BuildMeasuredWetGripPace(
            circuit,dry,water,wetInput).Valid,
            "false station mapping cannot qualify native grip");
        wetInput=Grip(circuit,0.65);
        wetInput.Probes[50].ClockSeconds=5.0;
        Check(!BuildMeasuredWetGripPace(
            circuit,dry,water,wetInput).Valid,
            "stale physical grip station cannot be extrapolated");
        wetInput=Grip(circuit,0.65);
        wetInput.Revision=4u;
        const auto staleEpoch=BuildMeasuredWetGripPace(
            circuit,dry,water,wetInput);
        Check(!staleEpoch.Valid &&
              staleEpoch.Fault==WetGripPaceFault::SourceEpochChanged,
              "official session revision separates measured weather");
        ++routes;
    }
    Check(routes==WorldScaleVariantCount,
          "all six distinct road geometries verified");
    std::cout<<"WorldScaleMeasuredWetGripPace: six circuits passed\n";
    return EXIT_SUCCESS;
}
