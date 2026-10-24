#pragma once

#include "Progression/RecordValidation.h"
#include "Racing/RaceProgress.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Measured, within-run feedback for the currently driven sector. This is
// NOT a split prediction or a record: the existing best is an already
// completed positive split on this same sector. Never compare unlike gates.
struct LiveSectorTarget final
{
    bool Valid = false;
    int SectorIndex = -1;
    double ElapsedSectorSeconds = 0.0;
    double BestSectorSeconds = 0.0;
    double SecondsUntilBest = 0.0;
    double SecondsOverBest = 0.0;
    bool OverBest = false;
};

inline LiveSectorTarget MeasureLiveSectorTarget(
    const RaceProgress& race) noexcept
{
    LiveSectorTarget result;
    if (race.IsFinished() || race.GetTimedLapCount() < 1 ||
        !IsValidFinishedRaceTime(race.GetElapsedSeconds()) ||
        race.GetCompletedLaps() >= race.GetRequiredLaps())
    {
        return result;
    }
    const int sector = race.GetNextCheckpoint();
    if (sector < 0 || sector >= race.GetCheckpointCount())
    {
        return result;
    }
    const double elapsed = race.GetCurrentSectorSeconds();
    const double best = race.GetBestSectorSeconds(sector);
    if (!std::isfinite(elapsed) || elapsed < 0.0 ||
        elapsed > race.GetElapsedSeconds() || !IsValidFinishedRaceTime(best))
    {
        return result;
    }
    result.Valid = true;
    result.SectorIndex = sector;
    result.ElapsedSectorSeconds = elapsed;
    result.BestSectorSeconds = best;
    result.OverBest = elapsed > best;
    result.SecondsUntilBest = result.OverBest ? 0.0 : best - elapsed;
    result.SecondsOverBest = result.OverBest ? elapsed - best : 0.0;
    return result;
}
} // namespace latebrake
