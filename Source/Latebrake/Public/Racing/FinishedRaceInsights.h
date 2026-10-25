#pragma once

#include "Progression/RecordValidation.h"
#include "Racing/RaceProgress.h"
#include "Racing/FinishTimingEligibility.h"

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
    int LatestTimedLapNumber = 0;
    double LatestTimedLapSeconds = 0.0;
    int PreviousTimedLapNumber = 0;
    double PreviousTimedLapSeconds = 0.0;
    // Positive = last timed lap slower, negative = faster.
    double LatestVsPreviousDeltaSeconds = 0.0;
    bool HasLatestVsPrevious = false;
};

inline FinishedRaceInsights BuildFinishedRaceInsights(
    const RaceProgress& race) noexcept
{
    FinishedRaceInsights result;
    // A completed gate sequence is not necessarily an awardable finish:
    // invalid/zero-time and penalty-only laps cannot become a polished
    // results card while the authoritative reward path refuses them.
    if (!CanAwardFinishedRace(race))
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

    const TimedLapRecord latest = race.GetRecentTimedLap(0);
    const TimedLapRecord previous = race.GetRecentTimedLap(1);
    if (latest.Valid() && IsValidFinishedRaceTime(latest.Seconds))
    {
        result.LatestTimedLapNumber = latest.CompletedLap;
        result.LatestTimedLapSeconds = latest.Seconds;
        if (previous.Valid() &&
            IsValidFinishedRaceTime(previous.Seconds) &&
            previous.CompletedLap < latest.CompletedLap)
        {
            result.PreviousTimedLapNumber = previous.CompletedLap;
            result.PreviousTimedLapSeconds = previous.Seconds;
            result.LatestVsPreviousDeltaSeconds =
                latest.Seconds - previous.Seconds;
            result.HasLatestVsPrevious = true;
        }
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
