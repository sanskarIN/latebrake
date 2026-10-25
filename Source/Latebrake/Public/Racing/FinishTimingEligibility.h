#pragma once

#include "Progression/RecordValidation.h"
#include "Racing/RaceProgress.h"

namespace latebrake
{
// Eligibility is deliberately separate from RaceProgress::IsFinished():
// ordered gate completion is a fact of the run, but awards and persistent
// records additionally require recordable times AND an active clock sample
// on every completed lap. Penalties never masquerade as driving time.
enum class FinishTimingEligibility
{
    Eligible,
    RaceIncomplete,
    MissingActiveLapTiming,
    UnrecordableClock
};

inline FinishTimingEligibility ValidateFinishTiming(
    const RaceProgress& race) noexcept
{
    const int laps = race.GetCompletedLaps();
    if (!race.IsFinished() || laps != race.GetRequiredLaps() || laps <= 0)
    {
        return FinishTimingEligibility::RaceIncomplete;
    }
    if (race.GetActiveTimedLapCount() != laps ||
        race.GetTimedLapCount() != laps)
    {
        return FinishTimingEligibility::MissingActiveLapTiming;
    }
    const double elapsed = race.GetElapsedSeconds();
    const double best = race.GetBestLapSeconds();
    const double last = race.GetLastLapSeconds();
    const double slowest = race.GetSlowestLapSeconds();
    if (!IsValidFinishedRaceTime(elapsed) ||
        !IsValidFinishedRaceTime(best) ||
        !IsValidFinishedRaceTime(last) ||
        !IsValidFinishedRaceTime(slowest) ||
        best > slowest || slowest > elapsed || last > elapsed)
    {
        return FinishTimingEligibility::UnrecordableClock;
    }
    return FinishTimingEligibility::Eligible;
}

inline bool CanAwardFinishedRace(const RaceProgress& race) noexcept
{
    return ValidateFinishTiming(race) == FinishTimingEligibility::Eligible;
}
} // namespace latebrake
