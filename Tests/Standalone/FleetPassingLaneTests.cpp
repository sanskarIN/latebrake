#include "AI/FleetPassingLane.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Require(bool ok,const char* why)
{
    if (!ok)
    {
        std::cerr<<"FleetPassingLane: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Fixture final
{
    FleetPassSample Sample;
    PilotCommand Original{0.7,0.12,false,false};
    FleetPassingLane Passing;
    Fixture(int gate=0)
    {
        Sample.GateOrdinal=gate;
        Sample.NextGate=gate;
        Sample.DeltaSeconds=0.05;
        Sample.QualifiedRoad=true;
        Sample.Stamp={1,1,101,1,0};
        Sample.Traffic.SessionQualified=true;
        Sample.Traffic.BrakeObservationQualified=true;
        Sample.Traffic.ConservativeBrakeScale=1.0;
        Sample.Traffic.BlockerCount=MaxRecoveryObstacles;
        SetSelf(-4900,0,1000);
        SetOther(0,0,-3100,0,700);
        for (int i=1;i<5;++i)
        {
            const auto s=DescribeFleetIncomingStraight((gate+2)%4);
            Sample.Traffic.Blockers[static_cast<std::size_t>(i)]={
                true,i+1,static_cast<std::uint64_t>(102+i),
                {FleetLaneWorld(s,-600.0-600.0*i,0.0),s.Heading},0,0};
        }
    }

    RecoveryObstacle Body(int slot,double along,double side,double forward,double lateral=0.0) const
    {
        const auto s=DescribeFleetIncomingStraight(Sample.NextGate);
        return {true,slot,static_cast<std::uint64_t>(101+slot),
            {FleetLaneWorld(s,along,side),s.Heading},
            s.Forward.X*forward+s.Right.X*lateral,
            s.Forward.Y*forward+s.Right.Y*lateral};
    }

    void SetSelf(double along,double side,double forward,double lateral=0.0)
    {
        Sample.Traffic.Self=Body(1,along,side,forward,lateral);
    }

    void SetOther(int index,int slot,double along,double side,double forward,double lateral=0.0)
    {
        Sample.Traffic.Blockers[static_cast<std::size_t>(index)]=
            Body(slot,along,side,forward,lateral);
    }

    FleetPassReadout Step()
    {
        const auto out=Passing.Apply(Original,Sample);
        ++Sample.Stamp.Frame;
        ++Sample.Stamp.EngineFrame;
        return out;
    }

    FleetPassReadout Confirm()
    {
        FleetPassReadout r;
        for (int i=0;i<4;++i) r=Step();
        return r;
    }
};

void SweptIntervals()
{
    auto hit=FleetPassSweptBox(1000,0,-1000,0,100,100,2);
    Require(hit.Intersects && std::abs(hit.FirstSeconds-0.9)<1e-9,"front crossing");
    hit=FleetPassSweptBox(-1000,0,1000,0,100,100,2);
    Require(hit.Intersects && std::abs(hit.FirstSeconds-0.9)<1e-9,"rear crossing");
    hit=FleetPassSweptBox(0,400,0,-800,100,100,1);
    Require(hit.Intersects && std::abs(hit.FirstSeconds-0.375)<1e-9,"side crossing between endpoints");
    Require(!FleetPassSweptBox(1000,400,-1000,0,100,100,2).Intersects,"separate lateral axes");
    Require(!FleetPassSweptBox(1000,400,-1000,-1000,100,100,2).Intersects,"disjoint time windows");
    Require(FleetPassSweptBox(100,100,0,0,100,100,1).Intersects,"inclusive boundary");
    Require(FleetPassSweptBox(0,0,0,0,-1,100,1).Intersects,"invalid extent refuses clearance");
    Require(FleetPassSweptBox(0,0,0,0,100,100,
        std::numeric_limits<double>::quiet_NaN()).Intersects,"invalid horizon");
}

void QualificationOnLongStraights()
{
    for (int gate : {0,2})
    {
        Fixture f(gate);
        const auto g=InspectFleetPassGeometry(f.Sample);
        Require(g.Valid && FleetPassStraightQualified(g,f.Sample.Traffic.Self,true),"long straight");
        const auto* leader=FindFleetPassLeader(g);
        Require(leader && leader->Slot==0,"actual slower player selected");
        const auto choice=SelectFleetPassLane(g,f.Sample.Traffic.Self,*leader);
        Require(choice.Valid && choice.Clear && std::abs(choice.LaneCm)==FleetPassLaneCm,"bounded lane");
        auto r=f.Step();
        Require(r.Phase==FleetPassPhase::Confirming && !r.SteeringChanged &&
            r.Command.Steering==f.Original.Steering,"no first-frame steer");
        r=f.Step();
        Require(r.Phase==FleetPassPhase::Confirming,"second frame confirming");
        r=f.Step();
        Require(r.SteeringChanged && r.Phase==FleetPassPhase::Shifting &&
            r.LeaderToken==101 && r.Command.Throttle==f.Original.Throttle &&
            !r.Command.Brake && !r.Command.Arrived,"confirmed changes steering only");
        Require(std::abs(r.Command.Steering)<=FleetPassMaxSteer,"steering cap");
        f.SetSelf(-4700,r.LaneCm,1000);
        r=f.Step();
        Require(r.Phase==FleetPassPhase::Holding && r.SteeringChanged,"stable target lane");
    }
    Fixture shortStraight(1);
    shortStraight.SetSelf(-3000,0,1000);
    shortStraight.SetOther(0,0,-1500,0,700);
    Require(!shortStraight.Confirm().SteeringChanged,"short straight denied");
}

void RealFleetBlockers()
{
    Fixture f;
    auto g=InspectFleetPassGeometry(f.Sample);
    const auto* lead=FindFleetPassLeader(g);
    Require(lead!=nullptr,"initial leader");
    const auto first=SelectFleetPassLane(g,f.Sample.Traffic.Self,*lead);
    f.SetOther(1,2,-4700,first.LaneCm,1000);
    g=InspectFleetPassGeometry(f.Sample);
    Require(g.Valid,"side blocker roster");
    const auto* next=FindFleetPassLeader(g);
    Require(next!=nullptr,"leader remains");
    const auto other=SelectFleetPassLane(g,f.Sample.Traffic.Self,*next);
    Require(other.Clear && other.LaneCm==-first.LaneCm,"occupied side vetoed");
    f.SetOther(2,3,-4700,-first.LaneCm,1000);
    g=InspectFleetPassGeometry(f.Sample);
    Require(!SelectFleetPassLane(g,f.Sample.Traffic.Self,*FindFleetPassLeader(g)).Clear,"both sides blocked");
    Require(f.Confirm().Reason==FleetPassReason::NoClearLane,"no forced lane");

    Fixture rear;
    rear.SetSelf(-4400,0,1000);
    rear.SetOther(0,0,-2600,0,700);
    rear.SetOther(1,2,-5250,440,1800);
    g=InspectFleetPassGeometry(rear.Sample);
    Require(g.Valid,"rear fixture");
    const auto evidence=InspectFleetPassLane(g,rear.Sample.Traffic.Self,440);
    Require(evidence.Valid && !evidence.Clear && evidence.BlockerSlot==2,"closing rear rival");
}

void LeaderAndRosterValidation()
{
    Fixture f;
    f.SetOther(1,2,-3250,0,700);
    auto g=InspectFleetPassGeometry(f.Sample);
    Require(g.Valid && FindFleetPassLeader(g) && FindFleetPassLeader(g)->Slot==2,"nearer AI selected");
    std::reverse(f.Sample.Traffic.Blockers.begin(),f.Sample.Traffic.Blockers.end());
    g=InspectFleetPassGeometry(f.Sample);
    Require(g.Valid && FindFleetPassLeader(g) && FindFleetPassLeader(g)->Slot==2,"order independent");
    f.Sample.Traffic.Blockers[0].ActorToken=f.Sample.Traffic.Self.ActorToken;
    Require(!InspectFleetPassGeometry(f.Sample).Valid && !f.Confirm().SteeringChanged,"duplicate actor");
    Fixture absent;
    absent.Sample.Traffic.BlockerCount=4;
    Require(!absent.Confirm().SteeringChanged,"missing fifth body");
    Fixture stale;
    stale.Sample.NextGate=2;
    Require(!stale.Confirm().SteeringChanged,"mismatched ordinal");
    Fixture invalid;
    invalid.Sample.Traffic.Blockers[4].VelocityX=std::numeric_limits<double>::infinity();
    Require(!invalid.Confirm().SteeringChanged,"malformed distant rival");
    Fixture offroad;
    offroad.Sample.QualifiedRoad=false;
    Require(!offroad.Confirm().SteeringChanged,"missing road evidence");
}

void AbortAndRearm()
{
    Fixture f;
    auto r=f.Confirm();
    Require(r.SteeringChanged,"active baseline");
    const double lane=r.LaneCm;
    const auto saved=f.Sample.Traffic.Blockers[1];
    f.SetOther(1,2,-4600,lane,1000);
    r=f.Step();
    Require(r.Phase==FleetPassPhase::Retired &&
        r.Reason==FleetPassReason::CorridorOccupied && !r.SteeringChanged &&
        r.Command.Steering==f.Original.Steering,"intrusion restores steer");
    f.Sample.Traffic.Blockers[1]=saved;
    Require(f.Confirm().Phase==FleetPassPhase::Retired,"same gate no retry");
    f.Passing.Suspend();
    Require(f.Confirm().Phase==FleetPassPhase::Retired,"pause cannot rearm");
    f.Sample.GateOrdinal=4;
    Require(f.Confirm().SteeringChanged,"actual next lap ordinal re-arms");
    f.Passing.AbortTraffic();
    Require(f.Step().Phase==FleetPassPhase::Retired,"traffic brake latches retirement");
    f.Passing.Reset();
    Require(f.Confirm().SteeringChanged,"explicit reset");
    f.Original.Brake=true;
    r=f.Step();
    Require(r.Reason==FleetPassReason::PriorBrake && !r.SteeringChanged &&
        r.Command.Brake,"prior brake preserved");
}

void TimingAndContinuity()
{
    Fixture f;
    f.Step();
    f.Sample.DeltaSeconds=0.11;
    Require(!f.Step().SteeringChanged,"hitch rejects");
    f.Sample.DeltaSeconds=0.05;
    Require(f.Confirm().Phase==FleetPassPhase::Retired,"no hitch credit");
    Fixture second;
    Require(second.Confirm().SteeringChanged,"second baseline");
    const auto token=second.Sample.Traffic.Blockers[4].ActorToken;
    second.Sample.Traffic.Blockers[4].ActorToken=999;
    Require(!second.Step().SteeringChanged,"changed roster");
    second.Sample.Traffic.Blockers[4].ActorToken=token;
    Require(second.Confirm().Phase==FleetPassPhase::Retired,"old roster no retry");
    Fixture third;
    auto r=third.Confirm();
    Require(r.SteeringChanged,"timeout baseline");
    for (int i=0;i<100;++i) r=third.Step();
    Require(r.Phase==FleetPassPhase::Retired && r.Reason==FleetPassReason::Timeout,"bounded duration");
    Fixture fourth;
    r=fourth.Confirm();
    fourth.SetSelf(-2100,r.LaneCm,1000);
    fourth.SetOther(0,0,-2800,0,700);
    r=fourth.Step();
    Require(!r.SteeringChanged && r.Phase==FleetPassPhase::Retired,"passed leader retires");
    Fixture fifth;
    fifth.Original.Throttle=std::numeric_limits<double>::quiet_NaN();
    r=fifth.Step();
    Require(r.Command.Brake && r.Command.Throttle==0.0,"malformed command stopped");
}

void FrameAuthorityAndDriftEnvelope()
{
    const FleetPassStamp stamp{1,10,50,3,0};
    auto next=stamp;
    ++next.Frame;++next.EngineFrame;
    Require(FleetPassContinuous(stamp,next),"exact consecutive frame");
    ++next.TeleportRevision;
    Require(FleetPassFresh(stamp,next) && !FleetPassContinuous(stamp,next),
        "teleport breaks physical continuity");
    next=stamp;
    ++next.Frame;
    Require(!FleetPassFresh(stamp,next),"same engine frame is not fresh");
    next=stamp;
    ++next.Attempt;++next.Frame;++next.EngineFrame;
    Require(!FleetPassFresh(stamp,next),"foreign attempt requires reset");

    next=stamp;++next.Frame;++next.EngineFrame;--next.SessionEpoch;
    Require(!FleetPassFresh(stamp,next),"session rollback is not fresh");
    auto recoveredStamp=stamp;recoveredStamp.TeleportRevision=3;
    next=recoveredStamp;++next.Frame;++next.EngineFrame;--next.TeleportRevision;
    Require(!FleetPassFresh(recoveredStamp,next),"teleport rollback is not fresh");

    Fixture duplicate;
    Require(duplicate.Step().Phase==FleetPassPhase::Confirming,"confirmation begins");
    --duplicate.Sample.Stamp.Frame;--duplicate.Sample.Stamp.EngineFrame;
    Require(!duplicate.Step().SteeringChanged,"duplicate cannot advance confirmation");
    Require(duplicate.Confirm().Phase==FleetPassPhase::Retired,"duplicate retires manoeuvre");

    Fixture epoch;
    Require(epoch.Confirm().SteeringChanged,"active before pause");
    ++epoch.Sample.Stamp.SessionEpoch;
    Require(epoch.Step().Phase==FleetPassPhase::Retired &&
        !epoch.Step().SteeringChanged,"changed epoch clears active pass");

    Fixture recovery;
    Require(recovery.Confirm().SteeringChanged,"active before recovery");
    ++recovery.Sample.Stamp.TeleportRevision;
    Require(!recovery.Step().SteeringChanged,"teleported body cannot borrow confirmation");

    Fixture rollback;
    rollback.Sample.Stamp.TeleportRevision=5;
    Require(rollback.Confirm().SteeringChanged,"active before stale recovery");
    rollback.Sample.GateOrdinal=4;
    rollback.Sample.Stamp.TeleportRevision=4;
    Require(!rollback.Step().SteeringChanged,"old revision cannot rearm at a new gate");
    rollback.Sample.Stamp.TeleportRevision=5;
    Require(rollback.Confirm().SteeringChanged,"current authenticated new gate can rearm");

    Fixture gap;
    Require(gap.Step().Phase==FleetPassPhase::Confirming,"gap fixture");
    ++gap.Sample.Stamp.Frame;
    Require(!gap.Step().SteeringChanged &&
        gap.Confirm().Phase==FleetPassPhase::Retired,"skipped observation breaks confirmation");

    Fixture foreign;
    Require(foreign.Confirm().SteeringChanged,"foreign attempt fixture");
    ++foreign.Sample.Stamp.Attempt;
    Require(!foreign.Step().SteeringChanged,"foreign attempt rejected");
    foreign.Passing.Reset();
    Require(foreign.Confirm().SteeringChanged,"explicit new attempt reset permits fresh observation");

    Fixture drift;
    drift.SetSelf(-4900,0,1000,-200);
    drift.SetOther(1,2,-4700,-440,1000);
    auto geometry=InspectFleetPassGeometry(drift.Sample);
    Require(geometry.Valid,"drift geometry");
    const auto evidence=InspectFleetPassLane(geometry,drift.Sample.Traffic.Self,440);
    Require(evidence.Valid && !evidence.Clear && evidence.BlockerSlot==2,
        "opposing measured lateral drift reserves the other side too");

    Fixture edge;
    edge.SetSelf(-4900,300,1000,350);
    geometry=InspectFleetPassGeometry(edge.Sample);
    Require(geometry.Valid,"current body remains paved");
    Require(!InspectFleetPassLane(geometry,edge.Sample.Traffic.Self,440).Valid,
        "drift beyond authored slab refuses lane proposal");
}

void RandomizedSweptWindowAndCommands()
{
    std::mt19937 random(16943);
    std::uniform_real_distribution<double> along(-3000,3000);
    std::uniform_real_distribution<double> side(-1200,1200);
    std::uniform_real_distribution<double> speed(-2200,2200);
    for (int i=0;i<16000;++i)
    {
        const double x=along(random), y=side(random),vx=speed(random),vy=speed(random);
        const auto hit=FleetPassSweptBox(x,y,vx,vy,500,300,1.8);
        bool discrete=false;
        for (int n=0;n<=180;++n)
        {
            const double t=n*0.01;
            if (std::abs(x+vx*t)<=500 && std::abs(y+vy*t)<=300) discrete=true;
        }
        Require(!discrete || hit.Intersects,"continuous interval includes all sampled hits");
        if (hit.Intersects)
            Require(hit.FirstSeconds>=0 && hit.FirstSeconds<=1.8,"bounded time");
    }
    std::uniform_real_distribution<double> lane(-470,470);
    std::uniform_real_distribution<double> forward(850,2400);
    for (int i=0;i<5000;++i)
    {
        Fixture f(i%2==0?0:2);
        f.SetSelf(-4800,lane(random),forward(random));
        f.SetOther(0,0,-3000,lane(random),forward(random));
        const auto out=f.Confirm();
        Require(out.Command.Throttle==f.Original.Throttle &&
            out.Command.Brake==f.Original.Brake &&
            out.Command.Arrived==f.Original.Arrived,"preserve existing command authority");
        if (out.SteeringChanged)
            Require(std::abs(out.Command.Steering)<=FleetPassMaxSteer &&
                out.LaneEvidence.Valid && out.LaneEvidence.Clear &&
                out.LeaderSlot>=0,"steering requires actual clear-lane evidence");
    }
}
}

int main()
{
    SweptIntervals();
    QualificationOnLongStraights();
    RealFleetBlockers();
    LeaderAndRosterValidation();
    AbortAndRearm();
    TimingAndContinuity();
    FrameAuthorityAndDriftEnvelope();
    RandomizedSweptWindowAndCommands();
    std::cout<<"Fleet passing lane geometry and state-machine tests passed\n";
}
