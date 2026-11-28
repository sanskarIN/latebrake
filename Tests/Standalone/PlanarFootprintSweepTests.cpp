#include "Track/PlanarFootprintSweep.h"
#include "AI/RivalRecoveryOccupancy.h"
#include "AI/RivalFleetCollisionForecast.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool value,const char* reason)
{
    if(!value)
    {
        std::cerr<<"PlanarFootprintSweep FAIL: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double eps=1.0e-8)
{
    return std::abs(a-b)<=eps;
}

void TestAxisIntervals()
{
    auto axis=SolveFootprintAxisWindow(10,-20,2,1);
    Require(axis.Valid && axis.Intersects && !axis.StartsOverlapping &&
        Near(axis.EntrySeconds,0.4) && Near(axis.ExitSeconds,0.6),
        "analytic negative velocity interval");
    axis=SolveFootprintAxisWindow(-10,20,2,1);
    Require(axis.Valid && axis.Intersects && Near(axis.EntrySeconds,0.4) &&
        Near(axis.ExitSeconds,0.6),"analytic positive velocity interval");
    axis=SolveFootprintAxisWindow(0,10,2,1);
    Require(axis.Valid && axis.Intersects && axis.StartsOverlapping &&
        axis.EntrySeconds==0 && Near(axis.ExitSeconds,0.2),"starts within support");
    axis=SolveFootprintAxisWindow(10,-8,2,1);
    Require(axis.Valid && axis.Intersects && axis.EntrySeconds==1 &&
        axis.ExitSeconds==1,"exact horizon contact included");
    axis=SolveFootprintAxisWindow(2,1,2,1);
    Require(axis.Valid && axis.Intersects && axis.StartsOverlapping &&
        axis.ExitSeconds==0,"exact time-zero edge moving away");
    axis=SolveFootprintAxisWindow(3,0,2,1);
    Require(axis.Valid && !axis.Intersects,"stationary separation");
    axis=SolveFootprintAxisWindow(1,0,2,1);
    Require(axis.Valid && axis.Intersects && axis.StartsOverlapping &&
        axis.ExitSeconds==1,"stationary overlap");
    axis=SolveFootprintAxisWindow(0,100,2,0);
    Require(axis.Valid && axis.Intersects && axis.EntrySeconds==0 &&
        axis.ExitSeconds==0,"zero horizon");
    axis=SolveFootprintAxisWindow(3,-100,2,0);
    Require(axis.Valid && !axis.Intersects,"zero horizon cannot predict motion");
    axis=SolveFootprintAxisWindow(3,-1e-200,2,1);
    Require(axis.Valid && !axis.Intersects,"tiny motion cannot generate huge times");
    axis=SolveFootprintAxisWindow(0,1e-200,2,1);
    Require(axis.Valid && axis.Intersects && Near(axis.ExitSeconds,1),"tiny motion within");
    Require(!SolveFootprintAxisWindow(1,0,-1,1).Valid,"negative support");
    Require(!SolveFootprintAxisWindow(1,0,1,-1).Valid,"negative horizon");
    Require(!SolveFootprintAxisWindow(1,0,1,3).Valid,"unbounded horizon");
    Require(!SolveFootprintAxisWindow(std::numeric_limits<double>::quiet_NaN(),0,1,1).Valid,
        "nonfinite separation");
}

void TestFootprints()
{
    const double length=2.0*GridPaddedHalfLengthCm;
    const double width=2.0*GridPaddedHalfWidthCm;
    PlanarFootprintSweepRequest req{{{0,0},0},{{length+100,0},0},-200,0,1,0};
    auto r=SweepPlanarFootprints(req);
    Require(r.Valid && r.Intersects && !r.StartsOverlapping && Near(r.EntrySeconds,0.5)
        && r.ExitSeconds==1,"parallel approach at known gap");
    req.Other.Position.Y=width+1;
    r=SweepPlanarFootprints(req);
    Require(r.Valid && !r.Intersects,"adjacent lane separated for whole horizon");
    req.RelativeVelocityY=-2;
    r=SweepPlanarFootprints(req);
    Require(r.Valid && r.Intersects && Near(r.EntrySeconds,0.5),"both axes enter simultaneously");
    req.Other.Position={length+100,0};
    req.RelativeVelocityY=0;
    req.Padding=50;
    r=SweepPlanarFootprints(req);
    Require(r.Valid && r.Intersects && Near(r.EntrySeconds,0.25),"prediction padding");
    req.Other.Position={length+25,0};
    r=SweepPlanarFootprints(req);
    Require(r.Valid && r.Intersects && r.StartsOverlapping,"padded current contact");
    req.Padding=0;
    r=SweepPlanarFootprints(req);
    Require(r.Valid && r.Intersects && !r.StartsOverlapping && Near(r.EntrySeconds,0.125),
        "physical and uncertainty support are distinct");

    req={{{0,0},0},{{0,width+100},0},0,-200,1,0};
    r=SweepPlanarFootprints(req);
    Require(r.Valid && r.Intersects && Near(r.EntrySeconds,0.5),"lateral crossing");
    req.Other.HeadingDegrees=90;
    req.Other.Position.Y=GridPaddedHalfWidthCm+GridPaddedHalfLengthCm+100;
    r=SweepPlanarFootprints(req);
    Require(r.Valid && r.Intersects && Near(r.EntrySeconds,0.5),"perpendicular body projection");

    // A trajectory entering one axis after leaving the other cannot hit.
    req={{{0,0},0},{{length+100,width+100},0},-200,-20,1,0};
    r=SweepPlanarFootprints(req);
    Require(r.Valid && !r.Intersects,"nonoverlapping SAT time windows");
    req={{{0,0},0},{{-length,0},0},-100,0,0.4,0};
    r=SweepPlanarFootprints(req);
    Require(r.Valid && r.Intersects && r.StartsOverlapping && r.ExitSeconds==0,
        "rear edge touching");

    req.RelativeVelocityX=30001;
    Require(!SweepPlanarFootprints(req).Valid,"excessive relative velocity");
    req.RelativeVelocityX=0;
    req.Horizon=2.01;
    Require(!SweepPlanarFootprints(req).Valid,"excessive horizon");
    req.Horizon=0.4;
    req.Padding=-1;
    Require(!SweepPlanarFootprints(req).Valid,"negative padding");
    req.Padding=0;
    req.Other.HeadingDegrees=std::numeric_limits<double>::infinity();
    Require(!SweepPlanarFootprints(req).Valid,"malformed heading");
}

void TestRelativeSpeedAndWrapperCompatibility()
{
    const CircuitPose a{{0,0},0};
    const CircuitPose distant{{5000,600},0};
    auto r=SweepPlanarFootprints({a,distant,-30000,0,0.4,0});
    Require(r.Valid && !r.Intersects,"two valid opposing velocities do not fake a far-lane hazard");
    const CircuitPose approaching{{5000,0},180};
    r=SweepPlanarFootprints({a,approaching,-30000,0,0.4,0});
    Require(r.Valid && r.Intersects && Near(r.EntrySeconds,
        (5000.0-2.0*GridPaddedHalfLengthCm)/30000.0),"high relative speed contact time");
    Require(PredictedRecoveryFootprintsOverlap(a,distant,30000,0),
        "stationary recovery wrapper retains its physical speed bound");
    Require(!PredictedRecoveryFootprintsOverlap(a,distant,-15000,0),
        "stationary recovery observes legal distant blocker");
    const RecoveryObstacle self{true,1,10,a,15000,0};
    const RecoveryObstacle other{true,0,11,approaching,-15000,0};
    const auto longForecast=ForecastFleetContact(self,other);
    const auto expected=SweepPlanarFootprints({a,approaching,-30000,0,
        FleetForecastHorizonSeconds,FleetForecastContactPaddingCm});
    Require(longForecast.Intersects && expected.Valid &&
        Near(longForecast.FirstContactSeconds,expected.EntrySeconds),
        "predictive and immediate forecasts share exact interval kernel");
    auto bad=other;
    bad.VelocityX=std::numeric_limits<double>::quiet_NaN();
    Require(ForecastFleetContact(self,bad).Intersects,"invalid predictive input fails closed");
}

RivalFleetTrafficSample Traffic()
{
    RivalFleetTrafficSample sample;
    sample.SessionQualified=true;
    sample.BlockerCount=5;
    sample.Self={true,1,101,{{0,-1800},0},1000,0};
    for(int i=0;i<5;++i)
    {
        const int slot=i==0?0:i+1;
        sample.Blockers[static_cast<std::size_t>(i)]={true,slot,
            static_cast<std::uint64_t>(100+slot),{{5000,3000.0+i*300.0},0},0,0};
    }
    return sample;
}

void CheckPermutations(RivalFleetTrafficSample sample,int slot,FleetImmediateThreat threat,
    bool brake,double throttle,double contact)
{
    std::array<int,5> order{{0,1,2,3,4}};
    const auto blockers=sample.Blockers;
    int permutations=0;
    do
    {
        for(std::size_t i=0;i<order.size();++i)
            sample.Blockers[i]=blockers[static_cast<std::size_t>(order[i])];
        const auto out=ApplyRivalFleetTraffic({1,0.3,false,false},sample);
        Require(out.Qualified && out.LimitingSlot==slot && out.Threat==threat &&
            out.EmergencyBrake==brake && out.Command.Brake==brake &&
            Near(out.Command.Throttle,throttle) && out.Command.Steering==0.3,
            "all 120 blocker permutations preserve exact command and attribution");
        if(std::isfinite(contact))Require(Near(out.ContactSeconds,contact),"exact limiting contact");
        else Require(std::isinf(out.ContactSeconds),"no invented contact time");
        ++permutations;
    }
    while(std::next_permutation(order.begin(),order.end()));
    Require(permutations==120,"every complete roster ordering checked");
}

void TestThreatSelection()
{
    auto sample=Traffic();
    const double length=2.0*GridPaddedHalfLengthCm;
    sample.Blockers[0].Pose.Position={length+300,-1800};
    sample.Blockers[4].Pose.Position={length+100,-1800};
    CheckPermutations(sample,5,FleetImmediateThreat::SweptContact,true,0,0.1);
    sample.Blockers[3].Pose.Position={length+100,-1800};
    CheckPermutations(sample,4,FleetImmediateThreat::SweptContact,true,0,0.1);
    sample.Blockers[0].Pose.Position={-length,-1800};
    CheckPermutations(sample,0,FleetImmediateThreat::Touching,true,0,0);

    sample=Traffic();
    sample.Blockers[0].Pose.Position={1450,-1800};
    sample.Blockers[1].Pose.Position={1300,-1800};
    sample.Blockers[2].Pose.Position={1300,-1800};
    const double cap=0.15+0.65*(1300.0-450.0)/(1550.0-450.0);
    CheckPermutations(sample,2,FleetImmediateThreat::Following,false,cap,FootprintSweepNoContact);

    sample=Traffic();
    sample.Self.VelocityX=15000;
    sample.Blockers[0].Pose.Position={5000,-1200};
    sample.Blockers[0].VelocityX=-15000;
    CheckPermutations(sample,-1,FleetImmediateThreat::None,false,1,FootprintSweepNoContact);
    sample.Blockers[0].Pose.Position={5000,-1800};
    CheckPermutations(sample,0,FleetImmediateThreat::SweptContact,true,0,
        (5000.0-length)/30000.0);

    sample=Traffic();
    sample.Blockers[0].Pose.Position={-800,-1800};
    sample.Blockers[0].VelocityX=9000;
    CheckPermutations(sample,-1,FleetImmediateThreat::None,false,1,FootprintSweepNoContact);

    sample.Blockers[4].ActorToken=sample.Blockers[0].ActorToken;
    auto invalid=ApplyRivalFleetTraffic({1,0.3,false,false},sample);
    Require(!invalid.Qualified && invalid.Command.Brake,"duplicate roster fails closed");
    sample=Traffic();
    const auto brake=ApplyRivalFleetTraffic({0,-0.3,true,false},sample);
    Require(brake.Qualified && brake.Command.Brake && brake.Command.Steering==-0.3 &&
        brake.Threat==FleetImmediateThreat::None,"existing brake is not reclassified");
}

void TestSymmetryAndContinuity()
{
    std::mt19937_64 generator(6112026u);
    std::uniform_real_distribution<double> position(-2000,2000);
    std::uniform_real_distribution<double> heading(-180,180);
    std::uniform_real_distribution<double> velocity(-10000,10000);
    for(int i=0;i<3000;++i)
    {
        const CircuitPose a{{position(generator),position(generator)},heading(generator)};
        const CircuitPose b{{position(generator),position(generator)},heading(generator)};
        const double vx=velocity(generator);
        const double vy=velocity(generator);
        const auto forward=SweepPlanarFootprints({a,b,vx,vy,0.4,0});
        const auto reverse=SweepPlanarFootprints({b,a,-vx,-vy,0.4,0});
        Require(forward.Valid && reverse.Valid &&
            forward.Intersects==reverse.Intersects &&
            forward.StartsOverlapping==reverse.StartsOverlapping,
            "exchange symmetry of moving bodies");
        Require(forward.StartsOverlapping==StartGridFootprintsOverlap(a,b),
            "time-zero equality with actual padded footprint test");
        if(!forward.Intersects)continue;
        Require(Near(forward.EntrySeconds,reverse.EntrySeconds) &&
            Near(forward.ExitSeconds,reverse.ExitSeconds),"exchange symmetry of time window");
        Require(forward.EntrySeconds>=0 && forward.EntrySeconds<=forward.ExitSeconds &&
            forward.ExitSeconds<=0.4,"bounded ordered collision interval");
        const double middle=(forward.EntrySeconds+forward.ExitSeconds)*0.5;
        CircuitPose moved=b;
        moved.Position.X+=vx*middle;
        moved.Position.Y+=vy*middle;
        // Skip the isolated tangent case for floating-point reconstruction
        // at a mathematically exact boundary.
        if(forward.ExitSeconds-forward.EntrySeconds>1.0e-7)
            Require(StartGridFootprintsOverlap(a,moved),"sample inside exact SAT interval");
        if(forward.EntrySeconds>1.0e-5)
        {
            moved=b;
            const double before=forward.EntrySeconds-1.0e-5;
            moved.Position.X+=vx*before;
            moved.Position.Y+=vy*before;
            Require(!StartGridFootprintsOverlap(a,moved),"no overlap before entry");
        }
    }
}
}

int main()
{
    TestAxisIntervals();
    TestFootprints();
    TestRelativeSpeedAndWrapperCompatibility();
    TestThreatSelection();
    TestSymmetryAndContinuity();
    std::cout<<"Shared planar sweep and deterministic fleet traffic tests passed.\n";
    return EXIT_SUCCESS;
}
