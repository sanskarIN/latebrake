#include "AI/FleetLineAim.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool value,const char* message)
{
    if(!value){std::cerr<<message<<'\n';std::exit(EXIT_FAILURE);}
}
FleetLineSample At(int gate,int slot,double along,double lateral,double speed=2400.0)
{
    const auto straight=DescribeFleetIncomingStraight(gate);
    return {slot,gate,{FleetLaneWorld(straight,along,lateral),straight.Heading},speed,true};
}
bool Same(CircuitPoint a,CircuitPoint b)
{
    return a.X==b.X && a.Y==b.Y;
}
void PreferredLines()
{
    for(int gate=0;gate<CircuitGateCount;++gate)
    {
        const auto straight=DescribeFleetIncomingStraight(gate);
        for(int slot=1;slot<StartGridCapacity;++slot)
        {
            const double preferred=FleetLanePreference(slot);
            Require(std::abs(preferred)==FleetLanePreferredOffsetCm,"bounded two-lane assignment");
            const auto sample=At(gate,slot,-2400.0,preferred);
            const auto aim=PlanFleetLineAim(sample);
            Require(aim.Status==FleetLineStatus::LaneTarget,"aligned incoming straight uses lane aim");
            Require(aim.TargetLateralCm*preferred>0.0,"original side retained");
            Require(aim.TargetAlongCm>aim.CurrentAlongCm,"target ahead");
            Require(FleetLanePoseFitsSlab({aim.Target,straight.Heading},straight.Slab),
                "padded target fits exact slab");
            const auto near=PlanFleetLineAim(At(gate,slot,-200.0,preferred));
            Require(near.Status==FleetLineStatus::LaneTarget &&
                near.TargetAlongCm==RivalGateRunThroughDistanceCm &&
                std::abs(near.TargetLateralCm)==FleetLaneGateOffsetCm,
                "near-gate target remains through real plane with narrower lane");
            const auto before=FleetLaneWorld(straight,-10.0,near.TargetLateralCm);
            const auto after=FleetLaneWorld(straight,10.0,near.TargetLateralCm);
            Require(CrossesCircuitGatePlane(gate,{before,38.0},{after,38.0},
                straight.Forward.X*1000.0,straight.Forward.Y*1000.0),
                "planned lane lies inside unchanged verified crossing corridor");
        }
        const auto left=PlanFleetLineAim(At(gate,2,-2400.0,-270.0));
        const auto right=PlanFleetLineAim(At(gate,3,-2400.0,270.0));
        Require(left.TargetLateralCm<0.0 && right.TargetLateralCm>0.0,
            "opposite lane targets do not collapse into one");
    }
}
void Fallbacks()
{
    for(int gate=0;gate<CircuitGateCount;++gate)
    {
        const auto normal=At(gate,1,-2000.0,200.0);
        const auto fallback=RivalGateRunThroughPoint(gate);
        auto bad=normal;bad.PhysicalReady=false;
        Require(PlanFleetLineAim(bad).Status==FleetLineStatus::OriginalTarget &&
            Same(PlanFleetLineAim(bad).Target,fallback),"missing physics preserves original target");
        bad=normal;bad.Actual.HeadingDegrees+=31.0;
        Require(Same(PlanFleetLineAim(bad).Target,fallback),"turning body preserves original target");
        bad=At(gate,1,-2000.0,790.0);
        Require(Same(PlanFleetLineAim(bad).Target,fallback),"off-road padded body does not use lane target");
        bad=normal;bad.SpeedCmS=std::numeric_limits<double>::infinity();
        Require(Same(PlanFleetLineAim(bad).Target,fallback),"invalid speed fallback");
        bad=normal;bad.Actual.Position.X=std::numeric_limits<double>::quiet_NaN();
        Require(Same(PlanFleetLineAim(bad).Target,fallback),"invalid position fallback");
        bad=At(gate,1,500.0,0.0);
        Require(Same(PlanFleetLineAim(bad).Target,fallback),"no near-zero forward aim");
        bad=normal;bad.Slot=0;
        Require(PlanFleetLineAim(bad).Status==FleetLineStatus::Invalid,"human not AI lane subject");
        bad=normal;bad.NextGate=4;
        Require(PlanFleetLineAim(bad).Status==FleetLineStatus::Invalid,"invalid route denied");
    }
    Require(!FleetLanePoseFitsSlab({{0.0,0.0},0.0},0),"ground not asphalt");
    Require(!FleetLanePoseFitsSlab({{0.0,2750.0},0.0},5),"barrier not asphalt");
}
void GridStartAndStress()
{
    for(int slot=1;slot<StartGridCapacity;++slot)
    {
        const auto aim=PlanFleetLineAim({slot,0,StartGridPoses[slot],0.0,true});
        Require(aim.Status!=FleetLineStatus::Invalid,"actual grid pose has valid route target");
        if(aim.Status==FleetLineStatus::LaneTarget)
            Require(aim.TargetAlongCm>aim.CurrentAlongCm,"grid target ahead");
    }
    std::mt19937 generator(61302);
    std::uniform_real_distribution<double> along(-6500.0,600.0);
    std::uniform_real_distribution<double> lateral(-800.0,800.0);
    std::uniform_real_distribution<double> yaw(-45.0,45.0);
    std::uniform_real_distribution<double> speed(0.0,10000.0);
    int admitted=0;
    for(int iteration=0;iteration<24000;++iteration)
    {
        const int gate=iteration%4;
        const int slot=iteration%5+1;
        auto s=At(gate,slot,along(generator),lateral(generator),speed(generator));
        s.Actual.HeadingDegrees+=yaw(generator);
        const auto a=PlanFleetLineAim(s);
        const auto straight=DescribeFleetIncomingStraight(gate);
        Require(a.Status!=FleetLineStatus::Invalid,"valid slots and gates keep a route target");
        if(a.Status==FleetLineStatus::OriginalTarget)
        {
            Require(Same(a.Target,RivalGateRunThroughPoint(gate)),"fallback exact original target");
            continue;
        }
        ++admitted;
        Require(a.TargetAlongCm-a.CurrentAlongCm>=FleetLaneMinAheadCm &&
            a.TargetAlongCm<=RivalGateRunThroughDistanceCm &&
            std::abs(a.TargetLateralCm)<=FleetLanePreferredOffsetCm &&
            std::abs(a.TargetLateralCm-a.CurrentLateralCm)<=FleetLaneMaxLateralChangeCm,
            "admitted target bounded in route coordinates");
        Require(FleetLanePoseFitsSlab(s.Actual,straight.Slab) &&
            FleetLanePoseFitsSlab({a.Target,s.Actual.HeadingDegrees},straight.Slab),
            "same slab contains current and translated actual footprints");
        for(int i=0;i<=10;++i)
        {
            const double f=i/10.0;
            const CircuitPoint p{s.Actual.Position.X+(a.Target.X-s.Actual.Position.X)*f,
                s.Actual.Position.Y+(a.Target.Y-s.Actual.Position.Y)*f};
            Require(FleetLanePoseFitsSlab({p,s.Actual.HeadingDegrees},straight.Slab),
                "constant-heading translated footprint stays within convex slab");
        }
    }
    Require(admitted>1000,"stress exercises many real alternate targets");
}
}
int main()
{
    PreferredLines();Fallbacks();GridStartAndStress();
    std::cout<<"Fleet lane planning: four gates, five slots, 24000 sampled poses passed\n";
}
