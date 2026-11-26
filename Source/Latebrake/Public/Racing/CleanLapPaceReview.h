#pragma once

#include "Racing/RecentLapReview.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace latebrake
{
// Results-only comparison of genuinely completed, fee-free retained laps.
// Do not subtract penalties to invent a new personal best. The race's
// authoritative clock, profile, rewards and best lap remain unchanged.
struct CleanLapPaceReview final
{
    bool Valid = false;
    bool CoversEntireRace = false;
    bool HasVariation = false;
    bool HasAdjacentComparison = false;
    int RetainedLaps = 0;
    int CleanLaps = 0;
    int PenalizedLaps = 0;
    int FastestLap = 0;
    int SlowestLap = 0;
    int NewestCleanLap = 0;
    int AdjacentNewerLap = 0;
    int AdjacentOlderLap = 0;
    double FastestSeconds = 0.0;
    double SlowestSeconds = 0.0;
    double MeanSeconds = 0.0;
    double MedianSeconds = 0.0;
    double MedianAbsoluteDeviationSeconds = 0.0;
    double NewestCleanSeconds = 0.0;
    double NewestVsMedianSeconds = 0.0;
    double AdjacentDeltaSeconds = 0.0;
};

namespace clean_lap_detail
{
using Samples = std::array<double, RecentTimedLaps::Capacity>;

// A fixed-size insertion sort is deterministic on every supported host.
// Caller provides 1..Capacity finite positive durations or deviations.
inline double Median(Samples values, int count) noexcept
{
    for (int i = 1; i < count; ++i)
    {
        const double value = values[static_cast<std::size_t>(i)];
        int j = i;
        while (j > 0 && values[static_cast<std::size_t>(j - 1)] > value)
        {
            values[static_cast<std::size_t>(j)] =
                values[static_cast<std::size_t>(j - 1)];
            --j;
        }
        values[static_cast<std::size_t>(j)] = value;
    }
    const auto middle = static_cast<std::size_t>(count / 2);
    return count % 2 == 0
        ? (values[middle - 1] + values[middle]) * 0.5
        : values[middle];
}
} // namespace clean_lap_detail

inline CleanLapPaceReview BuildCleanLapPaceReview(
    const RaceProgress& race) noexcept
{
    CleanLapPaceReview result;
    const RecentLapReview review = BuildRecentLapReview(race);
    if (!review.Valid || !review.CompletePenaltyAttribution ||
        review.Count < 1 || review.Count > RecentTimedLaps::Capacity ||
        review.Count != std::min(race.GetCompletedLaps(),
                                  RecentTimedLaps::Capacity))
    {
        return result;
    }

    clean_lap_detail::Samples clean{};
    double sum = 0.0;
    int expected = race.GetCompletedLaps();
    bool previousWasClean = false;
    double previousCleanSeconds = 0.0;
    for (int offset = 0; offset < review.Count; ++offset)
    {
        const auto& row = review.Rows[static_cast<std::size_t>(offset)];
        const auto lap = race.GetRecentTimedLap(offset);
        if (row.CompletedLap != expected || lap.CompletedLap != expected ||
            !lap.ActiveClockEvidence || !row.PenaltyAttributionKnown ||
            !IsValidFinishedRaceTime(row.LapSeconds) ||
            !std::isfinite(row.IncludedPenaltySeconds) ||
            row.IncludedPenaltySeconds < 0.0 ||
            row.IncludedPenaltySeconds > row.LapSeconds)
        {
            return {};
        }
        --expected;
        if (row.IncludedPenaltySeconds != 0.0)
        {
            ++result.PenalizedLaps;
            previousWasClean = false;
            continue;
        }

        const double seconds = row.LapSeconds;
        if (result.CleanLaps == 0)
        {
            result.FastestLap = result.SlowestLap =
                result.NewestCleanLap = row.CompletedLap;
            result.FastestSeconds = result.SlowestSeconds =
                result.NewestCleanSeconds = seconds;
        }
        // Newest-first iteration makes equal-duration ties deterministic.
        if (seconds < result.FastestSeconds)
        {
            result.FastestSeconds = seconds;
            result.FastestLap = row.CompletedLap;
        }
        if (seconds > result.SlowestSeconds)
        {
            result.SlowestSeconds = seconds;
            result.SlowestLap = row.CompletedLap;
        }
        if (previousWasClean && !result.HasAdjacentComparison)
        {
            result.HasAdjacentComparison = true;
            result.AdjacentNewerLap = row.CompletedLap + 1;
            result.AdjacentOlderLap = row.CompletedLap;
            result.AdjacentDeltaSeconds = previousCleanSeconds - seconds;
        }
        previousWasClean = true;
        previousCleanSeconds = seconds;
        clean[static_cast<std::size_t>(result.CleanLaps++)] = seconds;
        sum += seconds;
    }

    if (result.CleanLaps == 0 ||
        result.CleanLaps != review.PenaltyFreeLaps ||
        result.PenalizedLaps != review.PenalizedLaps ||
        result.CleanLaps + result.PenalizedLaps != review.Count)
    {
        return {};
    }
    result.RetainedLaps = review.Count;
    result.CoversEntireRace = review.CoversEntireRace;
    result.MeanSeconds = sum / static_cast<double>(result.CleanLaps);
    result.MedianSeconds = clean_lap_detail::Median(clean, result.CleanLaps);
    result.NewestVsMedianSeconds =
        result.NewestCleanSeconds - result.MedianSeconds;
    if (result.CleanLaps >= 2)
    {
        clean_lap_detail::Samples deviations{};
        for (int i = 0; i < result.CleanLaps; ++i)
        {
            const auto index = static_cast<std::size_t>(i);
            deviations[index] = std::abs(clean[index] - result.MedianSeconds);
        }
        result.MedianAbsoluteDeviationSeconds =
            clean_lap_detail::Median(deviations, result.CleanLaps);
        result.HasVariation = true;
    }
    if (!std::isfinite(result.MeanSeconds) ||
        !std::isfinite(result.MedianSeconds) ||
        !std::isfinite(result.NewestVsMedianSeconds) ||
        !std::isfinite(result.MedianAbsoluteDeviationSeconds) ||
        !std::isfinite(result.AdjacentDeltaSeconds))
    {
        return {};
    }
    result.Valid = true;
    return result;
}
} // namespace latebrake
