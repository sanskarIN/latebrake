#include "Track/WorldScaleVariantReferenceTiming.h"
#include <cstdlib>
#include <iostream>
#include <limits>
namespace
{
using namespace latebrake;
void Require(bool ok,const char* reason)
{
    if(!ok){std::cerr<<"Variant reference timing: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b){return std::abs(a-b)<=1e-7;}
void AllCircuits()
{
    for(unsigned id=0;id<WorldScaleVariantCount;++id)
    {
        const auto plan=BuildWorldScaleVariantPace(WorldScaleVariantAt(id));
        const auto timing=BuildVariantReferenceSectorTiming(plan);
        Require(timing.Valid&&timing.AdvisoryOnly&&timing.Track==plan.Track.Id&&
            Near(timing.EstimatedLapSeconds,plan.EstimatedLapSeconds),
            "four sectors reproduce builder lap estimate");
        double sum=0.0;
        for(unsigned i=0;i<VariantReferenceSectorCount;++i)
        {
            const auto& sector=timing.Sectors[i];
            Require(sector.Valid&&!sector.OfficialRaceTime&&sector.EstimatedSeconds>0.0,
                "only bounded advisory timing");
            sum+=sector.EstimatedSeconds;
            Require(Near(sum,timing.CumulativeSeconds[i]),"cumulative sector timing");
            Require(timing.Sectors[timing.LongestSector].EstimatedSeconds>=
                sector.EstimatedSeconds,"longest sector");
        }
        for(unsigned i=0;i<WorldScalePaceStations;++i)
        {
            const double station=(i+0.25)*plan.StepCm;
            const double length=1.25*plan.StepCm;
            const auto whole=EstimateVariantReferenceInterval(plan,station,length);
            const auto a=EstimateVariantReferenceInterval(plan,station,0.5*length);
            const auto b=EstimateVariantReferenceInterval(plan,station+0.5*length,0.5*length);
            const auto prior=EstimateVariantReferenceInterval(plan,
                station-plan.Track.LapLengthCm,length);
            Require(whole.Valid&&a.Valid&&b.Valid&&prior.Valid&&
                Near(whole.EstimatedSeconds,a.EstimatedSeconds+b.EstimatedSeconds)&&
                Near(whole.EstimatedSeconds,prior.EstimatedSeconds),
                "partial segments and signed wrap are additive");
            const auto full=EstimateVariantReferenceInterval(plan,station,plan.Track.LapLengthCm);
            Require(full.Valid&&Near(full.EstimatedSeconds,plan.EstimatedLapSeconds),
                "full lap is independent of start station");
        }
    }
}
void Malformed()
{
    const auto plan=BuildWorldScaleVariantPace(WorldScaleVariantAt(0));
    const auto point=plan.Nodes[0].Position;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    for(double step:{0.0,-1.0,1e-300,1.0,1e300,nan,
        std::numeric_limits<double>::infinity()})
    {
        auto bad=plan;bad.StepCm=step;
        Require(!SampleWorldScaleVariantPace(bad,100.0).Valid&&
            !ProjectWorldScaleVariantPace(bad,point).Valid&&
            !BuildVariantReferenceSectorTiming(bad).Valid,
            "invalid spacing rejected before integer conversion");
    }
    for(double speed:{-3000.0,0.0,1e100,nan})
    {
        auto bad=plan;bad.Nodes[0].TargetCmps=speed;
        Require(!SampleWorldScaleVariantPace(bad,0.0).Valid&&
            !ProjectWorldScaleVariantPace(bad,point).Valid,
            "invalid speed cannot become plausible by squaring");
    }
    auto bad=plan;bad.Nodes[1].DistanceCm+=1.0;
    Require(!SampleWorldScaleVariantPace(bad,0.0).Valid,"misaligned station");
    bad=plan;bad.Nodes[200].Position.X=1e100;
    Require(!ProjectWorldScaleVariantPace(bad,point).Valid,"invalid geometry");
    bad=plan;bad.EstimatedLapSeconds+=1.0;
    Require(!BuildVariantReferenceSectorTiming(bad).Valid,"forged lap estimate");
    bad=plan;bad.OriginalRaceResultsUnchanged=false;
    Require(!SampleWorldScaleVariantPace(bad,0.0).Valid,"authority flag");
    for(double length:{0.0,-1.0,plan.Track.LapLengthCm+1.0,nan})
        Require(!EstimateVariantReferenceInterval(plan,0.0,length).Valid,"invalid interval");
    Require(!EstimateVariantReferenceInterval(plan,1e100,100.0).Valid,"huge station");
    Require(Near(VariantReferenceSegmentSeconds(1000.0,1000.0,1000.0),1.0),
        "constant speed integral");
    Require(Near(VariantReferenceSegmentSeconds(1000.0,1000.0,2000.0),2.0/3.0),
        "variable speed integral");
}
}
int main()
{
    AllCircuits();Malformed();
    std::cout<<"Six-circuit reference timing and invalid-plan regression passed.\n";
    return EXIT_SUCCESS;
}
