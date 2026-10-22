#pragma once

#include "Progression/RecordValidation.h"
#include "Racing/RaceProgress.h"

namespace latebrake
{
// Derived at results time from an actual run's portable race clock and splits.
// No engine objects, allocations, persistent saves or additional rewards.
struct FinishedRaceInsights final
{
    bool Valid = false;
    int CompletedLaps = 0;
    int TimedLaps = 0;
    double BestLapSeconds = 0.0;
    double LastLapSeconds = 0.0;
    double TheoreticalBestLapSeconds = 0.0;
    double BestToTheoreticalGapSeconds = 0.0;
    double LapSpreadSeconds = 0.0;
    bool HasTheoreticalGap = false;
    bool HasLapSpread = false;
};

inline FinishedRaceInsights BuildFinishedRaceInsights(
    const RaceProgress& race) noexcept
{
    FinishedRaceInsights result;
    // Invalid/clamped clocks are not results. Match the same seven-day
    // float-representability policy used for rewards and SaveGame records.
    if (!race.IsFinished() ||
        !IsValidFinishedRaceTime(race.GetElapsedSeconds()) ||
        !IsValidFinishedRaceTime(race.GetBestLapSeconds()))
    {
        return result;
    }

    result.Valid = true;
    result.CompletedLaps = race.GetCompletedLaps();
    result.TimedLaps = race.GetTimedLapCount();
    result.BestLapSeconds = race.GetBestLapSeconds();

    if (IsValidFinishedRaceTime(race.GetLastLapSeconds()))
    {
        result.LastLapSeconds = race.GetLastLapSeconds();
    }

    const double theoretical = race.GetTheoreticalBestLapSeconds();
    // A theoretical lap is a within-run sum of best valid sector splits,
    // not an achieved lap record. Reject inconsistent >best sums rather than
    // presenting a negative "room to improve" number.
    if (IsValidFinishedRaceTime(theoretical) &&
        theoretical <= result.BestLapSeconds)
    {
        result.TheoreticalBestLapSeconds = theoretical;
        result.BestToTheoreticalGapSeconds =
            result.BestLapSeconds - theoretical;
        result.HasTheoreticalGap = true;
    }

    const double slowest = race.GetSlowestLapSeconds();
    if (result.TimedLaps >= 2 &&
        IsValidFinishedRaceTime(slowest) &&
        slowest >= result.BestLapSeconds)
    {
        result.LapSpreadSeconds = slowest - result.BestLapSeconds;
        result.HasLapSpread = true;
    }

    return result;
}
} // namespace latebrake
