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
              reference.SourceOnly==false,
              "placeholder");
        ++routes;
    }
    Check(routes==WorldScaleVariantCount,
          "all six distinct road geometries verified");
    std::cout<<"WorldScaleMeasuredWetGripPace: six circuits passed\n";
    return EXIT_SUCCESS;
}
