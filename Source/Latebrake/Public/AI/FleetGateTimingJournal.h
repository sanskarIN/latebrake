#pragma once

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
        int LastOrdinal = 0;
        double LastSeconds = 0.0;
    };

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

    std::uint64_t Attempt = 0;
    int GateCount = 0;
    int TotalGates = 0;
    FleetTimingIdentity Identity{};
    std::array<Stream, FleetTimingCarCount> Streams{};
};
} // namespace latebrake
