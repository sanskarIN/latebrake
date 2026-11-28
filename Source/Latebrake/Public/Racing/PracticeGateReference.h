#pragma once

#include "Racing/PracticeSessionHistory.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// A reference contains actual cumulative gate clocks for one complete
// practice attempt, not independently selected fastest sector fragments.
// The reference is isolated by the same exact configuration as the
// session finish history. No clock, rewards or vehicle controls consume it.
inline constexpr int PracticeMaximumGates=40;
inline constexpr double PracticeGateClockTolerance=1e-7;

struct PracticeGateClock final
{
    double TotalSeconds=0.0;
    double PenaltySeconds=0.0;

    double ActiveSeconds() const noexcept {return TotalSeconds-PenaltySeconds;}
};

inline bool PracticeGateEqual(double a,double b) noexcept
{
    return std::isfinite(a) && std::isfinite(b) &&
        std::abs(a-b)<=PracticeGateClockTolerance;
}

inline bool ValidPracticeGateClock(PracticeGateClock previous,
    PracticeGateClock current) noexcept
{
    return IsValidFinishedRaceTime(current.TotalSeconds) &&
        std::isfinite(current.PenaltySeconds) &&
        current.PenaltySeconds>=previous.PenaltySeconds &&
        current.PenaltySeconds<=current.TotalSeconds &&
        current.TotalSeconds>previous.TotalSeconds &&
        current.ActiveSeconds()>previous.ActiveSeconds();
}

struct PracticeGateTrace final
{
    int Count=0;
    int Expected=0;
    bool Complete=false;
    std::array<PracticeGateClock,PracticeMaximumGates> Gates{};

    PracticeGateClock At(int ordinal) const noexcept
    {
        return ordinal>0 && ordinal<=Count && ordinal<=PracticeMaximumGates
            ? Gates[static_cast<std::size_t>(ordinal-1)] : PracticeGateClock{};
    }

    bool Append(int ordinal,PracticeGateClock clock) noexcept
    {
        if(Complete || Expected<1 || Expected>PracticeMaximumGates ||
            Count<0 || Count>=Expected || ordinal!=Count+1 ||
            !ValidPracticeGateClock(At(Count),clock))
            return false;
        Gates[static_cast<std::size_t>(Count++)]=clock;
        return true;
    }

    bool Seal(const RaceProgress& race) noexcept
    {
        if(Complete || Expected<1 || Expected>PracticeMaximumGates ||
            Count!=Expected || !CanAwardFinishedRace(race) ||
            race.GetCheckpointCount()*static_cast<std::int64_t>(race.GetRequiredLaps())!=Expected ||
            !PracticeGateEqual(At(Count).TotalSeconds,race.GetElapsedSeconds()) ||
            !PracticeGateEqual(At(Count).PenaltySeconds,race.GetTotalPenaltySeconds()))
            return false;
        Complete=true;
        return true;
    }
};

struct PracticeGateDelta final
{
    bool Valid=false;
    int Ordinal=0;
    int Lap=0;
    int Gate=0;
    double CurrentTotalSeconds=0.0;
    double ReferenceTotalSeconds=0.0;
    double TotalDeltaSeconds=0.0;
    double ActiveDeltaSeconds=0.0;
    double FeeDeltaSeconds=0.0;
    double SectorDeltaSeconds=0.0;
    double SectorActiveDeltaSeconds=0.0;
    double SectorFeeDeltaSeconds=0.0;
};

inline PracticeGateDelta ComparePracticeGate(const PracticeGateTrace& current,
    const PracticeGateTrace& reference,int ordinal,int gates) noexcept
{
    if(!reference.Complete || current.Expected!=reference.Expected ||
        gates<1 || gates>PracticeMaximumGates ||
        current.Expected<1 || current.Expected>PracticeMaximumGates ||
        current.Expected%gates!=0 || ordinal<1 || ordinal>current.Count ||
        ordinal>reference.Count || ordinal>PracticeMaximumGates)
        return {};
    const auto a=current.At(ordinal),b=reference.At(ordinal);
    const auto ap=current.At(ordinal-1),bp=reference.At(ordinal-1);
    if(!ValidPracticeGateClock(ap,a) || !ValidPracticeGateClock(bp,b))
        return {};
    PracticeGateDelta out;
    out.Ordinal=ordinal;
    out.Lap=(ordinal-1)/gates+1;
    out.Gate=(ordinal-1)%gates;
    out.CurrentTotalSeconds=a.TotalSeconds;
    out.ReferenceTotalSeconds=b.TotalSeconds;
    out.TotalDeltaSeconds=a.TotalSeconds-b.TotalSeconds;
    out.ActiveDeltaSeconds=a.ActiveSeconds()-b.ActiveSeconds();
    out.FeeDeltaSeconds=a.PenaltySeconds-b.PenaltySeconds;
    out.SectorDeltaSeconds=(a.TotalSeconds-ap.TotalSeconds)-(b.TotalSeconds-bp.TotalSeconds);
    out.SectorActiveDeltaSeconds=(a.ActiveSeconds()-ap.ActiveSeconds())-
        (b.ActiveSeconds()-bp.ActiveSeconds());
    out.SectorFeeDeltaSeconds=(a.PenaltySeconds-ap.PenaltySeconds)-
        (b.PenaltySeconds-bp.PenaltySeconds);
    if(!PracticeGateEqual(out.TotalDeltaSeconds,out.ActiveDeltaSeconds+out.FeeDeltaSeconds) ||
        !PracticeGateEqual(out.SectorDeltaSeconds,out.SectorActiveDeltaSeconds+out.SectorFeeDeltaSeconds))
        return {};
    out.Valid=true;
    return out;
}

struct PracticeGateReview final
{
    bool Valid=false;
    bool Finished=false;
    bool ReferenceAvailable=false;
    bool NewReference=false;
    bool EvidenceRejected=false;
    int ObservedGates=0;
    int ExpectedGates=0;
    int FasterSectors=0;
    int SlowerSectors=0;
    int MatchedSectors=0;
    double ReferenceFinishSeconds=0.0;
    PracticeGateDelta Latest{};
    PracticeGateDelta LargestGain{};
    PracticeGateDelta LargestLoss{};
    std::array<PracticeGateDelta,PracticeMaximumGates> Rows{};
};

class PracticeGateReferenceHistory final
{
public:
    void CancelAttempt() noexcept
    {
        Active=false;
        Rejected=false;
        Key={};
        Actor=0;
        Index=-1;
        Current={};
        Review={};
    }

    void ResetSession() noexcept
    {
        References.fill({});
        CancelAttempt();
    }

    bool Begin(PracticeConfiguration key,std::uint64_t actor) noexcept
    {
        CancelAttempt();
        const auto definition=GetQuickPracticeDefinition(key.Preset);
        const int index=PracticeConfigurationIndex(key);
        const int expected=definition.CheckpointCount*definition.RequiredLaps;
        if(index<0 || actor==0 || !definition.Valid ||
            expected<1 || expected>PracticeMaximumGates)
            return false;
        Active=true;
        Key=key;
        Actor=actor;
        Index=index;
        Current.Expected=expected;
        Review.ExpectedGates=expected;
        const auto& ref=References[static_cast<std::size_t>(Index)];
        Review.ReferenceAvailable=ref.Complete && ref.Expected==expected;
        if(Review.ReferenceAvailable)
            Review.ReferenceFinishSeconds=ref.At(expected).TotalSeconds;
        return true;
    }

    void Invalidate() noexcept
    {
        if(!Active) return;
        Rejected=true;
        Review={};
        Review.ExpectedGates=Current.Expected;
        Review.EvidenceRejected=true;
    }

    void ObserveConfiguration(PracticeConfiguration key,std::uint64_t actor) noexcept
    {
        if(Active && (!(key==Key) || actor!=Actor || actor==0))
            Invalidate();
    }

    // The accepted checkpoint callback supplies the real post-gate race
    // state. A skipped, duplicated or zero-active-time sector makes the
    // exact-gate reference unavailable, but never changes race progress.
    bool ObserveGate(PracticeConfiguration key,std::uint64_t actor,
        const RaceProgress& race,int checkpoint) noexcept
    {
        if(!Active || Rejected) return false;
        ObserveConfiguration(key,actor);
        if(Rejected) return false;
        const auto definition=GetQuickPracticeDefinition(Key.Preset);
        const auto ordinal=static_cast<std::int64_t>(race.GetCompletedLaps())*
            race.GetCheckpointCount()+race.GetNextCheckpoint();
        if(race.GetCheckpointCount()!=definition.CheckpointCount ||
            race.GetRequiredLaps()!=definition.RequiredLaps ||
            checkpoint<0 || checkpoint>=definition.CheckpointCount ||
            ordinal<1 || ordinal>Current.Expected ||
            (ordinal-1)%definition.CheckpointCount!=checkpoint ||
            race.GetLastSectorIndex()!=checkpoint ||
            !Current.Append(static_cast<int>(ordinal),{race.GetElapsedSeconds(),
                race.GetTotalPenaltySeconds()}))
        {
            Invalidate();
            return false;
        }
        Review.ObservedGates=Current.Count;
        if(!Review.ReferenceAvailable) return true;
        const auto& ref=References[static_cast<std::size_t>(Index)];
        const auto delta=ComparePracticeGate(Current,ref,Current.Count,definition.CheckpointCount);
        if(!delta.Valid)
        {
            Invalidate();
            return false;
        }
        Review.Valid=true;
        Review.Latest=delta;
        Review.Rows[static_cast<std::size_t>(Current.Count-1)]=delta;
        if(delta.SectorDeltaSeconds<-PracticeGateClockTolerance)
        {
            ++Review.FasterSectors;
            if(!Review.LargestGain.Valid ||
                delta.SectorDeltaSeconds<Review.LargestGain.SectorDeltaSeconds)
                Review.LargestGain=delta;
        }
        else if(delta.SectorDeltaSeconds>PracticeGateClockTolerance)
        {
            ++Review.SlowerSectors;
            if(!Review.LargestLoss.Valid ||
                delta.SectorDeltaSeconds>Review.LargestLoss.SectorDeltaSeconds)
                Review.LargestLoss=delta;
        }
        else ++Review.MatchedSectors;
        return true;
    }

    PracticeGateReview Finish(const PracticeComparison& comparison,
        const RaceProgress& race) noexcept
    {
        if(!Active) return {};
        const bool comparable=comparison.Status==PracticeComparisonStatus::First ||
            comparison.Status==PracticeComparisonStatus::Improved ||
            comparison.Status==PracticeComparisonStatus::Matched ||
            comparison.Status==PracticeComparisonStatus::Slower;
        if(Rejected || !comparable || !(comparison.Configuration==Key) ||
            !Current.Seal(race))
        {
            Invalidate();
            Active=false;
            Review.Finished=true;
            return Review;
        }
        Active=false;
        Review.Finished=true;
        auto& reference=References[static_cast<std::size_t>(Index)];
        const double total=Current.At(Current.Count).TotalSeconds;
        if(!reference.Complete || total<reference.At(reference.Count).TotalSeconds)
        {
            reference=Current;
            Review.NewReference=true;
        }
        return Review;
    }

    PracticeGateReview Read() const noexcept {return Review;}

    double ReferenceFinish(PracticeConfiguration key) const noexcept
    {
        const int index=PracticeConfigurationIndex(key);
        if(index<0) return 0.0;
        const auto& trace=References[static_cast<std::size_t>(index)];
        return trace.Complete?trace.At(trace.Count).TotalSeconds:0.0;
    }

private:
    std::array<PracticeGateTrace,PracticeConfigurationCount> References{};
    PracticeGateTrace Current{};
    PracticeGateReview Review{};
    PracticeConfiguration Key{};
    std::uint64_t Actor=0;
    int Index=-1;
    bool Active=false;
    bool Rejected=false;
};
} // namespace latebrake
