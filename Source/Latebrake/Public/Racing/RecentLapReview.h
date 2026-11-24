#pragma once

#include "Racing/FinishTimingEligibility.h"
#include "Racing/RecentTimedLaps.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace latebrake
{
// Read-only, bounded results detail for the last eight genuinely timed
// completed laps. Newest first, including actual physical lap numbers.
// Never awards rewards, creates record times, changes penalties or saves data.
struct RecentLapReviewRow final
{
    int CompletedLap = 0;
    double LapSeconds = 0.0;
    double SecondsBehindRunBest = 0.0;
    double DeltaToPreviousSeconds = 0.0; // Positive means slower.
    bool HasPrevious = false;
    bool EqualsRunBest = false;
    bool PenaltyAttributionKnown = false;
    double IncludedPenaltySeconds = 0.0;
    double ClockExcludingPenaltySeconds = 0.0;
    // A positive delta means more penalty time than the prior physical lap.
    bool HasAdjacentPenaltyComparison = false;
    double PenaltyDeltaToPreviousSeconds = 0.0;
    double ClockDeltaToPreviousSeconds = 0.0;
};

struct RecentLapReview final
{
    bool Valid = false;
    int Count = 0;
    std::array<RecentLapReviewRow, RecentTimedLaps::Capacity> Rows{};
    bool CompletePenaltyAttribution = false;
    bool CoversEntireRace = false;
    int PenaltyFreeLaps = 0;
    int PenalizedLaps = 0;
    double RetainedPenaltySeconds = 0.0;
    double RetainedMeasuredSeconds = 0.0;
    double RetainedClockExcludingPenaltySeconds = 0.0;
    int LargestPenaltyLap = 0;
    double LargestPenaltySeconds = 0.0;
};

inline RecentLapReview BuildRecentLapReview(
    const RaceProgress& race) noexcept
{
    RecentLapReview result;
    // An ordered finish does not prove that all laps were actively timed.
    // Do not decorate penalty-only or otherwise unawardable finishes.
    if (!CanAwardFinishedRace(race))
    {
        return result;
    }

    const int count = std::clamp(
        race.GetRecentTimedLapCount(), 0, RecentTimedLaps::Capacity);
    const double best = race.GetBestLapSeconds();
    if (count == 0 || !IsValidFinishedRaceTime(best))
    {
        return result;
    }

    result.CompletePenaltyAttribution = true;
    result.CoversEntireRace = count == race.GetCompletedLaps();
    // Build atomically: an invalid/out-of-order sample invalidates the
    // whole advisory instead of showing an apparently complete summary.
    for (int offset = 0; offset < count; ++offset)
    {
        const TimedLapRecord lap = race.GetRecentTimedLap(offset);
        if (!lap.Valid() || !IsValidFinishedRaceTime(lap.Seconds) ||
            lap.CompletedLap > race.GetCompletedLaps() ||
            (offset > 0 &&
             lap.CompletedLap >= result.Rows[offset - 1].CompletedLap))
        {
            return {};
        }

        const double gap = lap.Seconds - best;
        if (!std::isfinite(gap) || gap < 0.0)
        {
            return {};
        }

        RecentLapReviewRow row;
        row.CompletedLap = lap.CompletedLap;
        row.LapSeconds = lap.Seconds;
        row.SecondsBehindRunBest = gap;
        row.EqualsRunBest = gap == 0.0;
        row.PenaltyAttributionKnown = lap.PenaltyAttributionKnown;
        result.RetainedMeasuredSeconds += lap.Seconds;
        if (row.PenaltyAttributionKnown)
        {
            if (!std::isfinite(lap.PenaltySeconds) ||
                lap.PenaltySeconds < 0.0 || lap.PenaltySeconds > lap.Seconds)
                return {};
            row.IncludedPenaltySeconds = lap.PenaltySeconds;
            row.ClockExcludingPenaltySeconds = lap.Seconds - lap.PenaltySeconds;
            result.RetainedPenaltySeconds += row.IncludedPenaltySeconds;
            result.RetainedClockExcludingPenaltySeconds += row.ClockExcludingPenaltySeconds;
            if (row.IncludedPenaltySeconds > 0.0)
            {
                ++result.PenalizedLaps;
                if (row.IncludedPenaltySeconds > result.LargestPenaltySeconds)
                {
                    result.LargestPenaltySeconds = row.IncludedPenaltySeconds;
                    result.LargestPenaltyLap = row.CompletedLap;
                }
            }
            else
            {
                ++result.PenaltyFreeLaps;
            }
        }
        else
        {
            result.CompletePenaltyAttribution = false;
        }
        if (offset + 1 < count)
        {
            // This is the preceding STORED timed lap, not an invented
            // adjacent physical lap when history contains gaps.
            const TimedLapRecord previous = race.GetRecentTimedLap(offset + 1);
            if (!previous.Valid() ||
                !IsValidFinishedRaceTime(previous.Seconds) ||
                previous.CompletedLap >= lap.CompletedLap)
            {
                return {};
            }
            row.DeltaToPreviousSeconds = lap.Seconds - previous.Seconds;
            if (!std::isfinite(row.DeltaToPreviousSeconds))
            {
                return {};
            }
            row.HasPrevious = true;
            if (row.PenaltyAttributionKnown && previous.PenaltyAttributionKnown &&
                lap.CompletedLap == previous.CompletedLap + 1 &&
                std::isfinite(previous.PenaltySeconds) &&
                previous.PenaltySeconds >= 0.0 && previous.PenaltySeconds <= previous.Seconds)
            {
                row.HasAdjacentPenaltyComparison = true;
                row.PenaltyDeltaToPreviousSeconds =
                    row.IncludedPenaltySeconds - previous.PenaltySeconds;
                row.ClockDeltaToPreviousSeconds =
                    row.ClockExcludingPenaltySeconds -
                    (previous.Seconds - previous.PenaltySeconds);
            }
        }
        result.Rows[offset] = row;
    }
    // Retained totals must reconcile against the authoritative whole-run
    // clock. Do not claim complete accounting after a ring eviction.
    if (result.CompletePenaltyAttribution)
    {
        constexpr double tolerance = 1e-7;
        const double total = race.GetTotalPenaltySeconds();
        const bool consistent =
            std::isfinite(result.RetainedPenaltySeconds) &&
            std::isfinite(result.RetainedMeasuredSeconds) &&
            std::isfinite(result.RetainedClockExcludingPenaltySeconds) &&
            std::abs(result.RetainedMeasuredSeconds -
                (result.RetainedPenaltySeconds +
                 result.RetainedClockExcludingPenaltySeconds)) <= tolerance &&
            result.RetainedPenaltySeconds <= total + tolerance &&
            (!result.CoversEntireRace ||
             (std::abs(result.RetainedPenaltySeconds - total) <= tolerance &&
              std::abs(result.RetainedMeasuredSeconds -
                       race.GetElapsedSeconds()) <= tolerance));
        if (!consistent)
            result.CompletePenaltyAttribution = false;
    }
    if (!result.CompletePenaltyAttribution)
    {
        // Unknown is not zero. Keep the original observed lap rows, but
        // withhold misleading partial aggregate attribution.
        result.PenaltyFreeLaps = 0;
        result.PenalizedLaps = 0;
        result.RetainedPenaltySeconds = 0.0;
        result.RetainedClockExcludingPenaltySeconds = 0.0;
        result.LargestPenaltyLap = 0;
        result.LargestPenaltySeconds = 0.0;
    }
    result.Count = count;
    result.Valid = true;
    return result;
}
} // namespace latebrake
