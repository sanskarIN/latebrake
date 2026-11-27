#pragma once

#include "Racing/PhysicalReplayHighlightClips.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>

namespace latebrake
{
// One GameMode admission and exactly six same-frame pawn samples.
// This adapter never changes gameplay, credits, clocks or transforms.
using FleetReplayIdentity = std::array<std::uint64_t,PhysicalReplayCarCount>;

struct FleetReplayFrameAdmission final
{
    std::uint64_t Attempt=0;
    std::uint64_t LocalFrame=0;
    std::uint64_t EngineFrame=0;
    std::uint64_t SessionEpoch=0;
    double ActiveSeconds=0.0;
    double AdvancedSeconds=0.0;
    FleetReplayIdentity Identity{};
};

struct FleetReplayContribution final
{
    int Slot=-1;
    std::uint64_t Attempt=0;
    std::uint64_t LocalFrame=0;
    std::uint64_t EngineFrame=0;
    std::uint64_t SessionEpoch=0;
    std::uint64_t TeleportRevision=0;
    PhysicalReplayCarSample Car{};
};

enum class FleetReplayCaptureStatus
{
    Unavailable,
    AwaitingCars,
    IncompleteFrame,
    InvalidAdmission,
    InvalidContribution,
    DuplicateContribution,
    RetainedFrame,
    Frozen,
    Discarded
};

struct FleetReplayCoverage final
{
    std::uint64_t AdmittedFrames=0;
    std::uint64_t RecordedFrames=0;
    std::uint64_t IncompleteFrames=0;
    std::uint64_t RefusedAdmissions=0;
    std::uint64_t RefusedContributions=0;
    std::uint64_t ContinuityBreaks=0;
    std::uint64_t Segments=0;
    std::size_t RetainedFrames=0;
    double FirstClockSeconds=0.0;
    double LastClockSeconds=0.0;
    bool HasReplay=false;
    // True only when finish selected an earlier complete segment rather
    // than the latest, unplayable one-frame fragment.
    bool EarlierSegmentUsed=false;
    bool Frozen=false;
    FleetReplayCaptureStatus Status=FleetReplayCaptureStatus::Unavailable;
};

struct FleetReplayHighlightRow final
{
    bool Valid=false;
    int FocusSlot=-1;
    ReplayBroadcastEventKind Kind=ReplayBroadcastEventKind::Unknown;
    double EventSeconds=0.0;
    double StartSeconds=0.0;
    double EndSeconds=0.0;
    double SampledSpeedKmh=0.0;
    unsigned Observations=0;
};

struct FleetReplayReview final
{
    bool Valid=false;
    std::uint64_t Attempt=0;
    FleetReplayCoverage Coverage{};
    unsigned HighlightCount=0;
    std::array<FleetReplayHighlightRow,PhysicalReplayMaximumHighlights> Highlights{};
};

inline const char* FleetReplayEventLabel(ReplayBroadcastEventKind kind) noexcept
{
    switch(kind)
    {
        case ReplayBroadcastEventKind::CloseTraffic: return "CLOSE TRAFFIC";
        case ReplayBroadcastEventKind::SideBySide: return "SIDE BY SIDE";
        case ReplayBroadcastEventKind::ApparentPositionSwap: return "OBSERVED SWAP";
        case ReplayBroadcastEventKind::TrafficSeparated: return "TRAFFIC SEPARATED";
        case ReplayBroadcastEventKind::StrongMeasuredBraking: return "MEASURED BRAKING";
        default: return "UNAVAILABLE";
    }
}

inline bool ValidFleetReplayIdentity(const FleetReplayIdentity& identity) noexcept
{
    for(std::size_t i=0;i<identity.size();++i)
    {
        if(identity[i]==0) return false;
        for(std::size_t j=0;j<i;++j)
            if(identity[i]==identity[j]) return false;
    }
    return true;
}

inline bool ValidFleetReplayAdmission(const FleetReplayFrameAdmission& a) noexcept
{
    return a.Attempt!=0 && a.LocalFrame!=0 && a.EngineFrame!=0 &&
        a.SessionEpoch!=0 && ValidFleetReplayIdentity(a.Identity) &&
        std::isfinite(a.ActiveSeconds) && a.ActiveSeconds>0.0 &&
        a.ActiveSeconds<=86400.0 && std::isfinite(a.AdvancedSeconds) &&
        a.AdvancedSeconds>=0.001 &&
        a.AdvancedSeconds<=PhysicalReplayMaximumFrameSeconds &&
        a.ActiveSeconds+PhysicalReplayClockEpsilon>=a.AdvancedSeconds;
}

inline bool FleetReplayFreshAdmission(const FleetReplayFrameAdmission& a,
    const FleetReplayFrameAdmission& b) noexcept
{
    return a.Attempt==b.Attempt && a.Identity==b.Identity &&
        b.LocalFrame>a.LocalFrame && b.EngineFrame>a.EngineFrame &&
        b.ActiveSeconds>a.ActiveSeconds && b.SessionEpoch>=a.SessionEpoch;
}

inline bool FleetReplayAdjacentAdmission(const FleetReplayFrameAdmission& a,
    const FleetReplayFrameAdmission& b) noexcept
{
    if(!FleetReplayFreshAdmission(a,b) || b.LocalFrame-a.LocalFrame!=1 ||
        b.EngineFrame-a.EngineFrame!=1 || b.SessionEpoch!=a.SessionEpoch)
        return false;
    const double dt=b.ActiveSeconds-a.ActiveSeconds;
    return dt<=PhysicalReplayMaximumFrameSeconds &&
        std::abs(dt-b.AdvancedSeconds)<=1.0e-6;
}

class FleetReplayCapture final
{
public:
    void Reset() noexcept
    {
        Timeline.Reset();
        Events.Reset();
        Archive.reset();
        Bound={};
        Attempt=0;
        LatestAdmission={};
        LatestRecorded={};
        LatestTeleport={};
        Pending={};
        Open=false;
        ForceBreak=false;
        Finished=false;
        Coverage={};
        Review={};
    }

    bool BeginFrame(const FleetReplayFrameAdmission& admission) noexcept
    {
        if(Finished) return false;
        // A stale or foreign admission is not a real break in the
        // current attempt. In particular it cannot close a newer frame.
        if((Attempt!=0 && (admission.Attempt!=Attempt || admission.Identity!=Bound)) ||
            (LatestAdmission.LocalFrame!=0 &&
                !FleetReplayFreshAdmission(LatestAdmission,admission)))
        {
            Increment(Coverage.RefusedAdmissions);
            return false;
        }
        CloseIncomplete();
        if(!ValidFleetReplayAdmission(admission))
        {
            Increment(Coverage.RefusedAdmissions);
            Coverage.Status=FleetReplayCaptureStatus::InvalidAdmission;
            BreakContinuity();
            return false;
        }
        if(Attempt==0)
        {
            Attempt=admission.Attempt;
            Bound=admission.Identity;
        }
        if(LatestAdmission.LocalFrame!=0 &&
            !FleetReplayAdjacentAdmission(LatestAdmission,admission))
            BreakContinuity();
        LatestAdmission=admission;
        Pending={};
        Pending.Frame.Session=admission.Attempt;
        Pending.Frame.SessionRevision=admission.SessionEpoch;
        Pending.Frame.SourceFrame=admission.LocalFrame;
        Pending.Frame.ActiveClockSeconds=admission.ActiveSeconds;
        Pending.Frame.SourceDeltaSeconds=admission.AdvancedSeconds;
        Open=true;
        Increment(Coverage.AdmittedFrames);
        Coverage.Status=FleetReplayCaptureStatus::AwaitingCars;
        return true;
    }

    bool Submit(const FleetReplayContribution& contribution) noexcept
    {
        if(Finished) return false;
        // Ignore foreign/stale callbacks without poisoning a newer frame.
        if(!Open || contribution.Attempt!=LatestAdmission.Attempt ||
            contribution.LocalFrame!=LatestAdmission.LocalFrame ||
            contribution.EngineFrame!=LatestAdmission.EngineFrame ||
            contribution.SessionEpoch!=LatestAdmission.SessionEpoch)
        {
            Increment(Coverage.RefusedContributions);
            return false;
        }
        if(contribution.Slot<0 || contribution.Slot>=static_cast<int>(PhysicalReplayCarCount))
        {
            RefuseOpen(FleetReplayCaptureStatus::InvalidContribution);
            return false;
        }
        const auto index=static_cast<std::size_t>(contribution.Slot);
        if(Pending.Seen[index])
        {
            RefuseOpen(FleetReplayCaptureStatus::DuplicateContribution);
            return false;
        }
        auto car=contribution.Car;
        car.Session=LatestAdmission.Attempt;
        car.SessionRevision=LatestAdmission.SessionEpoch;
        car.SourceFrame=LatestAdmission.LocalFrame;
        car.EngineClockRunning=true;
        if(car.Actor!=Bound[index] || !FinitePhysicalReplayCar(car))
        {
            RefuseOpen(FleetReplayCaptureStatus::InvalidContribution);
            return false;
        }
        Pending.Frame.Cars[index]=car;
        Pending.Teleport[index]=contribution.TeleportRevision;
        Pending.Seen[index]=true;
        ++Pending.Count;
        if(Pending.Count!=PhysicalReplayCarCount) return true;
        Pending.Frame.AllSixEngineActorsObserved=true;
        return Publish();
    }

    void Suspend() noexcept
    {
        if(Finished) return;
        CloseIncomplete();
        BreakContinuity();
        Coverage.Status=FleetReplayCaptureStatus::Unavailable;
    }

    FleetReplayReview Freeze(std::uint64_t attempt,const FleetReplayIdentity& identity,
        bool eligible) noexcept
    {
        if(Finished)
            return attempt==Attempt && identity==Bound && eligible ? Review : FleetReplayReview{};
        // Foreign callbacks must not freeze or discard the real attempt.
        if(attempt==0 || attempt!=Attempt || identity!=Bound) return {};
        // A finish may precede the sixth actor's tick. Keep only complete
        // observed frames and the actual last sampled clock.
        CloseIncomplete();
        Finished=true;
        Coverage.Frozen=true;
        if(!eligible)
        {
            Timeline.Reset();
            Events.Reset();
            Archive.reset();
            Coverage.Status=FleetReplayCaptureStatus::Discarded;
            RefreshCoverage();
            Review={};
            return Review;
        }
        // Never join disjoint windows. If the latest fragment contains
        // fewer than two samples, restore the last independently playable
        // segment and its own event log. Preserve total capture counters.
        if(!Timeline.HasReplay() && Archive && Archive->Timeline.HasReplay())
        {
            Timeline=Archive->Timeline;
            Events=Archive->Events;
            Coverage.EarlierSegmentUsed=true;
        }
        Archive.reset();
        Coverage.Status=FleetReplayCaptureStatus::Frozen;
        RefreshCoverage();
        Review=BuildReview();
        return Review;
    }

    FleetReplayCoverage ReadCoverage() const noexcept {return Coverage;}

    FleetReplayReview FrozenReview() const noexcept
    {
        return Finished ? Review : FleetReplayReview{};
    }

    PhysicalReplayPlaybackFrame EvaluateFrozen(std::uint64_t attempt,double seconds) const noexcept
    {
        if(!Finished || !Review.Valid || attempt!=Attempt) return {};
        return Timeline.Evaluate(seconds);
    }

private:
    struct ArchivedSegment final
    {
        PhysicalSixCarReplayTimeline Timeline;
        PhysicalReplayRaceEventLog Events;
    };

    // Allocate at most one additional fixed-capacity ring, and only on
    // an actual segment boundary. Allocation failure leaves capture
    // functional with the latest segment; it never grants authority.
    void StartSegment() noexcept
    {
        if(Timeline.HasReplay())
        {
            if(!Archive)
                Archive.reset(new(std::nothrow) ArchivedSegment{Timeline,Events});
            else
            {
                Archive->Timeline=Timeline;
                Archive->Events=Events;
            }
        }
        Timeline.Reset();
        Events.Reset();
    }

    struct Assembly final
    {
        PhysicalReplayFrame Frame{};
        FleetReplayIdentity Teleport{};
        std::array<bool,PhysicalReplayCarCount> Seen{};
        std::size_t Count=0;
    };

    static void Increment(std::uint64_t& value) noexcept
    {
        if(value!=std::numeric_limits<std::uint64_t>::max()) ++value;
    }

    void BreakContinuity() noexcept
    {
        if(LatestRecorded.LocalFrame!=0 && !ForceBreak) Increment(Coverage.ContinuityBreaks);
        ForceBreak=true;
    }

    void CloseIncomplete() noexcept
    {
        if(!Open) return;
        Open=false;
        Pending={};
        Increment(Coverage.IncompleteFrames);
        Coverage.Status=FleetReplayCaptureStatus::IncompleteFrame;
        BreakContinuity();
    }

    void RefuseOpen(FleetReplayCaptureStatus status) noexcept
    {
        CloseIncomplete();
        Increment(Coverage.RefusedContributions);
        Coverage.Status=status;
    }

    bool Publish() noexcept
    {
        if(ValidatePhysicalReplayFrame(Pending.Frame)!=PhysicalReplayReject::None)
        {
            RefuseOpen(FleetReplayCaptureStatus::InvalidContribution);
            return false;
        }
        bool newSegment=LatestRecorded.LocalFrame==0 || ForceBreak;
        if(LatestRecorded.LocalFrame!=0 &&
            (!FleetReplayAdjacentAdmission(LatestRecorded,LatestAdmission) ||
                LatestTeleport!=Pending.Teleport))
        {
            BreakContinuity();
            newSegment=true;
        }
        if(newSegment) StartSegment();
        auto receipt=Timeline.Record(Pending.Frame);
        // A measured position discontinuity cannot be interpolated.
        // Retain a new complete starting point instead of a fake bridge.
        if(!receipt.Accepted && !newSegment)
        {
            BreakContinuity();
            newSegment=true;
            StartSegment();
            receipt=Timeline.Record(Pending.Frame);
        }
        if(!receipt.Accepted)
        {
            RefuseOpen(FleetReplayCaptureStatus::InvalidContribution);
            RefreshCoverage();
            return false;
        }
        const auto event=Events.Observe(Pending.Frame);
        if(!event.Valid) Events.Reset();
        if(newSegment) Increment(Coverage.Segments);
        Increment(Coverage.RecordedFrames);
        ForceBreak=false;
        LatestRecorded=LatestAdmission;
        LatestTeleport=Pending.Teleport;
        Open=false;
        Pending={};
        Coverage.Status=FleetReplayCaptureStatus::RetainedFrame;
        RefreshCoverage();
        return true;
    }

    void RefreshCoverage() noexcept
    {
        Coverage.RetainedFrames=Timeline.FrameCount();
        Coverage.FirstClockSeconds=Timeline.EarliestClockSeconds();
        Coverage.LastClockSeconds=Timeline.LatestClockSeconds();
        Coverage.HasReplay=Timeline.HasReplay();
    }

    int FindSlot(std::uint64_t actor) const noexcept
    {
        if(!actor) return -1;
        for(std::size_t i=0;i<Bound.size();++i)
            if(Bound[i]==actor) return static_cast<int>(i);
        return -1;
    }

    FleetReplayReview BuildReview() const noexcept
    {
        FleetReplayReview out;
        out.Attempt=Attempt;
        out.Coverage=Coverage;
        if(!Timeline.HasReplay()) return out;
        out.Valid=true;
        const auto clips=AssemblePhysicalReplayHighlights(Timeline,Events);
        if(!clips.Valid) return out;
        for(unsigned i=0;i<clips.Count;++i)
        {
            const auto& clip=clips.Clips[i];
            const int slot=FindSlot(clip.FocusActor);
            if(slot<0 || !clip.Valid || clip.Session!=Attempt ||
                clip.RepresentativeEventClockSeconds<clip.StartClockSeconds ||
                clip.RepresentativeEventClockSeconds>clip.EndClockSeconds)
                continue;
            const auto playback=Timeline.Evaluate(clip.RepresentativeEventClockSeconds);
            if(!playback.Valid) continue;
            const auto& car=playback.Cars[static_cast<std::size_t>(slot)];
            if(!car.ProvenanceMatched || car.Actor!=clip.FocusActor) continue;
            out.Highlights[out.HighlightCount++]={true,slot,clip.SourceEvent,
                clip.RepresentativeEventClockSeconds,clip.StartClockSeconds,clip.EndClockSeconds,
                car.ObservedSpeedCmps*0.036,clip.EventsIncluded};
        }
        return out;
    }

    PhysicalSixCarReplayTimeline Timeline;
    PhysicalReplayRaceEventLog Events;
    std::unique_ptr<ArchivedSegment> Archive;
    FleetReplayIdentity Bound{};
    std::uint64_t Attempt=0;
    FleetReplayFrameAdmission LatestAdmission{};
    FleetReplayFrameAdmission LatestRecorded{};
    FleetReplayIdentity LatestTeleport{};
    Assembly Pending{};
    bool Open=false;
    bool ForceBreak=false;
    bool Finished=false;
    FleetReplayCoverage Coverage{};
    FleetReplayReview Review{};
};
} // namespace latebrake
