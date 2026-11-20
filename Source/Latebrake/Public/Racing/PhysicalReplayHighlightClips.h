#pragma once

#include "Racing/PhysicalReplayCameraDirector.h"
#include "Racing/PhysicalReplayRaceEvents.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// An offline, read-only cinematic highlight candidate. It combines only
// physically observed six-car telemetry and bounded replay-history windows.
// The official winner, overtake steward, career credits and game clock
// remain exclusively owned by the actual game simulation.
inline constexpr unsigned PhysicalReplayMaximumHighlights=12u;

struct PhysicalReplayHighlightSettings final
{
    double PreEventSeconds=1.25;
    double PostEventSeconds=2.5;
    double MaximumClipSeconds=8.0;
    double MinimumClipSeconds=0.3;
    double MergeGapSeconds=0.35;
    unsigned MaximumClips=PhysicalReplayMaximumHighlights;
    bool RequireOriginalPhysicalReplay=true;
    bool RefuseOfficialRaceScoring=true;
    bool RefuseCameraOcclusionClaims=true;
};

struct PhysicalReplayHighlightClip final
{
    bool Valid=false;
    ReplayBroadcastEventKind SourceEvent=
        ReplayBroadcastEventKind::Unknown;
    PhysicalReplayShot PreferredShot=
        PhysicalReplayShot::Chase;
    std::uint64_t Session=0;
    std::uint64_t FocusActor=0;
    std::uint64_t LastObservedSourceFrame=0;
    double FirstEventClockSeconds=0.0;
    double StartClockSeconds=0.0;
    double EndClockSeconds=0.0;
    unsigned EventsIncluded=0u;
    unsigned EditorialPriority=0u;
    bool OriginalReplayProvenance=true;
    bool ReplayOnly=true;
    bool OfficialPassingResultChanged=false;
    bool UnrealCameraSpawned=false;
};

struct PhysicalReplayHighlightPlaylist final
{
    bool Valid=false;
    std::uint64_t Session=0;
    std::uint64_t SessionRevision=0;
    std::array<PhysicalReplayHighlightClip,
        PhysicalReplayMaximumHighlights> Clips{};
    unsigned Count=0u;
    unsigned ConsideredEvents=0u;
    unsigned DiscardedEvents=0u;
    double TotalCandidateSeconds=0.0;
    bool NoGameplayAuthority=true;
    bool NoOfficialRanking=true;
};

inline bool ValidPhysicalReplayHighlightSettings(
    PhysicalReplayHighlightSettings p) noexcept
{
    const auto finite=[](double value) noexcept {
        return std::isfinite(value);
    };
    return finite(p.PreEventSeconds) &&
        p.PreEventSeconds>=0.0 &&
        p.PreEventSeconds<=4.0 &&
        finite(p.PostEventSeconds) &&
        p.PostEventSeconds>=0.10 &&
        p.PostEventSeconds<=6.0 &&
        finite(p.MaximumClipSeconds) &&
        p.MaximumClipSeconds>=1.0 &&
        p.MaximumClipSeconds<=15.0 &&
        p.PreEventSeconds+p.PostEventSeconds<=
            p.MaximumClipSeconds &&
        finite(p.MinimumClipSeconds) &&
        p.MinimumClipSeconds>0.0 &&
        p.MinimumClipSeconds<=p.MaximumClipSeconds &&
        finite(p.MergeGapSeconds) &&
        p.MergeGapSeconds>=0.0 &&
        p.MergeGapSeconds<=1.0 &&
        p.MaximumClips>=1u &&
        p.MaximumClips<=PhysicalReplayMaximumHighlights &&
        p.RequireOriginalPhysicalReplay &&
        p.RefuseOfficialRaceScoring &&
        p.RefuseCameraOcclusionClaims;
}

inline unsigned PhysicalReplayEventPriority(
    ReplayBroadcastEventKind kind) noexcept
{
    switch(kind)
    {
    case ReplayBroadcastEventKind::ApparentPositionSwap:return 5u;
    case ReplayBroadcastEventKind::SideBySide:return 4u;
    case ReplayBroadcastEventKind::StrongMeasuredBraking:return 3u;
    case ReplayBroadcastEventKind::CloseTraffic:return 2u;
    case ReplayBroadcastEventKind::TrafficSeparated:return 1u;
    default:return 0u;
    }
}

inline PhysicalReplayShot PhysicalReplayShotForEvent(
    ReplayBroadcastEventKind kind) noexcept
{
    switch(kind)
    {
    case ReplayBroadcastEventKind::ApparentPositionSwap:
        return PhysicalReplayShot::HighAerial;
    case ReplayBroadcastEventKind::SideBySide:
        return PhysicalReplayShot::RightSide;
    case ReplayBroadcastEventKind::StrongMeasuredBraking:
        return PhysicalReplayShot::Bonnet;
    case ReplayBroadcastEventKind::CloseTraffic:
        return PhysicalReplayShot::Chase;
    case ReplayBroadcastEventKind::TrafficSeparated:
        return PhysicalReplayShot::HighAerial;
    default:
        return PhysicalReplayShot::Chase;
    }
}

inline bool ReplayHighlightActorInFrame(
    const PhysicalReplayFrame& frame,
    std::uint64_t actor) noexcept
{
    if(actor==0 ||
       ValidatePhysicalReplayFrame(frame)!=
           PhysicalReplayReject::None)
        return false;
    for(const auto& car:frame.Cars)
        if(car.Actor==actor)return true;
    return false;
}

inline PhysicalReplayHighlightPlaylist AssemblePhysicalReplayHighlights(
    const PhysicalSixCarReplayTimeline& source,
    const PhysicalReplayRaceEventLog& events,
    PhysicalReplayHighlightSettings settings={}) noexcept
{
    PhysicalReplayHighlightPlaylist out;
    if(!ValidPhysicalReplayHighlightSettings(settings) ||
       !source.HasReplay())
        return out;
    PhysicalReplayFrame first;
    PhysicalReplayFrame last;
    if(!source.TryReadFrame(0u,first) ||
       !source.TryReadFrame(source.FrameCount()-1u,last) ||
       ValidatePhysicalReplayFrame(first)!=
           PhysicalReplayReject::None ||
       ValidatePhysicalReplayFrame(last)!=
           PhysicalReplayReject::None ||
       first.Session!=last.Session ||
       first.SessionRevision!=last.SessionRevision ||
       first.ActiveClockSeconds>=last.ActiveClockSeconds)
        return out;
    out.Session=first.Session;
    out.SessionRevision=first.SessionRevision;
    const double earliest=first.ActiveClockSeconds;
    const double latest=last.ActiveClockSeconds;
    for(std::size_t reverse=events.Size();reverse-- >0u;)
    {
        ReplayBroadcastEvent observation;
        if(!events.TryReadEvent(reverse,observation))
            continue;
        ++out.ConsideredEvents;
        const unsigned priority=PhysicalReplayEventPriority(
            observation.Kind);
        if(!observation.Valid ||
           priority==0u ||
           observation.PrimaryActor==0 ||
           observation.SourceSession!=first.Session ||
           observation.SourceFrame<first.SourceFrame ||
           observation.SourceFrame>last.SourceFrame ||
           !observation.PhysicalSourceRequired ||
           observation.OfficialPassAwarded ||
           observation.GameplayAuthorityChanged ||
           !std::isfinite(observation.ActiveClockSeconds) ||
           observation.ActiveClockSeconds<earliest ||
           observation.ActiveClockSeconds>latest)
        {
            ++out.DiscardedEvents;
            continue;
        }
        // The event is already independently validated by the observer.
        // A second session and actor check prevents stale ring entries
        // from creating clips after the frame-history window has wrapped.
        if(!ReplayHighlightActorInFrame(
               first,observation.PrimaryActor))
        {
            ++out.DiscardedEvents;
            continue;
        }
        const double start=std::max(
            earliest,observation.ActiveClockSeconds-
                     settings.PreEventSeconds);
        const double end=std::min(
            latest,observation.ActiveClockSeconds+
                   settings.PostEventSeconds);
        if(!std::isfinite(start) ||
           !std::isfinite(end) ||
           end-start<settings.MinimumClipSeconds)
        {
            ++out.DiscardedEvents;
            continue;
        }
        const auto shot=PhysicalReplayShotForEvent(
            observation.Kind);
        bool merged=false;
        // Merge only same-actor camera intervals. A pair change or
        // excessively long interval needs a separate bounded edit.
        if(out.Count>0u)
        {
            auto& candidate=out.Clips[out.Count-1u];
            const double proposedEnd=std::max(
                candidate.EndClockSeconds,end);
            if(candidate.FocusActor==observation.PrimaryActor &&
               start<=candidate.EndClockSeconds+
                   settings.MergeGapSeconds &&
               proposedEnd-candidate.StartClockSeconds<=
                   settings.MaximumClipSeconds)
            {
                candidate.EndClockSeconds=proposedEnd;
                candidate.LastObservedSourceFrame=
                    observation.SourceFrame;
                if(priority>candidate.EditorialPriority)
                {
                    candidate.SourceEvent=observation.Kind;
                    candidate.PreferredShot=shot;
                    candidate.EditorialPriority=priority;
                }
                ++candidate.EventsIncluded;
                merged=true;
            }
        }
        if(merged)continue;
        const double boundedEnd=std::min(
            end,start+settings.MaximumClipSeconds);
        if(boundedEnd-start<settings.MinimumClipSeconds)
        {
            ++out.DiscardedEvents;
            continue;
        }
        PhysicalReplayHighlightClip clip;
        clip.Valid=true;
        clip.SourceEvent=observation.Kind;
        clip.PreferredShot=shot;
        clip.Session=observation.SourceSession;
        clip.FocusActor=observation.PrimaryActor;
        clip.LastObservedSourceFrame=observation.SourceFrame;
        clip.FirstEventClockSeconds=
            observation.ActiveClockSeconds;
        clip.StartClockSeconds=start;
        clip.EndClockSeconds=boundedEnd;
        clip.EventsIncluded=1u;
        clip.EditorialPriority=priority;
        if(out.Count<settings.MaximumClips)
        {
            out.Clips[out.Count++]=clip;
        }
        else
        {
            // Bound memory while retaining the most RECENT meaningful
            // highlights, rather than an unbounded or timestamp-sorted log.
            for(unsigned i=1u;i<out.Count;++i)
                out.Clips[i-1u]=out.Clips[i];
            out.Clips[out.Count-1u]=clip;
        }
    }
    for(unsigned i=0u;i<out.Count;++i)
    {
        const auto& clip=out.Clips[i];
        if(!clip.Valid ||
           clip.StartClockSeconds<earliest ||
           clip.EndClockSeconds>latest ||
           clip.EndClockSeconds<=clip.StartClockSeconds ||
           clip.EndClockSeconds-clip.StartClockSeconds>
               settings.MaximumClipSeconds ||
           !source.Evaluate(clip.StartClockSeconds).Valid ||
           !source.Evaluate(clip.EndClockSeconds).Valid)
            return {};
        out.TotalCandidateSeconds+=
            clip.EndClockSeconds-clip.StartClockSeconds;
    }
    out.Valid=std::isfinite(out.TotalCandidateSeconds) &&
        out.TotalCandidateSeconds>=0.0;
    return out;
}
} // namespace latebrake
