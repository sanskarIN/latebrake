#pragma once

#include "Racing/FinishTimingEligibility.h"
#include "Racing/RecentTimedLaps.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace latebrake
{
// Pure post-race statistics for the last up to eight RETAINED timed laps,
// not all laps in an arbitrarily long event. No live steering, race awards,
// profile persistence, ghost data or allocations depend on this advice.
struct RecentLapPaceStatistics final
{
    bool Valid = false;
    int SampleCount = 0;
    int OldestLap = 0;
    int NewestLap = 0;
    double AverageSeconds = 0.0;
    double MedianSeconds = 0.0;
    double FastestSampleSeconds = 0.0;
    double SlowestSampleSeconds = 0.0;
    double PopulationDeviationSeconds = 0.0;
    double NewestVsAverageSeconds = 0.0; // Positive = newest slower.
    int ConsecutiveImprovements = 0;    // Most recent adjacent timed laps.
};

inline RecentLapPaceStatistics CalculateRecentLapPaceStatistics(
    const RaceProgress& race) noexcept
{
    RecentLapPaceStatistics result;
    if (!CanAwardFinishedRace(race))
    {
        return result;
    }
    const int count = race.GetRecentTimedLapCount();
    if (count < 2 || count > RecentTimedLaps::Capacity ||
        count != std::min(race.GetCompletedLaps(), RecentTimedLaps::Capacity))
    {
        return result;
    }

    std::array<double, RecentTimedLaps::Capacity> values{};
    double sum = 0.0;
    int previousLap = race.GetCompletedLaps() + 1;
    for (int index = 0; index < count; ++index)
    {
        const TimedLapRecord lap = race.GetRecentTimedLap(index);
        if (!lap.Valid() || !IsValidFinishedRaceTime(lap.Seconds) ||
            lap.CompletedLap != previousLap - 1)
        {
            return {};
        }
        previousLap = lap.CompletedLap;
        values[static_cast<std::size_t>(index)] = lap.Seconds;
        sum += lap.Seconds; // <= 8 * 604800 due to source validation.
    }
    std::sort(values.begin(), values.begin() + count);
    const double mean = sum / static_cast<double>(count);
    const double median = count % 2 == 0
        ? (values[static_cast<std::size_t>(count / 2 - 1)] +
           values[static_cast<std::size_t>(count / 2)]) / 2.0
        : values[static_cast<std::size_t>(count / 2)];
    double squared = 0.0;
    for (int index = 0; index < count; ++index)
    {
        const double diff = values[static_cast<std::size_t>(index)] - mean;
        squared += diff * diff;
    }
    const double deviation = std::sqrt(squared / static_cast<double>(count));
    const double latest = race.GetRecentTimedLap(0).Seconds;
    const double latestDelta = latest - mean;
    if (!std::isfinite(mean) || !std::isfinite(median) ||
        !std::isfinite(deviation) || !std::isfinite(latestDelta))
    {
        return {};
    }

    // Compare only adjacent physical lap IDs; ties do not count as faster.
    int streak = 0;
    for (int index = 0; index < count - 1; ++index)
    {
        const TimedLapRecord newer = race.GetRecentTimedLap(index);
        const TimedLapRecord older = race.GetRecentTimedLap(index + 1);
        if (newer.CompletedLap != older.CompletedLap + 1 ||
            newer.Seconds >= older.Seconds)
        {
            break;
        }
        ++streak;
    }
    result.Valid = true;
    result.SampleCount = count;
    result.OldestLap = previousLap;
    result.NewestLap = race.GetCompletedLaps();
    result.AverageSeconds = mean;
    result.MedianSeconds = median;
    result.FastestSampleSeconds = values[0];
    result.SlowestSampleSeconds = values[static_cast<std::size_t>(count - 1)];
    result.PopulationDeviationSeconds = deviation;
    result.NewestVsAverageSeconds = latestDelta;
    result.ConsecutiveImprovements = streak;
    return result;
}
} // namespace latebrake
