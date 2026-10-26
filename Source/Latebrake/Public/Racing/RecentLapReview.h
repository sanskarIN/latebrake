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
};

struct RecentLapReview final
{
    bool Valid = false;
    int Count = 0;
    std::array<RecentLapReviewRow, RecentTimedLaps::Capacity> Rows{};
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
        }
        result.Rows[offset] = row;
    }
    result.Count = count;
    result.Valid = true;
    return result;
}
} // namespace latebrake
