#include "AI/FleetHeadwayGovernor.h"

#include <algorithm>
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
        std::cerr<<"FleetHeadwayGovernor: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a,double b,double tolerance=1e-8)
{
    return std::abs(a-b)<=tolerance;
}

struct Scene final
{
    FleetPassSample Sample{};
    FleetHeadwayGovernor Governor;
    PilotCommand Original{0.85,0.21,false,false};

    explicit Scene(int gate=0)
    {
        Sample.GateOrdinal=gate;
        Sample.NextGate=gate;
        Sample.DeltaSeconds=0.05;
        Sample.QualifiedRoad=true;
        Sample.Stamp={71,1,101,1,0};
        Sample.Traffic.SessionQualified=true;
        Sample.Traffic.BrakeObservationQualified=true;
        Sample.Traffic.ConservativeBrakeScale=1.0;
        Sample.Traffic.BlockerCount=MaxRecoveryObstacles;
        Self(-4900,0,1000);
        Other(0,0,-3100,0,1000);
        for(int i=1;i<MaxRecoveryObstacles;++i)
        {
            const auto s=DescribeFleetIncomingStraight((gate+2)%CircuitGateCount);
            Sample.Traffic.Blockers[static_cast<std::size_t>(i)]={
                true,i+1,static_cast<std::uint64_t>(102+i),
                {FleetLaneWorld(s,-600.0-600.0*i,0.0),s.Heading},0,0};
        }
    }

    RecoveryObstacle Body(int slot,double along,double side,double forward,
        double lateral=0.0) const
    {
        const auto s=DescribeFleetIncomingStraight(Sample.NextGate);
        return {true,slot,static_cast<std::uint64_t>(101+slot),
            {FleetLaneWorld(s,along,side),s.Heading},
            s.Forward.X*forward+s.Right.X*lateral,
            s.Forward.Y*forward+s.Right.Y*lateral};
    }

    void Self(double along,double side,double forward,double lateral=0.0)
    {
        Sample.Traffic.Self=Body(1,along,side,forward,lateral);
    }

    void Other(int index,int slot,double along,double side,double forward,
        double lateral=0.0)
    {
        Sample.Traffic.Blockers[static_cast<std::size_t>(index)]=
            Body(slot,along,side,forward,lateral);
    }

    FleetHeadwayReadout Step()
    {
        const auto out=Governor.Apply(Original,Sample);
        ++Sample.Stamp.Frame;
        ++Sample.Stamp.EngineFrame;
        return out;
    }
};

FleetPassLocalBody Local(int slot,double along,double side,double forward,
    double lateral=0.0)
{
    return {slot,static_cast<std::uint64_t>(100+slot),along,side,
        forward,lateral,240,120,0};
}

void TestContinuousLateralPreview()
{
    Require(FleetHeadwayLateralConflict(0,0,100),"same lane");
    Require(FleetHeadwayLateralConflict(300,-400,100),"cut-in within preview");
    Require(FleetHeadwayLateralConflict(-300,400,100),"mirrored cut-in");
    Require(!FleetHeadwayLateralConflict(300,400,100),"moving farther away");
    Require(!FleetHeadwayLateralConflict(300,-100,100),"cut-in too late");
    Require(FleetHeadwayLateralConflict(100,0,100),"inclusive boundary");
    Require(!FleetHeadwayLateralConflict(0,0,-1),"invalid width");
    Require(!FleetHeadwayLateralConflict(
        std::numeric_limits<double>::quiet_NaN(),0,100),"invalid offset");
}

void TestBumperGapAndMatchingSpeed()
{
    const auto self=Local(1,-4900,0,1000);
    const auto tight=EvaluateFleetHeadwayCandidate(self,Local(0,-3700,0,1000),1);
    Require(tight.Valid && Near(tight.GapCm,720) &&
        Near(tight.ProjectedGapCm,720) && Near(tight.DesiredGapCm,980),
        "uses projected chassis extents and desired time gap");
    Require(tight.ThrottleCeiling==0,"matching speed can require coasting");
    const auto wider=EvaluateFleetHeadwayCandidate(self,Local(0,-3100,0,1000),1);
    Require(wider.Valid && wider.ThrottleCeiling>tight.ThrottleCeiling,
        "increased physical gap cannot tighten motor restriction");
    const auto far=EvaluateFleetHeadwayCandidate(self,Local(0,-1000,0,1000),1);
    Require(!far.Valid,"distant car outside pacing range");
    Require(!EvaluateFleetHeadwayCandidate(self,Local(2,-5200,0,2000),1).Valid,
        "fast rear driver owns its own following distance");
    Require(!EvaluateFleetHeadwayCandidate(self,Local(2,-3700,440,1000),1).Valid,
        "separated parallel lane not a fake leader");
    const auto cut=EvaluateFleetHeadwayCandidate(self,Local(2,-3700,440,1000,-400),1);
    Require(cut.Valid && cut.ThrottleCeiling==0,"measured cut-in limits motor");
    const auto movingAway=EvaluateFleetHeadwayCandidate(self,Local(2,-3700,440,1000,400),1);
    Require(!movingAway.Valid,"opposing lateral motion does not invent intersection");
    const auto closing=EvaluateFleetHeadwayCandidate(self,Local(0,-3100,0,600),1);
    Require(closing.Valid && closing.ProjectedGapCm<closing.GapCm &&
        closing.ThrottleCeiling<=wider.ThrottleCeiling,"closing preview reduces target");
    const auto worn=EvaluateFleetHeadwayCandidate(self,Local(0,-3100,0,600),0.25);
    Require(worn.Valid && worn.TargetForwardCmS<=closing.TargetForwardCmS &&
        worn.ThrottleCeiling<=closing.ThrottleCeiling,"worn grip never raises ceiling");
}

void TestWholeRosterAndGeometry()
{
    for(int gate:{0,2})
    {
        Scene scene(gate);
        auto clear=AssessFleetHeadway(scene.Sample);
        Require(clear.Qualified() && clear.Roster[0]==101 && clear.Roster[1]==102,
            "authenticated six-car roster on either long straight");
        scene.Other(0,0,-3700,0,1000);
        auto tight=AssessFleetHeadway(scene.Sample);
        Require(tight.Status==FleetHeadwayStatus::Following && tight.Leader.Valid &&
            tight.Leader.Slot==0 && tight.TargetCeiling<1,
            "equal speed following measured on actual circuit");
        scene.Other(1,2,-3900,0,1000);
        auto strong=AssessFleetHeadway(scene.Sample);
        Require(strong.Qualified() && strong.Leader.Slot==2 &&
            strong.TargetCeiling<=tight.TargetCeiling,"closest equally restrictive leader");
        std::reverse(scene.Sample.Traffic.Blockers.begin(),scene.Sample.Traffic.Blockers.end());
        const auto reordered=AssessFleetHeadway(scene.Sample);
        Require(reordered.Leader.Slot==strong.Leader.Slot &&
            Near(reordered.TargetCeiling,strong.TargetCeiling),"roster order independent");
        scene.Sample.Traffic.Blockers[0].ActorToken=scene.Sample.Traffic.Self.ActorToken;
        Require(!AssessFleetHeadway(scene.Sample).Qualified(),"duplicate identity");
    }
    Scene scene;
    scene.Sample.Traffic.BlockerCount=4;
    Require(!AssessFleetHeadway(scene.Sample).Qualified(),"missing physical body");
    scene.Sample.Traffic.BlockerCount=5;
    scene.Sample.Traffic.BrakeObservationQualified=false;
    Require(!AssessFleetHeadway(scene.Sample).Qualified(),"missing brake capacity");
    scene.Sample.Traffic.BrakeObservationQualified=true;
    scene.Sample.Traffic.ConservativeBrakeScale=1.01;
    Require(!AssessFleetHeadway(scene.Sample).Qualified(),"inflated brake capacity");
    scene.Sample.Traffic.ConservativeBrakeScale=1.0;
    scene.Sample.NextGate=2;
    Require(!AssessFleetHeadway(scene.Sample).Qualified(),"wrong ordered gate");
    scene.Sample.NextGate=0;
    scene.Sample.QualifiedRoad=false;
    Require(!AssessFleetHeadway(scene.Sample).Qualified(),"unqualified road");
    scene.Sample.QualifiedRoad=true;
    scene.Sample.DeltaSeconds=0.11;
    Require(!AssessFleetHeadway(scene.Sample).Qualified(),"oversized frame");
}

void TestRecoveryAndFrameAuthority()
{
    Scene scene;
    scene.Other(0,0,-3700,0,1000);
    const auto tight=scene.Step();
    Require(tight.Restricted && tight.Command.Throttle==0.0 &&
        tight.Command.Steering==scene.Original.Steering &&
        tight.Command.Brake==scene.Original.Brake &&
        tight.Command.Arrived==scene.Original.Arrived,"only motor command reduced");
    scene.Other(0,0,-3100,440,1000);
    const auto released=scene.Step();
    Require(released.Status==FleetHeadwayStatus::Releasing &&
        Near(released.AppliedCeiling,FleetHeadwayReleasePerSecond*scene.Sample.DeltaSeconds),
        "fresh clear frames release at bounded rate");
    --scene.Sample.Stamp.Frame;
    --scene.Sample.Stamp.EngineFrame;
    const auto duplicate=scene.Step();
    Require(duplicate.Status==FleetHeadwayStatus::Interrupted &&
        duplicate.Command.Throttle==0.0,"duplicate frame cannot authorize propulsion");
    const auto resumed=scene.Step();
    Require(resumed.AppliedCeiling<=2*FleetHeadwayReleasePerSecond*scene.Sample.DeltaSeconds+1e-8,
        "duplicate does not contribute release time");
    scene.Governor.Suspend();
    ++scene.Sample.Stamp.SessionEpoch;
    scene.Other(0,0,-3700,0,1000);
    Require(scene.Step().Command.Throttle==0.0,"resumed epoch measures current gap");
    ++scene.Sample.Stamp.TeleportRevision;
    scene.Other(0,0,-3100,440,1000);
    auto after=scene.Step();
    Require(after.Status==FleetHeadwayStatus::Clear &&
        Near(after.Command.Throttle,scene.Original.Throttle),"teleport invalidates old spacing");
    ++scene.Sample.Stamp.Attempt;
    Require(scene.Step().Status==FleetHeadwayStatus::Interrupted,"new attempt requires reset");
    scene.Governor.Reset();
    Require(scene.Step().Status==FleetHeadwayStatus::Clear,"explicit reset");
}

void TestNoAuthorityEscalation()
{
    Scene scene;
    scene.Other(0,0,-3700,0,1000);
    scene.Original={0.0,-0.42,true,false};
    auto out=scene.Step();
    Require(out.Command.Brake && out.Command.Throttle==0.0 &&
        out.Command.Steering==-0.42,"prior braking authoritative");
    scene.Original={0.0,0.0,false,true};
    out=scene.Step();
    Require(out.Command.Arrived && !out.Command.Brake && out.Command.Throttle==0.0,
        "prior arrival preserved");
    scene.Original={0.2,0.5,false,false};
    scene.Sample.QualifiedRoad=false;
    out=scene.Step();
    Require(!out.Assessment.Qualified() && Near(out.Command.Throttle,0.2) &&
        out.Command.Steering==0.5,"unsupported scene stays with existing traffic stack");
    scene.Original.Throttle=std::numeric_limits<double>::infinity();
    out=scene.Step();
    Require(out.Command.Brake && out.Command.Throttle==0.0,"malformed command stopped");
}

void TestReleaseAcrossFrameRates()
{
    for(int fps:{30,60,120})
    {
        Scene scene;
        scene.Sample.DeltaSeconds=1.0/fps;
        scene.Other(0,0,-3700,0,1000);
        Require(scene.Step().AppliedCeiling==0,"initial tight gap");
        scene.Other(0,0,-3100,440,1000);
        double prior=0;
        for(int i=0;i<fps;++i)
        {
            const auto out=scene.Step();
            Require(out.AppliedCeiling>=prior &&
                out.AppliedCeiling<=prior+FleetHeadwayReleasePerSecond/fps+1e-8,
                "bounded release independent of tick rate");
            Require(out.Command.Throttle<=scene.Original.Throttle &&
                out.Command.Steering==scene.Original.Steering,"upstream command cap");
            prior=out.AppliedCeiling;
        }
        Require(Near(prior,1),"release converges in under a second");
    }
}

void TestRandomizedMonotonicityAndSelection()
{
    std::mt19937 rng(78451);
    std::uniform_real_distribution<double> speed(0,2400),gap(500,3300);
    std::uniform_real_distribution<double> side(-650,650),drift(-450,450);
    for(int i=0;i<15000;++i)
    {
        const auto self=Local(1,-4900,0,speed(rng),drift(rng));
        auto other=Local(2,self.Along+gap(rng),side(rng),speed(rng),drift(rng));
        const auto normal=EvaluateFleetHeadwayCandidate(self,other,1.0);
        const auto worn=EvaluateFleetHeadwayCandidate(self,other,0.2);
        Require(normal.Valid==worn.Valid,"grip does not alter physical identity");
        if(!normal.Valid)continue;
        Require(std::isfinite(normal.TargetForwardCmS) && normal.TargetForwardCmS>=0 &&
            normal.ThrottleCeiling>=0 && normal.ThrottleCeiling<=1 &&
            worn.ThrottleCeiling<=normal.ThrottleCeiling+1e-9,
            "finite bounded and grip-monotone outputs");
        auto farther=other;
        farther.Along+=100;
        const auto relaxed=EvaluateFleetHeadwayCandidate(self,farther,1.0);
        Require(!relaxed.Valid || relaxed.ThrottleCeiling+1e-9>=normal.ThrottleCeiling,
            "larger longitudinal gap never worsens restriction");
        auto another=normal;
        another.Slot=3;
        Require(PreferFleetHeadwayCandidate(normal,another) &&
            !PreferFleetHeadwayCandidate(another,normal),"deterministic slot tie");
    }
}

void TestMalformedCandidateFields()
{
    const auto self=Local(1,-4900,0,1000);
    const auto other=Local(0,-3700,0,1000);
    Require(EvaluateFleetHeadwayCandidate(self,other,1).Valid,"baseline");
    const auto nan=std::numeric_limits<double>::quiet_NaN();
    for(int i=0;i<10;++i)
    {
        auto bad=other;
        switch(i)
        {
        case 0:bad.Along=nan;break;
        case 1:bad.Side=nan;break;
        case 2:bad.Forward=-1;break;
        case 3:bad.Lateral=nan;break;
        case 4:bad.HalfAlong=0;break;
        case 5:bad.HalfSide=nan;break;
        case 6:bad.Slot=1;break;
        case 7:bad.Token=self.Token;break;
        case 8:bad.Token=0;break;
        case 9:bad.Forward=FleetTrafficMaxSpeedCmS+1;break;
        }
        Require(!EvaluateFleetHeadwayCandidate(self,bad,1).Valid,"invalid candidate denied");
    }
}

} // namespace

int main()
{
    TestContinuousLateralPreview();
    TestBumperGapAndMatchingSpeed();
    TestWholeRosterAndGeometry();
    TestRecoveryAndFrameAuthority();
    TestNoAuthorityEscalation();
    TestReleaseAcrossFrameRates();
    TestRandomizedMonotonicityAndSelection();
    TestMalformedCandidateFields();
    std::cout<<"Fleet headway pacing tests passed\n";
}
