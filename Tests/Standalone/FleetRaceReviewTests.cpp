#include "AI/FleetRaceReview.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool value,const char* message)
{
    if (!value){std::cerr<<message<<'\n';std::exit(EXIT_FAILURE);}
}
struct Fixture final
{
    FleetPhysicalFrame Frame;
    SixCarIdentity Bound{};
    GridRivalLedger Ledger;
    std::array<FleetRouteObservation,StartGridCapacity> Routes{};
    Fixture()
    {
        Frame.Count=StartGridCapacity;Frame.AttemptToken=1;Frame.FrameToken=1;
        for(int i=0;i<StartGridCapacity;++i)
        {
            Bound[i]=static_cast<std::uint64_t>(100+i);
            Frame.Cars[i]={true,i,Bound[i],StartGridPoses[i],0.0,0.0};
            Routes[i].Verified=true;
        }
        Require(BindFleetReviewLedger(Ledger,Bound,4,3),"bind full roster");
    }
    void Advance(int slot,int ordinal,double time)
    {
        auto& route=Routes[slot];
        Require(Ledger.RecordAcceptedGate(slot,Bound[slot],ordinal,time),"accepted sequential gate");
        route.CompletedLaps=ordinal/4;route.NextGate=ordinal%4;route.Finished=ordinal==12;
    }
    void AdvanceTo(int slot,int ordinal,double end)
    {
        for(int i=1;i<=ordinal;++i) Advance(slot,i,end*i/ordinal);
    }
    FleetRaceReview Review(int playerOrdinal,double time)
    {
        return BuildFleetRaceReview(Frame,Bound,Ledger,Routes,
            playerOrdinal/4,playerOrdinal%4,playerOrdinal==12,4,3,time);
    }
};

void GateRelations()
{
    Fixture f;
    auto initial=f.Review(0,0.0);
    Require(initial.ValidRoster && initial.ComparedRivals==5 &&
        initial.SameGates==5 && initial.MoreGates==0 && initial.FewerGates==0,
        "initial physical roster no gates");
    f.AdvanceTo(1,7,10.0);f.AdvanceTo(2,6,9.0);f.AdvanceTo(3,4,8.0);
    f.AdvanceTo(4,12,11.0);f.AdvanceTo(5,1,2.0);
    auto review=f.Review(6,12.0);
    Require(review.PlayerAcceptedGates==6 && review.ComparedRivals==5 &&
        review.MoreGates==2 && review.SameGates==1 && review.FewerGates==2,
        "discrete counts, no invented ranking");
    Require(review.ObservedCompletedRoutes==1 && review.Rivals[3].NextGate==-1,
        "finished route no fictitious next gate");
    f.Routes[1].NextGate=0;
    review=f.Review(6,12.0);
    Require(review.UnavailableRivals==1 && review.ComparedRivals==4 &&
        !review.Rivals[0].RouteEvidenceValid && review.Rivals[0].Marker.Valid,
        "invalid progress cannot erase physical radar evidence");
    f.Routes[1].NextGate=3;
    f.Frame.Cars[5].ActorToken=999;
    Require(!f.Review(6,12.0).ValidRoster,"replacement denies full roster review");
}

void FinishAndRecovery()
{
    Fixture f;
    f.AdvanceTo(1,12,20.0);f.AdvanceTo(2,12,29.75);
    f.AdvanceTo(3,12,30.0);f.AdvanceTo(4,11,28.0);f.AdvanceTo(5,8,22.0);
    auto& route=f.Routes[4];
    Require(f.Ledger.RecordRecoveryResult(4,f.Bound[4],1,true),"verified recovery");
    Require(f.Ledger.RecordRecoveryResult(4,f.Bound[4],2,false),"failed recovery");
    route.RecoveryAttempts=2;route.SuccessfulRecoveries=1;route.FailedRecoveries=1;
    auto review=f.Review(12,30.0);
    Require(review.PlayerFinished && review.ObservedCompletedRoutes==3,
        "freeze observed routes at human finish");
    Require(review.Rivals[0].FinishRelation==FleetFinishRelation::EarlierObserved &&
        review.Rivals[0].EarlierObservedSeconds==10.0,"earlier sample not official classification");
    Require(review.Rivals[1].FinishRelation==FleetFinishRelation::SameClockWindow &&
        review.Rivals[2].FinishRelation==FleetFinishRelation::SameClockWindow,
        "0.25 second and identical clock windows uncertain");
    Require(review.Rivals[3].FinishRelation==FleetFinishRelation::RouteIncompleteAtPlayerFinish &&
        review.Rivals[3].SuccessfulRecoveries==1 && review.Rivals[3].FailedRecoveries==1,
        "verified attempt counters and incomplete route");
    route.SuccessfulRecoveries=2;route.FailedRecoveries=0;
    review=f.Review(12,30.0);
    Require(!review.Rivals[3].RouteEvidenceValid,"inconsistent recovery ledger denied");
    Require(f.Review(12,29.0).UnavailableRivals>=2,"future gate timestamps rejected");
    Require(!f.Review(12,std::numeric_limits<double>::infinity()).ValidRoster,
        "nonfinite active clock rejected");
    auto malformed=BuildFleetRaceReview(f.Frame,f.Bound,f.Ledger,f.Routes,3,1,true,4,3,30.0);
    Require(malformed.ValidRoster && !malformed.PlayerRouteValid && malformed.ComparedRivals==0,
        "malformed player route cannot claim relative comparisons");
}

void IdentityAndRadar()
{
    Fixture f;
    auto bad=f.Bound;bad[5]=bad[0];
    GridRivalLedger ledger;
    Require(!BindFleetReviewLedger(ledger,bad,4,3)&&!ledger.IsConfigured(),
        "human cannot occupy a rival slot");
    bad=f.Bound;bad[3]=bad[2];
    Require(!BindFleetReviewLedger(ledger,bad,4,3)&&!ledger.IsConfigured(),
        "duplicate AI cannot partially bind");
    Require(!MakeFleetRadarMarker({4200.01,0.0}).Valid &&
        !MakeFleetRadarMarker({0.0,-3000.01}).Valid &&
        !MakeFleetRadarMarker({std::numeric_limits<double>::quiet_NaN(),0.0}).Valid,
        "out-of-map and nonfinite bodies do not create edge markers");
    auto edge=MakeFleetRadarMarker({4200.0,-3000.0});
    Require(edge.Valid && edge.U==1.0 && edge.V==0.0,"inclusive original map edge");
    auto review=f.Review(0,0.0);
    for(int i=0;i<5;++i) review.Rivals[i].Marker={true,0.1*i,0.2};
    auto groups=GroupFleetRadar(review,0.01);
    Require(groups.Count==5,"separated markers retain individual identity");
    for(int i=0;i<5;++i)
        Require(groups.Groups[i].SlotMask==(1u<<static_cast<unsigned>(i+1)),
            "each individual marker has exact slot mask");
    review.Rivals[0].Marker={true,0.10,0.2};
    review.Rivals[1].Marker={true,0.14,0.2};
    review.Rivals[2].Marker={true,0.18,0.2};
    review.Rivals[3].Marker={true,0.8,0.8};
    review.Rivals[4].Marker={true,0.81,0.8};
    groups=GroupFleetRadar(review,0.05);
    Require(groups.Count==2 && groups.Groups[0].Count==3 &&
        groups.Groups[1].Count==2 && std::abs(groups.Groups[0].Marker.U-0.14)<1e-12 &&
        groups.Groups[0].SlotMask==14u,"transitive overlap grouping is deterministic");
    review.Rivals[1].Marker.Valid=false;
    groups=GroupFleetRadar(review,0.05);
    Require(groups.Count==3,"unobserved marker cannot bridge nearby cars");
    Require(GroupFleetRadar(review,-1.0).Count==0,"invalid cluster threshold rejected");
}

void SameStraightEvidence()
{
    for(int gate=0;gate<CircuitGateCount;++gate)
    {
        Fixture f;
        const auto from=CircuitGate((gate+CircuitGateCount-1)%CircuitGateCount);
        const auto to=CircuitGate(gate);
        const auto point=[&](double t,double offset=0.0)
        {
            const auto dx=to.X-from.X,dy=to.Y-from.Y;
            const auto length=std::hypot(dx,dy);
            return CircuitPoint{from.X+t*dx-offset*dy/length,
                from.Y+t*dy+offset*dx/length};
        };
        // Each rival has the same real accepted gate count. Their
        // longitudinal positions alone distinguish the three outcomes.
        const int ordinal=CircuitGateCount+gate;
        for(int slot=1;slot<StartGridCapacity;++slot) f.AdvanceTo(slot,ordinal,10.0);
        f.Frame.Cars[0].Pose.Position=point(0.5);
        f.Frame.Cars[1].Pose.Position=point(0.3);
        f.Frame.Cars[2].Pose.Position=point(0.7);
        f.Frame.Cars[3].Pose.Position=point(0.5,200.0);
        f.Frame.Cars[4].Pose.Position=point(0.01);
        f.Frame.Cars[5].Pose.Position=point(0.5,900.0);
        const auto review=f.Review(ordinal,12.0);
        Require(review.ValidRoster && review.PlayerRouteValid &&
            review.SameGates==5 && review.StraightPlayerAhead==1 &&
            review.StraightRivalAhead==1 && review.StraightClose==1 &&
            review.StraightUnavailable==2,"all four segments conserve evidence");
        Require(review.Rivals[0].Straight.Leader==RivalSegmentLeader::Player &&
            review.Rivals[1].Straight.Leader==RivalSegmentLeader::Rival &&
            review.Rivals[2].Straight.Leader==RivalSegmentLeader::TooClose &&
            !review.Rivals[3].Straight.Valid && !review.Rivals[4].Straight.Valid,
            "signed direction and unavailable evidence");
        const auto length=std::hypot(to.X-from.X,to.Y-from.Y);
        Require(std::abs(review.Rivals[0].Straight.AlongRoadSeparationMeters-
            0.2*length/100.0)<1e-9,"physical distance in metres");

        // Unverified gate evidence must not produce a spatial comparison,
        // even when the physical body is on the exact same straight.
        f.Routes[1].Verified=false;
        const auto missing=f.Review(ordinal,12.0);
        Require(missing.UnavailableRivals==1 && missing.SameGates==4 &&
            missing.StraightPlayerAhead==0 && !missing.Rivals[0].Straight.Valid,
            "missing gate evidence cannot be recovered from coordinates");
        f.Routes[1].Verified=true;
        f.Frame.Cars[0].Pose.Position=point(0.99);
        const auto corner=f.Review(ordinal,12.0);
        Require(corner.SameGates==5 && corner.StraightUnavailable==5 &&
            corner.StraightPlayerAhead==0 && corner.StraightRivalAhead==0 &&
            corner.StraightClose==0,"human near corner suppresses all projections");
    }

    Fixture finished;
    for(int slot=1;slot<StartGridCapacity;++slot)
        finished.AdvanceTo(slot,12,10.0);
    const auto review=finished.Review(12,11.0);
    Require(review.PlayerFinished && review.SameGates==5 &&
        review.StraightUnavailable==5 && review.StraightClose==0,
        "finished race is not projected as active spatial order");
}

void RandomizedClusterConservation()
{
    Fixture f;
    auto review=f.Review(0,0.0);
    std::mt19937 random(12345);
    std::uniform_real_distribution<double> unit(0.0,1.0);
    for(int iteration=0;iteration<4096;++iteration)
    {
        std::uint32_t expected=0;
        int count=0;
        for(int i=0;i<5;++i)
        {
            const bool visible=(random()&3u)!=0u;
            review.Rivals[i].Marker={visible,unit(random),unit(random)};
            if(visible){expected|=1u<<static_cast<unsigned>(i+1);++count;}
        }
        const auto groups=GroupFleetRadar(review,0.15);
        std::uint32_t combined=0;
        int total=0;
        for(int i=0;i<groups.Count;++i)
        {
            const auto& group=groups.Groups[i];
            Require(group.Marker.Valid && group.Count>0 &&
                group.Marker.U>=0.0 && group.Marker.U<=1.0 &&
                group.Marker.V>=0.0 && group.Marker.V<=1.0,
                "cluster centroid remains in map bounds");
            Require((combined&group.SlotMask)==0,"no car counted twice");
            combined|=group.SlotMask;total+=group.Count;
        }
        Require(combined==expected && total==count,"all observed cars retained exactly once");
    }
}
}
int main()
{
    GateRelations();FinishAndRecovery();IdentityAndRadar();SameStraightEvidence();RandomizedClusterConservation();
    std::cout<<"Fleet gate receipts, finish review and radar grouping passed\n";
}
