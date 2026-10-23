#pragma once

#include "Racing/RaceProgress.h"
#include "Progression/RecordValidation.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Purely informational estimate based on up to three most recent completed
// timed laps. It is neither a best-lap claim nor a race reward/record.
// Do not display this without a complete, valid timed lap.
struct LiveRacePaceProjection final
{
    bool Valid = false;
    int SampledLaps = 0;
    int LapsRemaining = 0;
    double ReferenceLapSeconds = 0.0;
    double ProjectedFinishSeconds = 0.0;
};

inline LiveRacePaceProjection ProjectLiveRacePace(
    const RaceProgress& race) noexcept
{
    LiveRacePaceProjection result;
    if (race.IsFinished() || race.GetCompletedLaps() < 1 ||
        race.GetCompletedLaps() >= race.GetRequiredLaps() ||
        !IsValidFinishedRaceTime(race.GetElapsedSeconds()))
    {
        return result;
    }

    constexpr int Window = 3;
    const int samples = std::min(Window, race.GetRecentTimedLapCount());
    if (samples < 1)
    {
        return result;
    }

    double sum = 0.0;
    for (int index = 0; index < samples; ++index)
    {
        const TimedLapRecord lap = race.GetRecentTimedLap(index);
        if (!lap.Valid() || !IsValidFinishedRaceTime(lap.Seconds) ||
            lap.Seconds > 604800.0 - sum)
        {
            return result;
        }
        sum += lap.Seconds;
    }

    const double reference = sum / static_cast<double>(samples);
    const double elapsed = race.GetElapsedSeconds();
    const double current = race.GetCurrentLapSeconds();
    const int remaining = race.GetRequiredLaps() - race.GetCompletedLaps();
    if (!IsValidFinishedRaceTime(reference) ||
        !std::isfinite(current) || current < 0.0 || remaining <= 0)
    {
        return result;
    }

    // The current lap may already be slower than the reference. In that case,
    // project NO negative time to finish the lap: never move a projected
    // absolute race finish backwards into the past.
    const double currentRemainder = std::max(0.0, reference - current);
    const double futureLaps = static_cast<double>(remaining - 1);
    const double estimatedRemaining = currentRemainder + futureLaps * reference;
    if (!std::isfinite(estimatedRemaining) ||
        estimatedRemaining < 0.0 ||
        estimatedRemaining > 604800.0 - elapsed)
    {
        return result;
    }
    const double predicted = elapsed + estimatedRemaining;
    if (!IsValidFinishedRaceTime(predicted))
    {
        return result;
    }

    result.Valid = true;
    result.SampledLaps = samples;
    result.LapsRemaining = remaining;
    result.ReferenceLapSeconds = reference;
    result.ProjectedFinishSeconds = predicted;
    return result;
}
} // namespace latebrake
