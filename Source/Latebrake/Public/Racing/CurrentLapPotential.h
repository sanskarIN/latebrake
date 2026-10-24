#pragma once

#include "Progression/RecordValidation.h"
#include "Racing/RaceProgress.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// An informational lower-bound-style lap estimate: the time already spent
// on this lap plus the best positive sector splits observed in THIS race.
// For the sector being driven, elapsed time cannot be undone; only the
// unspent portion of its observed best is included. This is not a ghost,
// an official result, a guaranteed achievable lap, or a SaveGame record.
struct CurrentLapPotential final
{
    bool Valid = false;
    int RemainingSectors = 0;
    double CurrentLapSeconds = 0.0;
    double PotentialLapSeconds = 0.0;
    double DeltaToBestLapSeconds = 0.0; // Negative = potential faster.
};

inline CurrentLapPotential EstimateCurrentLapPotential(
    const RaceProgress& race) noexcept
{
    CurrentLapPotential result;
    if (race.IsFinished() || race.GetTimedLapCount() < 1 ||
        race.GetCompletedLaps() >= race.GetRequiredLaps() ||
        !IsValidFinishedRaceTime(race.GetElapsedSeconds()) ||
        !IsValidFinishedRaceTime(race.GetBestLapSeconds()))
    {
        return result;
    }

    const int next = race.GetNextCheckpoint();
    const int gates = race.GetCheckpointCount();
    const double currentLap = race.GetCurrentLapSeconds();
    const double currentSector = race.GetCurrentSectorSeconds();
    if (next < 0 || next >= gates ||
        !std::isfinite(currentLap) || currentLap < 0.0 ||
        !std::isfinite(currentSector) || currentSector < 0.0 ||
        currentSector > currentLap)
    {
        return result;
    }

    // Require a real reference for every as-yet-unfinished sector. A timed
    // lap may contain a zero-duration gate split: never fabricate its best.
    constexpr double MaxSeconds = 7.0 * 24.0 * 60.0 * 60.0;
    double estimate = currentLap;
    for (int sector = next; sector < gates; ++sector)
    {
        const double best = race.GetBestSectorSeconds(sector);
        if (!IsValidFinishedRaceTime(best))
        {
            return result;
        }
        const double remaining = sector == next
            ? std::max(0.0, best - currentSector) : best;
        if (!std::isfinite(remaining) || remaining > MaxSeconds - estimate)
        {
            return result;
        }
        estimate += remaining;
    }
    if (!IsValidFinishedRaceTime(estimate) || estimate < currentLap)
    {
        return result;
    }

    const double delta = estimate - race.GetBestLapSeconds();
    if (!std::isfinite(delta))
    {
        return result;
    }
    result.Valid = true;
    result.RemainingSectors = gates - next;
    result.CurrentLapSeconds = currentLap;
    result.PotentialLapSeconds = estimate;
    result.DeltaToBestLapSeconds = delta;
    return result;
}
} // namespace latebrake
