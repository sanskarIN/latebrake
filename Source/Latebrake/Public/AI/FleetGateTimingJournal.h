#pragma once

#include "AI/FleetPaceReview.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Bounded, attempt-local timing observations for the six physical cars.
// This journal never accepts a checkpoint, changes the active race clock,
// decides finishing position, or grants a reward. Only already-authorized
// gate receipts on the shared fee-free active clock may enter the journal.
inline constexpr int FleetTimingCarCount = 6;
inline constexpr int FleetTimingMaxGates = 256;
inline constexpr int FleetTimingMaxLaps = 100;
inline constexpr int FleetTimingHistoryCapacity = FleetTimingMaxGates + 1;
inline constexpr double FleetTimingMaximumSeconds = 7.0 * 24.0 * 3600.0;
inline constexpr double FleetTimingUncertainSeconds = 0.25;

using FleetTimingIdentity = std::array<std::uint64_t, FleetTimingCarCount>;
static_assert(FleetPaceCars == FleetTimingCarCount);
static_assert(FleetPaceMaxGates == FleetTimingMaxGates);
static_assert(FleetPaceMaxLaps == FleetTimingMaxLaps);
static_assert(FleetPaceMaxSeconds == FleetTimingMaximumSeconds);
static_assert(FleetPaceClockWindow == FleetTimingUncertainSeconds);

enum class FleetTimingAvailability
{
    Unavailable,
    WaitingForCommonGate,
    OutsideRetainedHistory,
    Matched
};

enum class FleetTimingRelation
{
    Unavailable,
    RivalEarlierObserved,
    SameClockWindow,
    RivalLaterObserved
};

struct FleetTimingGateReceipt final
{
    std::uint64_t Attempt = 0;
    std::uint64_t ActorToken = 0;
    int Slot = -1;
    int Ordinal = 0;
    double ActiveSeconds = 0.0;
};

struct FleetTimingEvidence final
{
    bool Verified = false;
    int AcceptedGates = 0;
    // Independent rival-ledger clock. The player is verified by its
    // authoritative accepted-gate count and the current active clock.
    double LastGateSeconds = 0.0;
};

struct FleetTimingInterval final
{
    bool Valid = false;
    int FromOrdinal = 0;
    int ToOrdinal = 0;
    double PlayerSeconds = 0.0;
    double RivalSeconds = 0.0;
    double RivalMinusPlayerSeconds = 0.0;
};

// A duration comparison, deliberately separate from the accumulated
// arrival-time gap. A rival can arrive earlier but lose time this sector.
enum class FleetSectorTrend
{
    Unavailable,
    RivalGained,
    WithinClockWindow,
    RivalLost
};

inline FleetSectorTrend ClassifyFleetSectorTrend(
    const FleetTimingInterval& interval) noexcept
{
    if(!interval.Valid || interval.FromOrdinal<0 ||
        interval.ToOrdinal<=interval.FromOrdinal ||
        !std::isfinite(interval.PlayerSeconds) ||
        !std::isfinite(interval.RivalSeconds) ||
        interval.PlayerSeconds<=0.0 || interval.RivalSeconds<=0.0 ||
        interval.PlayerSeconds>FleetTimingMaximumSeconds ||
        interval.RivalSeconds>FleetTimingMaximumSeconds ||
        !std::isfinite(interval.RivalMinusPlayerSeconds) ||
        interval.RivalMinusPlayerSeconds!=interval.RivalSeconds-interval.PlayerSeconds)
        return FleetSectorTrend::Unavailable;
    // Two arrival comparisons define a duration difference. Use twice
    // the single-gate uncertainty, not a falsely precise sector verdict.
    constexpr double sectorWindow=2.0*FleetTimingUncertainSeconds;
    if(std::abs(interval.RivalMinusPlayerSeconds)<=sectorWindow)
        return FleetSectorTrend::WithinClockWindow;
    return interval.RivalMinusPlayerSeconds<0.0 ?
        FleetSectorTrend::RivalGained : FleetSectorTrend::RivalLost;
}

inline const char* FleetSectorTrendName(FleetSectorTrend trend) noexcept
{
    switch(trend)
    {
        case FleetSectorTrend::RivalGained: return "AI GAINED";
        case FleetSectorTrend::WithinClockWindow: return "SECTOR CLOSE";
        case FleetSectorTrend::RivalLost: return "AI LOST TIME";
        default: return "";
    }
}

struct FleetCohortSectorTiming final
{
    bool Valid=false;
    int FromOrdinal=0;
    int ToOrdinal=0;
    double PlayerSeconds=0.0;
    int RivalsGained=0;
    int RivalsClose=0;
    int RivalsLost=0;
    double MinimumRivalSeconds=0.0;
    double MaximumRivalSeconds=0.0;
};

struct FleetMatchedGateTiming final
{
    int Slot = -1;
    FleetTimingAvailability Availability = FleetTimingAvailability::Unavailable;
    FleetTimingRelation Relation = FleetTimingRelation::Unavailable;
    int CommonOrdinal = 0;
    int LapNumber = 0;
    int GateNumber = 0;
    int PlayerAcceptedGates = 0;
    int RivalAcceptedGates = 0;
    double PlayerAtGateSeconds = 0.0;
    double RivalAtGateSeconds = 0.0;
    double RivalMinusPlayerSeconds = 0.0;
    FleetTimingInterval PreviousSector{};
    FleetTimingInterval LatestCommonLap{};
    FleetSectorTrend PreviousSectorTrend=FleetSectorTrend::Unavailable;
};

struct FleetTimingReadout final
{
    bool ValidPlayer = false;
    int MatchedRivals = 0;
    int WaitingRivals = 0;
    int HistoryUnavailableRivals = 0;
    int InvalidRivals = 0;
    // A cohort comparison is valid only when all five rivals have
    // independently retained timing for the very same accepted gate.
    // Pairwise rows can otherwise refer to different common gates.
    bool CommonCohortValid = false;
    int CohortOrdinal = 0;
    int CohortEarlier = 0;
    int CohortUncertain = 0;
    int CohortLater = 0;
    FleetCohortSectorTiming CohortSector{};
    std::array<FleetMatchedGateTiming, FleetTimingCarCount - 1> Rivals{};
};

class FleetGateTimingJournal final
{
public:
    void Reset() noexcept
    {
        Attempt = 0;
        GateCount = 0;
        TotalGates = 0;
        Identity = {};
        Streams = {};
    }

    bool Configure(std::uint64_t attempt, const FleetTimingIdentity& identities,
                   int gates, int laps) noexcept
    {
        // A live journal may never silently be rebound within the same
        // attempt. Explicit Reset belongs to the engine's new-attempt path.
        if (Attempt != 0 || attempt == 0 ||
            gates < 1 || gates > FleetTimingMaxGates ||
            laps < 1 || laps > FleetTimingMaxLaps)
        {
            return false;
        }
        for (int i = 0; i < FleetTimingCarCount; ++i)
        {
            const auto token = identities[static_cast<std::size_t>(i)];
            if (token == 0)
            {
                return false;
            }
            for (int j = 0; j < i; ++j)
            {
                if (token == identities[static_cast<std::size_t>(j)])
                {
                    return false;
                }
            }
        }
        Attempt = attempt;
        GateCount = gates;
        TotalGates = gates * laps;
        Identity = identities;
        for (auto& stream:Streams)
            stream.Pace.Configure(gates,laps);
        return true;
    }

    bool Record(const FleetTimingGateReceipt& receipt) noexcept
    {
        if (Attempt == 0 || receipt.Attempt != Attempt ||
            !ValidSlot(receipt.Slot) || receipt.ActorToken == 0 ||
            Identity[static_cast<std::size_t>(receipt.Slot)] != receipt.ActorToken ||
            receipt.Ordinal < 1 || receipt.Ordinal > TotalGates ||
            !ValidPositiveClock(receipt.ActiveSeconds))
        {
            return false;
        }
        auto& stream = Streams[static_cast<std::size_t>(receipt.Slot)];
        if (receipt.Ordinal != stream.LastOrdinal + 1 ||
            receipt.ActiveSeconds <= stream.LastSeconds)
        {
            return false;
        }
        // Share the exact accepted receipt; no separate HUD clock or gate
        // can add a lap to the whole-run pace history.
        if (!stream.Pace.Record(receipt.Ordinal,receipt.ActiveSeconds))
            return false;
        stream.Samples[Index(receipt.Ordinal)] =
            {receipt.Ordinal, receipt.ActiveSeconds};
        stream.LastOrdinal = receipt.Ordinal;
        stream.LastSeconds = receipt.ActiveSeconds;
        return true;
    }

    FleetTimingReadout Review(std::uint64_t attempt,
                              const FleetTimingIdentity& identities,
                              int gates, int laps, double activeSeconds,
                              const std::array<FleetTimingEvidence, FleetTimingCarCount>& evidence) const noexcept
    {
        FleetTimingReadout out;
        if (Attempt == 0 || attempt != Attempt || identities != Identity ||
            gates != GateCount || laps < 1 || laps > FleetTimingMaxLaps ||
            gates * laps != TotalGates ||
            !std::isfinite(activeSeconds) || activeSeconds < 0.0 ||
            activeSeconds > FleetTimingMaximumSeconds)
        {
            return out;
        }
        const auto& player = Streams[0];
        if (!MatchesEvidence(player, evidence[0], activeSeconds, false))
        {
            return out;
        }
        out.ValidPlayer = true;
        int cohortOrdinal = player.LastOrdinal;
        bool cohortEligible = cohortOrdinal > 0;
        for (int slot = 1; slot < FleetTimingCarCount; ++slot)
        {
            const auto index = static_cast<std::size_t>(slot);
            auto& row = out.Rivals[index - 1];
            row.Slot = slot;
            const auto& rival = Streams[index];
            if (!MatchesEvidence(rival, evidence[index], activeSeconds, true))
            {
                ++out.InvalidRivals;
                cohortEligible = false;
                continue;
            }
            row.PlayerAcceptedGates = player.LastOrdinal;
            row.RivalAcceptedGates = rival.LastOrdinal;
            const int ordinal = player.LastOrdinal < rival.LastOrdinal
                ? player.LastOrdinal : rival.LastOrdinal;
            if (ordinal < cohortOrdinal)
            {
                cohortOrdinal = ordinal;
            }
            if (ordinal == 0)
            {
                row.Availability = FleetTimingAvailability::WaitingForCommonGate;
                ++out.WaitingRivals;
                cohortEligible = false;
                continue;
            }
            const auto playerSample = At(player, ordinal);
            const auto rivalSample = At(rival, ordinal);
            if (!playerSample.Valid || !rivalSample.Valid)
            {
                row.Availability = FleetTimingAvailability::OutsideRetainedHistory;
                ++out.HistoryUnavailableRivals;
                cohortEligible = false;
                continue;
            }
            row.Availability = FleetTimingAvailability::Matched;
            row.CommonOrdinal = ordinal;
            row.LapNumber = (ordinal - 1) / GateCount + 1;
            row.GateNumber = (ordinal - 1) % GateCount + 1;
            row.PlayerAtGateSeconds = playerSample.Seconds;
            row.RivalAtGateSeconds = rivalSample.Seconds;
            row.RivalMinusPlayerSeconds = rivalSample.Seconds - playerSample.Seconds;
            row.Relation = RelationFor(row.RivalMinusPlayerSeconds);
            row.PreviousSector = Interval(player, rival, ordinal - 1, ordinal);
            row.PreviousSectorTrend=ClassifyFleetSectorTrend(row.PreviousSector);
            const int lapEnd = ordinal - ordinal % GateCount;
            if (lapEnd > 0)
            {
                row.LatestCommonLap = Interval(player, rival, lapEnd - GateCount, lapEnd);
            }
            ++out.MatchedRivals;
        }
        if (!cohortEligible || cohortOrdinal <= 0 ||
            out.MatchedRivals != FleetTimingCarCount - 1)
        {
            return out;
        }

        // Never aggregate pairwise rows that refer to different gates.
        // Reconstruct the exact same common gate for the entire cohort.
        const auto sharedPlayer = At(player, cohortOrdinal);
        if (!sharedPlayer.Valid)
        {
            return out;
        }
        int earlier = 0;
        int uncertain = 0;
        int later = 0;
        for (int slot = 1; slot < FleetTimingCarCount; ++slot)
        {
            const auto sample = At(Streams[static_cast<std::size_t>(slot)], cohortOrdinal);
            if (!sample.Valid)
            {
                return out;
            }
            switch (RelationFor(sample.Seconds - sharedPlayer.Seconds))
            {
                case FleetTimingRelation::RivalEarlierObserved: ++earlier; break;
                case FleetTimingRelation::SameClockWindow: ++uncertain; break;
                case FleetTimingRelation::RivalLaterObserved: ++later; break;
                default: return out;
            }
        }
        out.CommonCohortValid = true;
        out.CohortOrdinal = cohortOrdinal;
        out.CohortEarlier = earlier;
        out.CohortUncertain = uncertain;
        out.CohortLater = later;
        out.CohortSector=BuildCohortSector(cohortOrdinal);
        return out;
    }

    // Called for the frozen Results snapshot, not on every HUD frame.
    FleetPaceReadout ReviewPace(std::uint64_t attempt,
        const FleetTimingIdentity& identities,int gates,int laps,double activeSeconds,
        const std::array<FleetTimingEvidence,FleetTimingCarCount>& evidence) const noexcept
    {
        FleetPaceReadout out;
        if (Attempt == 0 || attempt != Attempt || identities != Identity ||
            gates != GateCount || laps < 1 || laps > FleetTimingMaxLaps ||
            gates*laps != TotalGates || !std::isfinite(activeSeconds) ||
            activeSeconds < 0.0 || activeSeconds > FleetTimingMaximumSeconds)
            return out;
        const auto& player = Streams[0];
        if (!MatchesEvidence(player,evidence[0],activeSeconds,false) ||
            !MatchesPace(player))
            return out;
        out.ValidPlayer = true;
        out.Player = player.Pace.Summary(0);
        std::array<const FleetPaceStream*,FleetPaceCars-1> cohort{};
        for (int slot=1;slot<FleetTimingCarCount;++slot)
        {
            const auto index = static_cast<std::size_t>(slot);
            auto& row = out.Rivals[index-1];
            row.Slot = slot;
            const auto& rival = Streams[index];
            if (!MatchesEvidence(rival,evidence[index],activeSeconds,true) ||
                !MatchesPace(rival))
            {
                ++out.UnavailableRivals;
                continue;
            }
            row = BuildFleetPaceRival(slot,player.Pace,rival.Pace);
            row.RetainedSectors=BuildRetainedSectorWindow(player,rival);
            ++out.VerifiedRivals;
            if (row.CommonLaps.Valid) ++out.ComparableRivals;
            cohort[index-1] = &rival.Pace;
        }
        out.Cohort = BuildFleetPaceCohort(player.Pace,cohort);
        return out;
    }

private:
    struct Sample final
    {
        int Ordinal = 0;
        double Seconds = 0.0;
    };

    struct SampleLookup final
    {
        bool Valid = false;
        double Seconds = 0.0;
    };

    struct Stream final
    {
        std::array<Sample, FleetTimingHistoryCapacity> Samples{};
        FleetPaceStream Pace{};
        int LastOrdinal = 0;
        double LastSeconds = 0.0;
    };

    bool MatchesPace(const Stream& stream) const noexcept
    {
        return stream.Pace.GateCount()==GateCount &&
            stream.Pace.RequiredLaps()*GateCount==TotalGates &&
            stream.Pace.AcceptedGates()==stream.LastOrdinal &&
            stream.Pace.LastGateClock()==stream.LastSeconds;
    }

    static bool ValidSlot(int slot) noexcept
    {
        return slot >= 0 && slot < FleetTimingCarCount;
    }

    static bool ValidPositiveClock(double seconds) noexcept
    {
        return std::isfinite(seconds) && seconds > 0.0 &&
            seconds <= FleetTimingMaximumSeconds;
    }

    static std::size_t Index(int ordinal) noexcept
    {
        return static_cast<std::size_t>((ordinal - 1) % FleetTimingHistoryCapacity);
    }

    static SampleLookup At(const Stream& stream, int ordinal) noexcept
    {
        // The common post-GO clock starts at zero. This synthetic
        // boundary is used only for elapsed intervals, never as a gate.
        if (ordinal == 0)
        {
            return {true, 0.0};
        }
        if (ordinal < 0 || ordinal > stream.LastOrdinal ||
            stream.LastOrdinal - ordinal >= FleetTimingHistoryCapacity)
        {
            return {};
        }
        const auto& sample = stream.Samples[Index(ordinal)];
        return sample.Ordinal == ordinal && ValidPositiveClock(sample.Seconds)
            ? SampleLookup{true, sample.Seconds} : SampleLookup{};
    }

    static bool MatchesEvidence(const Stream& stream, const FleetTimingEvidence& evidence,
                                double activeSeconds, bool requireIndependentClock) noexcept
    {
        if (!evidence.Verified || evidence.AcceptedGates != stream.LastOrdinal ||
            stream.LastSeconds > activeSeconds)
        {
            return false;
        }
        if (stream.LastOrdinal == 0)
        {
            return stream.LastSeconds == 0.0 &&
                (!requireIndependentClock || evidence.LastGateSeconds == 0.0);
        }
        return ValidPositiveClock(stream.LastSeconds) &&
            (!requireIndependentClock ||
             (ValidPositiveClock(evidence.LastGateSeconds) &&
              evidence.LastGateSeconds == stream.LastSeconds));
    }

    static FleetTimingRelation RelationFor(double delta) noexcept
    {
        if (!std::isfinite(delta))
        {
            return FleetTimingRelation::Unavailable;
        }
        if (std::abs(delta) <= FleetTimingUncertainSeconds)
        {
            return FleetTimingRelation::SameClockWindow;
        }
        return delta < 0.0 ? FleetTimingRelation::RivalEarlierObserved
                           : FleetTimingRelation::RivalLaterObserved;
    }

    static FleetTimingInterval Interval(const Stream& player, const Stream& rival,
                                       int from, int to) noexcept
    {
        if (from < 0 || to <= from)
        {
            return {};
        }
        const auto p0 = At(player, from);
        const auto p1 = At(player, to);
        const auto r0 = At(rival, from);
        const auto r1 = At(rival, to);
        if (!p0.Valid || !p1.Valid || !r0.Valid || !r1.Valid ||
            p1.Seconds <= p0.Seconds || r1.Seconds <= r0.Seconds)
        {
            return {};
        }
        const double playerDuration = p1.Seconds - p0.Seconds;
        const double rivalDuration = r1.Seconds - r0.Seconds;
        return {true, from, to, playerDuration, rivalDuration,
            rivalDuration - playerDuration};
    }

    FleetPaceRetainedWindow BuildRetainedSectorWindow(
        const Stream& player,const Stream& rival) const noexcept
    {
        FleetPaceRetainedWindow out;
        const int end=std::min(player.LastOrdinal,rival.LastOrdinal);
        if(end<1 || GateCount<1) return out;
        // Both physical streams must retain the previous endpoint.
        // GO is usable only if gate one itself remains in both rings.
        const int firstEndpoint=std::max(0,
            std::max(player.LastOrdinal,rival.LastOrdinal)-FleetTimingHistoryCapacity+1);
        if(firstEndpoint>=end) return out;
        const auto pStart=At(player,firstEndpoint);
        const auto rStart=At(rival,firstEndpoint);
        const auto pEnd=At(player,end);
        const auto rEnd=At(rival,end);
        if(!pStart.Valid || !rStart.Valid || !pEnd.Valid || !rEnd.Valid)
            return out;
        out.StartArrivalGapSeconds=rStart.Seconds-pStart.Seconds;
        out.EndArrivalGapSeconds=rEnd.Seconds-pEnd.Seconds;
        for(int ordinal=firstEndpoint+1;ordinal<=end;++ordinal)
        {
            const auto interval=Interval(player,rival,ordinal-1,ordinal);
            const auto trend=ClassifyFleetSectorTrend(interval);
            if(trend==FleetSectorTrend::Unavailable) return {};
            const FleetPaceRetainedSector row{true,ordinal-1,ordinal,
                (ordinal-1)%GateCount+1,(ordinal-1)/GateCount+1,
                interval.PlayerSeconds,interval.RivalSeconds,interval.RivalMinusPlayerSeconds};
            ++out.Count;
            out.PlayerTotalSeconds+=row.PlayerSeconds;
            out.RivalTotalSeconds+=row.RivalSeconds;
            out.MeanAbsoluteDifferenceSeconds+=std::abs(row.RivalMinusPlayerSeconds);
            switch(trend)
            {
                case FleetSectorTrend::RivalLost:
                    ++out.PlayerShorter;
                    if(!out.LargestPlayerGain.Valid ||
                        row.RivalMinusPlayerSeconds>out.LargestPlayerGain.RivalMinusPlayerSeconds)
                        out.LargestPlayerGain=row;
                    break;
                case FleetSectorTrend::RivalGained:
                    ++out.RivalShorter;
                    if(!out.LargestPlayerLoss.Valid ||
                        row.RivalMinusPlayerSeconds<out.LargestPlayerLoss.RivalMinusPlayerSeconds)
                        out.LargestPlayerLoss=row;
                    break;
                case FleetSectorTrend::WithinClockWindow: ++out.Close; break;
                default: return {};
            }
            out.Latest=row;
        }
        if(out.Count<1 || out.Count>FleetTimingHistoryCapacity ||
            out.PlayerShorter+out.RivalShorter+out.Close!=out.Count)
            return {};
        // The sum of sector durations must telescope to the actual
        // endpoint clocks. Allow only ordinary floating point error.
        const double pDuration=pEnd.Seconds-pStart.Seconds;
        const double rDuration=rEnd.Seconds-rStart.Seconds;
        const double tolerance=1e-8*std::max({1.0,pDuration,rDuration});
        if(std::abs(out.PlayerTotalSeconds-pDuration)>tolerance ||
            std::abs(out.RivalTotalSeconds-rDuration)>tolerance)
            return {};
        out.FirstOrdinal=firstEndpoint+1;
        out.LastOrdinal=end;
        out.MeanAbsoluteDifferenceSeconds/=static_cast<double>(out.Count);
        out.Valid=true;
        return out;
    }

    FleetCohortSectorTiming BuildCohortSector(int ordinal) const noexcept
    {
        FleetCohortSectorTiming out;
        // Every row must reference the same interval. Never average the
        // latest per-rival sectors when they describe different gates.
        if(ordinal<1 || ordinal>TotalGates) return out;
        out.FromOrdinal=ordinal-1;
        out.ToOrdinal=ordinal;
        const auto& player=Streams[0];
        for(int slot=1;slot<FleetTimingCarCount;++slot)
        {
            const auto interval=Interval(player,Streams[static_cast<std::size_t>(slot)],
                out.FromOrdinal,out.ToOrdinal);
            switch(ClassifyFleetSectorTrend(interval))
            {
                case FleetSectorTrend::RivalGained: ++out.RivalsGained; break;
                case FleetSectorTrend::WithinClockWindow: ++out.RivalsClose; break;
                case FleetSectorTrend::RivalLost: ++out.RivalsLost; break;
                default: return {};
            }
            if(slot==1)
            {
                out.PlayerSeconds=interval.PlayerSeconds;
                out.MinimumRivalSeconds=interval.RivalSeconds;
                out.MaximumRivalSeconds=interval.RivalSeconds;
            }
            else
            {
                if(interval.PlayerSeconds!=out.PlayerSeconds) return {};
                if(interval.RivalSeconds<out.MinimumRivalSeconds)
                    out.MinimumRivalSeconds=interval.RivalSeconds;
                if(interval.RivalSeconds>out.MaximumRivalSeconds)
                    out.MaximumRivalSeconds=interval.RivalSeconds;
            }
        }
        out.Valid=true;
        return out;
    }

    std::uint64_t Attempt = 0;
    int GateCount = 0;
    int TotalGates = 0;
    FleetTimingIdentity Identity{};
    std::array<Stream, FleetTimingCarCount> Streams{};
};
} // namespace latebrake
