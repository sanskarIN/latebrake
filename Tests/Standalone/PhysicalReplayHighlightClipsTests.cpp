#include "Racing/PhysicalReplayHighlightClips.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool condition,const char* what)
{
    if(!condition)
    {
        std::cerr<<"PhysicalReplayHighlightClips FAIL: "<<what<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double epsilon=1e-8)
{
    return std::abs(a-b)<=epsilon;
}
latebrake::PhysicalReplayFrame Frame(
    std::uint64_t id,double clock)
{
    latebrake::PhysicalReplayFrame f;
    f.Session=99u;
    f.SessionRevision=5u;
    f.SourceFrame=id;
    f.ActiveClockSeconds=clock;
    f.SourceDeltaSeconds=0.05;
    f.AllSixEngineActorsObserved=true;
    for(std::size_t i=0;i<latebrake::PhysicalReplayCarCount;++i)
    {
        auto& car=f.Cars[i];
        car.Actor=500u+i;
        car.Session=f.Session;
        car.SessionRevision=f.SessionRevision;
        car.SourceFrame=id;
        car.WorldXcm=1000.0+i*2900.0;
        car.WorldYcm=1000.0+i*600.0;
        car.WorldZcm=110.0;
        car.VelocityXcmps=10000.0;
        car.HeadingRadians=0.0;
        car.ThrottleNormalized=0.4;
        car.BrakeNormalized=0.0;
        car.ActualChaosBodyObserved=true;
        car.OriginalRoadPhysicalContact=true;
        car.EngineClockRunning=true;
    }
    f.Cars[0].WorldXcm=1000.0;
    f.Cars[0].WorldYcm=1000.0;
    f.Cars[1].WorldXcm=id==1u ? 970.0:1030.0;
    f.Cars[1].WorldYcm=1250.0;
    if(id>=2u)
    {
        f.Cars[2].VelocityXcmps=9000.0;
        f.Cars[2].BrakeNormalized=0.9;
    }
    return f;
}
}

int main()
{
    using namespace latebrake;
    PhysicalReplayHighlightSettings policy;
    Require(ValidPhysicalReplayHighlightSettings(policy),
            "default bounded replay editing settings valid");
    policy.RefuseOfficialRaceScoring=false;
    Require(!ValidPhysicalReplayHighlightSettings(policy),
            "highlight package cannot grant official place or credits");
    policy={};
    policy.MaximumClips=PhysicalReplayMaximumHighlights+1u;
    Require(!ValidPhysicalReplayHighlightSettings(policy),
            "unbounded montage clips are disallowed");
    policy={};
    policy.PreEventSeconds=7.0;
    Require(!ValidPhysicalReplayHighlightSettings(policy),
            "negative effective clip window cannot be accepted");

    Require(PhysicalReplayEventPriority(
                ReplayBroadcastEventKind::ApparentPositionSwap)>
            PhysicalReplayEventPriority(
                ReplayBroadcastEventKind::StrongMeasuredBraking),
            "cinematic pass-adjacent highlight has distinct priority");
    Require(PhysicalReplayEventPriority(
                ReplayBroadcastEventKind::Unknown)==0u,
            "invalid highlight cannot become an official race event");
    Require(PhysicalReplayShotForEvent(
                ReplayBroadcastEventKind::StrongMeasuredBraking)==
                PhysicalReplayShot::Bonnet,
            "braking chooses forward-looking cinematic framing");
    PhysicalSixCarReplayTimeline replay;
    PhysicalReplayRaceEventLog events;
    Require(!AssemblePhysicalReplayHighlights(
                replay,events).Valid,
            "no replay footage exists before observed source frames");

    for(std::uint64_t id=1u;id<=60u;++id)
    {
        const auto frame=Frame(
            id,static_cast<double>(id)*0.05);
        Require(replay.Record(frame).Accepted,
                "physically continuous six-car source frames populate buffer");
        const auto receipt=events.Observe(frame);
        Require(receipt.Valid &&
                receipt.PairComparisons==ReplayObservedPairCount,
                "highlight metadata derived from six real actor witnesses");
    }
    Require(replay.HasReplay() &&
            events.Size()>=2u,
            "source buffers contain real adjacent-pass and brake observations");
    const auto playlist=AssemblePhysicalReplayHighlights(
        replay,events);
    Require(playlist.Valid &&
            playlist.Session==99u &&
            playlist.SessionRevision==5u &&
            playlist.Count>=2u &&
            playlist.Count<=PhysicalReplayMaximumHighlights &&
            playlist.TotalCandidateSeconds>0.0 &&
            playlist.NoGameplayAuthority &&
            playlist.NoOfficialRanking &&
            playlist.ConsideredEvents>=2u,
            "finite replay event windows qualify purely cosmetic montage");
    bool foundApparentPass=false;
    bool foundBrake=false;
    for(unsigned i=0u;i<playlist.Count;++i)
    {
        const auto& clip=playlist.Clips[i];
        Require(clip.Valid &&
                clip.Session==99u &&
                clip.StartClockSeconds>=
                    replay.EarliestClockSeconds() &&
                clip.EndClockSeconds<=
                    replay.LatestClockSeconds() &&
                clip.EndClockSeconds>clip.StartClockSeconds &&
                clip.EndClockSeconds-clip.StartClockSeconds<=
                    policy.MaximumClipSeconds &&
                clip.EventsIncluded>=1u &&
                clip.OriginalReplayProvenance &&
                clip.ReplayOnly &&
                !clip.OfficialPassingResultChanged &&
                !clip.UnrealCameraSpawned,
                "each clip comes from bounded, source-observed race footage");
        Require(replay.Evaluate(
                    clip.StartClockSeconds).Valid &&
                replay.Evaluate(
                    clip.EndClockSeconds).Valid,
                "clip never extends beyond physical recorded frame ring");
        if(clip.SourceEvent==
           ReplayBroadcastEventKind::ApparentPositionSwap)
        {
            foundApparentPass=true;
            Require(clip.FocusActor==500u &&
                    clip.PreferredShot==
                        PhysicalReplayShot::HighAerial,
                    "apparent swap receives aerial source-only camera");
        }
        if(clip.SourceEvent==
           ReplayBroadcastEventKind::StrongMeasuredBraking)
        {
            foundBrake=true;
            Require(clip.FocusActor==502u &&
                    clip.PreferredShot==
                        PhysicalReplayShot::Bonnet,
                    "physical brake event receives bonnet camera");
        }
    }
    Require(foundApparentPass && foundBrake,
            "both independently observed highlights remain selectable");
    policy={};
    policy.MaximumClips=1u;
    const auto lastOnly=AssemblePhysicalReplayHighlights(
        replay,events,policy);
    Require(lastOnly.Valid && lastOnly.Count==1u &&
            lastOnly.Clips[0].Valid &&
            lastOnly.Clips[0].SourceEvent==
                ReplayBroadcastEventKind::StrongMeasuredBraking,
            "bounded playlist retains recent footage without heap growth");
    // A later, higher priority event must supply its own displayed timestamp,
    // while the earlier event remains in the cinematic interval.
    replay.Reset();
    events.Reset();
    for(std::uint64_t id=1u;id<=60u;++id)
    {
        auto frame=Frame(id,static_cast<double>(id)*0.05);
        frame.Cars[1].WorldXcm=id==1u ? 1350.0 : id==2u ? 1030.0 : 970.0;
        frame.Cars[2].VelocityXcmps=10000.0;
        frame.Cars[2].BrakeNormalized=0.0;
        Require(replay.Record(frame).Accepted && events.Observe(frame).Valid,
                "continuous source supports highlight promotion");
    }
    auto promoted=AssemblePhysicalReplayHighlights(replay,events);
    Require(promoted.Valid && promoted.Count==1u,
            "same pair observations merge into one bounded clip");
    const auto& clip=promoted.Clips[0];
    Require(clip.SourceEvent==ReplayBroadcastEventKind::ApparentPositionSwap &&
            clip.SecondaryActor==501u && clip.EventsIncluded==2u &&
            Near(clip.FirstEventClockSeconds,0.10) &&
            Near(clip.RepresentativeEventClockSeconds,0.15) &&
            clip.RepresentativeSourceFrame==3u,
            "promotion retains the exact event clock and source frame");

    ReplayBroadcastEvent event;
    Require(events.TryReadEvent(0u,event) &&
            ReplayHighlightHasSourceFrame(replay,event),
            "matching event is bound to its exact retained source");
    auto forged=event;
    ++forged.SourceRevision;
    Require(!ReplayHighlightHasSourceFrame(replay,forged),
            "same attempt with another continuity revision rejected");
    forged=event;
    ++forged.SourceFrame;
    Require(!ReplayHighlightHasSourceFrame(replay,forged),
            "another retained frame cannot supply the event clock");
    forged=event;
    forged.ActiveClockSeconds+=0.001;
    Require(!ReplayHighlightHasSourceFrame(replay,forged),
            "interpolated timestamp is not exact event evidence");
    forged=event;
    forged.SecondaryActor=99999u;
    Require(!ReplayHighlightHasSourceFrame(replay,forged),
            "unknown secondary actor cannot qualify");
    forged=event;
    forged.SecondaryActor=forged.PrimaryActor;
    Require(!ReplayHighlightHasSourceFrame(replay,forged),
            "same actor cannot supply both pair witnesses");

    // Identical session/frame/clock ranges are insufficient after an epoch change.
    replay.Reset();
    for(std::uint64_t id=1u;id<=60u;++id)
    {
        auto frame=Frame(id,static_cast<double>(id)*0.05);
        ++frame.SessionRevision;
        for(auto& car:frame.Cars) car.SessionRevision=frame.SessionRevision;
        Require(replay.Record(frame).Accepted,"new revision source captured");
    }
    auto stale=AssemblePhysicalReplayHighlights(replay,events);
    Require(stale.Valid && stale.Count==0u &&
            stale.DiscardedEvents==stale.ConsideredEvents,
            "stale continuity events cannot attach to a new replay segment");
    policy={};
    events.Reset();
    auto empty=AssemblePhysicalReplayHighlights(
        replay,events);
    Require(empty.Valid && empty.Count==0u &&
            Near(empty.TotalCandidateSeconds,0.0),
            "no event does not fabricate high-priority replay moments");
    replay.Reset();
    Require(!AssemblePhysicalReplayHighlights(
                replay,events).Valid,
            "session reset erases all source-only clipping authority");
    std::cout<<"PhysicalReplayHighlightClips: six-car source edits passed\n";
    return EXIT_SUCCESS;
}
