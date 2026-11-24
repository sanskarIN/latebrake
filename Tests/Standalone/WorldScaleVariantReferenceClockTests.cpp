#include "Track/WorldScaleVariantReferenceClock.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Check(bool ok,const char* message)
{
    if(!ok){std::cerr<<"VariantReferenceClock: "<<message<<'\n';
        std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tol=1e-6)
{
    return std::abs(a-b)<=tol;
}
void RoundTrips()
{
    for(unsigned id=0;id<WorldScaleVariantCount;++id)
    {
        auto plan=BuildWorldScaleVariantPace(WorldScaleVariantAt(id));
        VariantReferenceClock clock;
        Check(clock.Build(plan)&&clock.Ready(),"build immutable reference clock");
        Check(clock.TrackId()==plan.Track.Id&&
            Near(clock.LapSeconds(),plan.EstimatedLapSeconds),"lap identity");
        const double length=clock.LapDistanceCm();
        const double lapTime=clock.LapSeconds();
        for(int lap:{-5,-1,0,1,7})
        for(unsigned i=0;i<WorldScalePaceStations;++i)
        for(double fraction:{0.0,0.25,0.75})
        {
            const double s=lap*length+(i+fraction)*plan.StepCm;
            const auto at=clock.AtStation(s);
            Check(at.Valid&&at.AdvisoryOnly,"signed station sample");
            const auto inverse=clock.AtTime(at.Seconds);
            Check(inverse.Valid&&Near(inverse.StationCm,s)&&
                Near(inverse.SpeedCmps,at.SpeedCmps),"station-time round trip");
            const auto reference=SampleWorldScaleVariantPace(plan,s);
            Check(reference.Valid&&Near(reference.TargetSpeedCmps,at.SpeedCmps),
                "cached speed matches original source sampler");
            const auto direct=EstimateVariantReferenceInterval(plan,s,1.5*plan.StepCm);
            const auto fast=clock.WindowByDistance(s,1.5*plan.StepCm);
            Check(direct.Valid&&fast.Valid&&Near(direct.EstimatedSeconds,fast.DurationSeconds),
                "cached interval matches independent integration");
            const auto reverseWindow=clock.WindowByTime(at.Seconds,fast.DurationSeconds);
            Check(reverseWindow.Valid&&Near(reverseWindow.DistanceCm,fast.DistanceCm),
                "time window inversion matches distance window");
        }
        const auto before=clock.AtStation(-0.5);
        const auto after=clock.AtStation(0.5);
        const auto cross=clock.WindowByDistance(-0.5,1.0);
        Check(before.Valid&&after.Valid&&cross.Valid&&cross.CrossesLapSeam&&
            before.Seconds<0.0&&after.Seconds>0.0,"signed seam continuity");
        const auto full=clock.WindowByTime(-0.5*lapTime,lapTime);
        Check(full.Valid&&Near(full.DistanceCm,length),"full period distance");
        const auto saved=clock.AtStation(100.0);
        plan.Nodes[0].TargetCmps=0.0;
        Check(!clock.Build(plan)&&clock.Ready()&&
            Near(clock.AtStation(100.0).Seconds,saved.Seconds),
            "failed rebuild leaves owned immutable snapshot intact");
        clock.Reset();
        Check(!clock.Ready()&&!clock.AtStation(100.0).Valid&&
            !clock.AtTime(0.0).Valid,"reset invalidates queries");
    }
}
void InvalidInputs()
{
    auto plan=BuildWorldScaleVariantPace(WorldScaleVariantAt(0));
    VariantReferenceClock clock;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    Check(!clock.Build({}),"empty source");
    auto bad=plan;bad.Nodes[50].Position.X+=1.0;
    Check(!clock.Build(bad),"detached source node rejected");
    bad=plan;bad.EstimatedLapSeconds+=1.0;
    Check(!clock.Build(bad),"inconsistent lap time rejected");
    Check(clock.Build(plan),"valid plan");
    for(double invalid:{nan,inf,-inf,1e100,-1e100})
    {
        Check(!clock.AtStation(invalid).Valid&&!clock.AtTime(invalid).Valid,
            "unbounded station or time");
        Check(!clock.WindowByDistance(0.0,invalid).Valid&&
            !clock.WindowByTime(0.0,invalid).Valid,"invalid window");
    }
    for(double invalid:{0.0,-1.0})
        Check(!clock.WindowByDistance(0.0,invalid).Valid&&
            !clock.WindowByTime(0.0,invalid).Valid,"nonpositive window");
    Check(!clock.WindowByDistance(0.0,clock.LapDistanceCm()+1.0).Valid&&
        !clock.WindowByTime(0.0,clock.LapSeconds()+1.0).Valid,"window exceeds lap");
    Check(!clock.WindowByDistance(1e8,1e-14).Valid&&
        !clock.WindowByTime(1000.0,1e-15).Valid,"unresolvable intervals");
}
}
int main()
{
    RoundTrips();InvalidInputs();
    std::cout<<"Six-circuit cached reference clock and signed inversion PASS\n";
    return EXIT_SUCCESS;
}
