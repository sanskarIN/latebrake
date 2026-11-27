#include "Racing/FleetSlipstreamDraft.h"
#include "Racing/SlipstreamDraftBoost.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Require(bool value, const char* message)
{
    if (!value)
    {
        std::cerr<<"FleetSlipstreamDraft: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a,double b)
{
    return std::abs(a-b)<1.0e-10;
}

struct Fixture final
{
    FleetDraftFrame Frame;
    Fixture()
    {
        Frame.Attempt=1;
        Frame.LocalFrame=1;
        Frame.EngineFrame=101;
        Frame.SessionRevision=10;
        Frame.BoundIdentity={100,101,102,103,104,105};
        auto& follower=Frame.Follower;
        follower.Token=100;
        follower.FrameSeconds=0.06;
        follower.ForwardSpeed=2000;
        follower.Throttle=0.8;
        follower.GroundSupported=true;
        follower.EligibleHumanRace=true;
        for (int slot=1;slot<FleetDraftCarCount;++slot)
        {
            auto& rival=At(slot);
            rival.Slot=slot;
            rival.Token=Frame.BoundIdentity[static_cast<std::size_t>(slot)];
            rival.PhysicalStateValid=true;
            rival.GroundSupported=true;
            rival.ClearPhysicalPath=true;
            rival.PositionX=10000+1000*slot;
            rival.VelocityX=2000;
        }
    }

    FleetDraftCandidate& At(int slot)
    {
        return Frame.Candidates[static_cast<std::size_t>(slot-1)];
    }

    void Place(int slot,double x,double y=0,double z=0)
    {
        auto& rival=At(slot);
        rival.PositionX=Frame.Follower.PositionX+x;
        rival.PositionY=Frame.Follower.PositionY+y;
        rival.PositionZ=Frame.Follower.PositionZ+z;
    }

    FleetDraftReadout Step(FleetSlipstreamDraft& meter)
    {
        const auto out=meter.Observe(Frame);
        ++Frame.LocalFrame;
        ++Frame.EngineFrame;
        return out;
    }

    FleetDraftReadout Confirm(FleetSlipstreamDraft& meter)
    {
        FleetDraftReadout out;
        for (int i=0;i<4;++i) out=Step(meter);
        return out;
    }
};

void StrongestOnlyAndStableIdentity()
{
    Fixture f;
    FleetSlipstreamDraft meter;
    for (int slot=1;slot<FleetDraftCarCount;++slot)
        f.Place(slot,700+slot*100,0);
    auto out=f.Step(meter);
    Require(out.Status==FleetDraftStatus::Confirming && out.TargetSlot==1 &&
        out.EligibleCandidates==5 && out.ExtraForce==0,
        "strongest fresh single target selected, no instant boost");
    out=f.Confirm(meter);
    Require(out.Active() && out.TargetToken==101 &&
        out.ExtraForce<=SlipstreamDraftBoost::MaxForceExtraFraction,
        "five wakes never sum");
    const auto candidate=f.At(1);
    SlipstreamDraftBoost single;
    const auto sample=MakeFleetDraftSample(f.Frame.Follower,candidate);
    for (int i=0;i<4;++i) single.Observe(sample);
    Require(Near(single.ExtraForce(),out.ExtraForce),
        "fleet bonus identical to established single-leader limit");
    f.Place(5,700);
    out=f.Step(meter);
    Require(out.Active() && out.TargetSlot==1,
        "a slightly stronger alternative cannot steal a confirmed stable target");

    f.At(1).ClearPhysicalPath=false;
    out=f.Step(meter);
    Require(out.TargetSlot==5 && out.Status==FleetDraftStatus::Confirming &&
        out.ExtraForce==0 && Near(out.QualifiedSeconds,0.06),
        "obstructed current leader forces independent confirmation");
    out=f.Confirm(meter);
    Require(out.Active() && out.TargetSlot==5,"new real leader may qualify");
}

void TieAndNoInventedTarget()
{
    Fixture f;
    FleetSlipstreamDraft meter;
    auto out=f.Step(meter);
    Require(out.Status==FleetDraftStatus::Searching && out.TargetSlot==-1 &&
        out.EligibleCandidates==0 && !out.Active(),"no fabricated target");
    f.Place(2,1400,20);
    f.Place(4,1400,-20);
    out=f.Step(meter);
    Require(out.TargetSlot==2 && out.EligibleCandidates==2,
        "deterministic lower-slot tie");
    f.At(2).RouteComplete=true;
    out=f.Step(meter);
    Require(out.TargetSlot==4 && out.ExtraForce==0,
        "finished opponent cannot provide aerodynamic help");
    f.At(4).GroundSupported=false;
    out=f.Step(meter);
    Require(out.Status==FleetDraftStatus::Searching && !out.Active(),
        "airborne leader cannot provide draft");
}

void EligibilityMatchesSingleLeader()
{
    Fixture f;
    f.Place(1,1400,30,20);
    auto& lead=f.At(1);
    auto& follower=f.Frame.Follower;
    const auto good=MakeFleetDraftSample(follower,lead);
    Require(MeasureFleetDraftPotential(good).Eligible,"physical baseline");
    auto reject=[&](const char* label)
    {
        Require(!MeasureFleetDraftPotential(MakeFleetDraftSample(follower,lead)).Eligible,label);
        lead.GroundSupported=true;
        lead.ClearPhysicalPath=true;
        lead.RouteComplete=false;
        lead.PositionX=1400;
        lead.PositionY=30;
        lead.PositionZ=20;
        lead.ForwardX=1;
        lead.ForwardY=0;
        lead.VelocityX=2000;
        lead.VelocityY=0;
        follower.ForwardSpeed=2000;
        follower.LateralSpeed=0;
        follower.Braking=false;
        follower.Throttle=0.8;
        follower.GroundSupported=true;
        follower.EligibleHumanRace=true;
    };
    lead.GroundSupported=false;reject("leader road support");
    lead.ClearPhysicalPath=false;reject("blocked sightline");
    lead.RouteComplete=true;reject("completed route");
    lead.PositionX=-1400;reject("rear body");
    lead.PositionX=699;reject("too close");
    lead.PositionX=3800;reject("too far");
    lead.PositionY=380;reject("too far laterally");
    lead.PositionZ=231;reject("vertical gap");
    lead.ForwardX=-1;lead.VelocityX=-2000;reject("opposite heading");
    lead.VelocityX=0;reject("stationary leader");
    lead.VelocityY=600;reject("lead lateral slip");
    follower.LateralSpeed=600;reject("follower lateral slip");
    follower.ForwardSpeed=3300;reject("closing speed cap");
    follower.Braking=true;reject("braking");
    follower.Throttle=0.29;reject("insufficient drive");
    follower.GroundSupported=false;reject("follower road support");
    follower.EligibleHumanRace=false;reject("nonhuman or inactive race");
    Require(MeasureFleetDraftPotential(MakeFleetDraftSample(follower,lead)).Eligible,
        "valid physical inputs restored");
}

void DiscontinuityAndNoCrossLeaderCredit()
{
    Fixture f;
    FleetSlipstreamDraft meter;
    f.Place(1,1400);
    f.Place(2,1800);
    Require(f.Confirm(meter).Active(),"baseline active");
    auto expectFresh=[&](const char* label)
    {
        const auto out=f.Step(meter);
        Require(out.Status==FleetDraftStatus::Confirming &&
            out.ExtraForce==0 && Near(out.QualifiedSeconds,0.06),label);
        Require(f.Confirm(meter).Active(),"reconfirmation completes");
    };
    ++f.At(1).TeleportRevision;expectFresh("leader recovery inside same lane");
    ++f.Frame.Follower.TeleportRevision;expectFresh("follower recovery");
    ++f.Frame.SessionRevision;expectFresh("checkpoint session changes");
    ++f.Frame.LocalFrame;expectFresh("missed local frame");
    ++f.Frame.EngineFrame;expectFresh("missed engine frame");
    ++f.Frame.Attempt;
    f.Frame.LocalFrame=1;
    expectFresh("new attempt");
    meter.Suspend();expectFresh("pause or focus interruption");
    f.Frame.BoundIdentity[1]=901;
    f.At(1).Token=901;
    expectFresh("replaced actor identity");
    f.At(1).ClearPhysicalPath=false;
    const auto out=f.Step(meter);
    Require(out.TargetSlot==2 && out.ExtraForce==0,
        "second leader cannot inherit first leader's meter");
}

void RelativeMotionAndReplay()
{
    Fixture f;
    FleetSlipstreamDraft meter;
    f.Place(1,1400);
    Require(f.Confirm(meter).Active(),"initial active");
    f.Place(1,1800);
    auto out=f.Step(meter);
    Require(!out.Active() && out.Status==FleetDraftStatus::Searching,
        "implausible relative offset jump revokes same-frame force");
    out=f.Step(meter);
    Require(out.Status==FleetDraftStatus::Confirming && out.ExtraForce==0,
        "fresh target after jump");
    const auto future=f.Frame;
    out=f.Step(meter);
    Require(out.QualifiedSeconds>0.06,"second continuous sample");
    out=meter.Observe(future);
    Require(!out.Active() && out.Status==FleetDraftStatus::Unavailable,
        "replayed stamp never accumulates");
    out=f.Step(meter);
    Require(out.ExtraForce==0 && Near(out.QualifiedSeconds,0.06),
        "replayed stamp revoked prior qualification");
    auto stale=f.Frame;
    stale.LocalFrame=1;
    stale.EngineFrame=101;
    Require(!meter.Observe(stale).Active(),"backward frame");
    meter.Reset();
    f.Frame.Attempt=1;
    f.Frame.LocalFrame=1;
    f.Frame.EngineFrame=1;
    Require(f.Step(meter).QualifiedSeconds>0.0,"explicit reset permits new world");
}

void CompleteRosterAndMalformedFrames()
{
    Fixture f;
    FleetSlipstreamDraft meter;
    f.Place(1,1400);
    Require(f.Confirm(meter).Active(),"valid baseline");
    f.At(5).PhysicalStateValid=false;
    Require(!f.Step(meter).Active(),"ineligible fifth body still must exist");
    f.At(5).PhysicalStateValid=true;
    f.At(5).Token=f.At(4).Token;
    Require(!f.Step(meter).Active(),"wrong actor token");
    f.At(5).Token=f.Frame.BoundIdentity[5];
    f.Frame.BoundIdentity[5]=f.Frame.BoundIdentity[4];
    Require(!f.Step(meter).Active(),"duplicate bound roster");
    f.Frame.BoundIdentity[5]=105;
    f.At(5).Slot=4;
    Require(!f.Step(meter).Active(),"duplicated slot");
    f.At(5).Slot=5;
    f.At(5).VelocityX=std::numeric_limits<double>::infinity();
    Require(!f.Step(meter).Active(),"nonfinite fifth body");
    f.At(5).VelocityX=2000;
    f.Frame.Follower.FrameSeconds=0.101;
    Require(!f.Step(meter).Active(),"hitch exceeds physical sampling cap");
    f.Frame.Follower.FrameSeconds=0.06;
    f.Frame.Follower.Token=0;
    Require(!f.Step(meter).Active(),"missing player");
    f.Frame.Follower.Token=100;
    const auto out=f.Step(meter);
    Require(out.Status==FleetDraftStatus::Confirming && out.ExtraForce==0,
        "valid physical roster recovers without old credit");
}

void RotatedWorldCoordinates()
{
    Fixture f;
    FleetSlipstreamDraft meter;
    f.Frame.Follower.ForwardX=0;
    f.Frame.Follower.ForwardY=1;
    f.Frame.Follower.PositionX=1250;
    f.Frame.Follower.PositionY=-900;
    auto& lead=f.At(3);
    lead.ForwardX=0;
    lead.ForwardY=1;
    lead.VelocityX=0;
    lead.VelocityY=2000;
    f.Place(3,80,1800,0);
    const auto out=f.Confirm(meter);
    Require(out.Active() && out.TargetSlot==3 &&
        Near(out.ForwardGapCm,1800) && Near(out.LateralGapCm,80),
        "real world direction rather than fixed track axis");
}

void RandomizedBoundedForceAndTargetTransitions()
{
    Fixture f;
    FleetSlipstreamDraft meter;
    std::mt19937 rng(58312);
    std::uniform_real_distribution<double> distance(500,4100);
    std::uniform_real_distribution<double> side(-500,500);
    std::uniform_real_distribution<double> velocity(500,3000);
    int previousSlot=-1;
    for (int tick=0;tick<10000;++tick)
    {
        if (tick%5==0)
        {
            for (int slot=1;slot<FleetDraftCarCount;++slot)
            {
                f.Place(slot,distance(rng),side(rng));
                auto& rival=f.At(slot);
                rival.VelocityX=velocity(rng);
                rival.ClearPhysicalPath=(rng()%7)!=0;
                rival.GroundSupported=(rng()%11)!=0;
                rival.RouteComplete=(rng()%19)==0;
            }
        }
        if (tick%31==0) ++f.Frame.SessionRevision;
        if (tick%53==0) ++f.At(3).TeleportRevision;
        f.Frame.Follower.Braking=(tick%17)==0;
        const auto out=f.Step(meter);
        Require(std::isfinite(out.ExtraForce) && out.ExtraForce>=0 &&
            out.ExtraForce<=SlipstreamDraftBoost::MaxForceExtraFraction,
            "bounded non-stacking motor fraction");
        Require(out.EligibleCandidates>=0 && out.EligibleCandidates<=5,
            "candidate count");
        if (out.TargetSlot>=1)
        {
            const auto& lead=f.At(out.TargetSlot);
            Require(out.TargetToken==lead.Token &&
                MeasureFleetDraftPotential(MakeFleetDraftSample(f.Frame.Follower,lead)).Eligible,
                "target corresponds to real qualified physical sample");
            if (previousSlot!=out.TargetSlot)
                Require(!out.Active() && Near(out.QualifiedSeconds,0.06),
                    "target change clears old qualification");
        }
        else
        {
            Require(!out.Active() && out.ExtraForce==0,"no target no force");
        }
        if (f.Frame.Follower.Braking)
            Require(!out.Active(),"no brake boost");
        previousSlot=out.TargetSlot;
    }
}
} // namespace

int main()
{
    StrongestOnlyAndStableIdentity();
    TieAndNoInventedTarget();
    EligibilityMatchesSingleLeader();
    DiscontinuityAndNoCrossLeaderCredit();
    RelativeMotionAndReplay();
    CompleteRosterAndMalformedFrames();
    RotatedWorldCoordinates();
    RandomizedBoundedForceAndTargetTransitions();
    std::cout<<"Fleet single-target slipstream selection and continuity passed\n";
}
