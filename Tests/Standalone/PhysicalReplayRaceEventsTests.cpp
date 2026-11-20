#include "Racing/PhysicalReplayRaceEvents.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool good,const char* description)
{
    if(!good)
    {
        std::cerr<<"PhysicalReplayRaceEvents FAIL: "<<description<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::PhysicalReplayFrame MakeFrame(
    std::uint64_t index,double clock)
{
    latebrake::PhysicalReplayFrame out;
    out.Session=99u;
    out.SessionRevision=5u;
    out.SourceFrame=index;
    out.ActiveClockSeconds=clock;
    out.SourceDeltaSeconds=0.05;
    out.AllSixEngineActorsObserved=true;
    for(std::size_t i=0;i<latebrake::PhysicalReplayCarCount;++i)
    {
        auto& c=out.Cars[i];
        c.Actor=500u+i;
        c.Session=out.Session;
        c.SessionRevision=out.SessionRevision;
        c.SourceFrame=index;
        c.WorldXcm=1000.0+static_cast<double>(i)*3000.0;
        c.WorldYcm=800.0+static_cast<double>(i)*600.0;
        c.WorldZcm=100.0;
        c.VelocityXcmps=10000.0;
        c.VelocityYcmps=0.0;
        c.VelocityZcmps=0.0;
        c.HeadingRadians=0.0;
        c.SteeringNormalized=0.0;
        c.ThrottleNormalized=0.5;
        c.BrakeNormalized=0.0;
        c.RoadContactConfidence=1.0;
        c.ActualChaosBodyObserved=true;
        c.OriginalRoadPhysicalContact=true;
        c.EngineClockRunning=true;
    }
    return out;
}
}

int main()
{
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    ReplayEventParameters settings;
    Require(ValidReplayEventParameters(settings),
            "conservative source-only broadcast parameters valid");
    settings.MinimumBrakingInput=nan;
    Require(!ValidReplayEventParameters(settings),
            "nonfinite threshold rejected");
    settings={};
    settings.NeverAwardRacingPlace=false;
    Require(!ValidReplayEventParameters(settings),
            "a replay event never authorizes official race pass");
    settings={};
    settings.NeverApplyActualBrake=false;
    Require(!ValidReplayEventParameters(settings),
            "broadcast source cannot command physical braking");
    settings={};

    auto initial=MakeFrame(1u,0.05);
    initial.Cars[0].WorldXcm=1000.0;
    initial.Cars[0].WorldYcm=1000.0;
    initial.Cars[1].WorldXcm=970.0;
    initial.Cars[1].WorldYcm=1250.0;
    auto close=ClassifyReplayPair(
        initial.Cars[0],initial.Cars[1]);
    Require(close.Classification==
                ReplayObservedPairClass::SideBySide &&
            close.ParallelSourceDirections &&
            close.AlongCm<0.0 &&
            close.LateralCm>0.0,
            "physical XY observation identifies unranked side-by-side cars");
    initial.Cars[1].HeadingRadians=
        3.1415926535897932384626433832795;
    auto reverse=ClassifyReplayPair(
        initial.Cars[0],initial.Cars[1]);
    Require(reverse.Classification==
                ReplayObservedPairClass::Distant,
            "opposed travel headings cannot qualify as parallel passing");
    initial.Cars[1].HeadingRadians=0.0;
    Require(ValidatePhysicalReplayFrame(initial)==
                PhysicalReplayReject::None,
            "six unique real actor witnesses have matching epoch/frame");
    PhysicalReplayRaceEventLog events;
    Require(!events.HasSource() && events.Size()==0u,
            "no imaginary broadcast moments before first real frame");
    auto receipt=events.Observe(initial);
    Require(receipt.Valid && receipt.NewEvents==0u &&
            receipt.PairComparisons==ReplayObservedPairCount &&
            events.Size()==0u && events.HasSource(),
            "first frame establishes pair baseline without inventing event");

    auto swap=MakeFrame(2u,0.10);
    swap.Cars[0].WorldXcm=1000.0;
    swap.Cars[0].WorldYcm=1000.0;
    swap.Cars[1].WorldXcm=1030.0;
    swap.Cars[1].WorldYcm=1250.0;
    swap.Cars[2].VelocityXcmps=9000.0;
    swap.Cars[2].BrakeNormalized=0.9;
    receipt=events.Observe(swap);
    Require(receipt.Valid && receipt.NewEvents>=2u &&
            events.Size()>=2u,
            "authentic close swap plus strong measured braking create events");
    ReplayBroadcastEvent latest;
    ReplayBroadcastEvent previous;
    Require(events.TryReadEvent(0u,latest) &&
            events.TryReadEvent(1u,previous),
            "newest replay events can be retrieved");
    Require(latest.Kind==
                ReplayBroadcastEventKind::StrongMeasuredBraking &&
            latest.PrimaryActor==502u &&
            latest.SecondaryActor==0u &&
            latest.MeasuredDecelCmps2>=200.0 &&
            !latest.GameplayAuthorityChanged &&
            !latest.OfficialPassAwarded,
            "measured braking highlight remains cosmetic");
    Require(previous.Kind==
                ReplayBroadcastEventKind::ApparentPositionSwap &&
            previous.PrimaryActor==500u &&
            previous.SecondaryActor==501u &&
            !previous.OfficialPassAwarded,
            "apparent side swap never becomes a certified overtake");
    const auto oldSize=events.Size();

    auto following=MakeFrame(3u,0.15);
    following.Cars[0].WorldXcm=1000.0;
    following.Cars[0].WorldYcm=1000.0;
    following.Cars[1].WorldXcm=1600.0;
    following.Cars[1].WorldYcm=1250.0;
    receipt=events.Observe(following);
    Require(receipt.Valid &&
            receipt.PairComparisons==ReplayObservedPairCount &&
            receipt.NewEvents==0u,
            "transition from side-by-side to following does not invent place");
    auto separating=MakeFrame(4u,0.20);
    separating.Cars[0].WorldXcm=1000.0;
    separating.Cars[0].WorldYcm=1000.0;
    separating.Cars[1].WorldXcm=2200.0;
    separating.Cars[1].WorldYcm=1250.0;
    receipt=events.Observe(separating);
    Require(receipt.Valid && events.Size()==oldSize,
            "intermediate following frame never invents separation");
    auto separated=MakeFrame(5u,0.25);
    separated.Cars[0].WorldXcm=1000.0;
    separated.Cars[0].WorldYcm=1000.0;
    separated.Cars[1].WorldXcm=2800.0;
    separated.Cars[1].WorldYcm=1250.0;
    receipt=events.Observe(separated);
    Require(receipt.Valid && events.Size()>oldSize,
            "physically continuous observed pair exit creates replay moment");
    Require(events.TryReadEvent(0u,latest) &&
            latest.Kind==
                ReplayBroadcastEventKind::TrafficSeparated,
            "contact classification transition recorded without scoring");
    ReplayBroadcastEvent nonexistent;
    Require(!events.TryReadEvent(events.Size(),nonexistent) &&
            !nonexistent.Valid,
            "out-of-range broadcast event never reads stale ring bytes");

    auto wrong=MakeFrame(6u,0.30);
    wrong.Cars[1].Actor=9999u;
    Require(!events.Observe(wrong).Valid &&
            !events.HasSource() && events.Size()==0u,
            "roster swap invalidates pending event confidence");
    Require(events.Observe(MakeFrame(1u,0.05)).Valid &&
            events.Size()==0u,
            "a reset requires a new physical baseline, no retroactive event");
    wrong=MakeFrame(2u,0.04);
    Require(!events.Observe(wrong).Valid &&
            !events.HasSource(),
            "clock rollback fails closed and clears source history");
    wrong=MakeFrame(1u,0.05);
    wrong.Cars[3].OriginalRoadPhysicalContact=false;
    Require(!events.Observe(wrong).Valid,
            "unverified asphalt never qualifies broadcast event");
    wrong=MakeFrame(1u,0.05);
    wrong.Cars[3].VelocityZcmps=nan;
    Require(!events.Observe(wrong).Valid,
            "nonfinite physical chassis velocities rejected");
    Require(events.Observe(MakeFrame(1u,0.05)).Valid,
            "valid new observation restores source-only review");
    wrong=MakeFrame(2u,0.10);
    wrong.Cars[2].WorldXcm+=1500.0;
    Require(!events.Observe(wrong).Valid &&
            !events.HasSource() &&
            events.Size()==0u,
            "teleport invalidates broadcast event history");
    std::cout<<"PhysicalReplayRaceEvents: 15 pairs source checks passed\n";
    return EXIT_SUCCESS;
}
