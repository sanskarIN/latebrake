#include "Racing/RaceProgress.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
RaceProgress::RaceProgress(int checkpointCount, int requiredLaps) noexcept
    : CheckpointCount(std::max(1, checkpointCount))
    , RequiredLaps(std::max(1, requiredLaps))
{
    Reset();
}

void RaceProgress::Reset() noexcept
{
    NextCheckpoint = 0;
    CompletedLaps = 0;
    ElapsedSeconds = 0.0;
    LapStartSeconds = 0.0;
    LastCheckpointSeconds = 0.0;
    LastLapSeconds = 0.0;
    BestLapSeconds = 0.0;
    LastSectorSeconds = 0.0;
    State = RaceState::Running;
}

void RaceProgress::Advance(double deltaSeconds) noexcept
{
    // Avoid corrupting the clock from engine pause/physics anomalies.
    if (State == RaceState::Running && std::isfinite(deltaSeconds) && deltaSeconds > 0.0 &&
        deltaSeconds <= std::numeric_limits<double>::max() - ElapsedSeconds)
    {
        ElapsedSeconds += deltaSeconds;
    }
}

bool RaceProgress::PassCheckpoint(int checkpointIndex) noexcept
{
    if (State != RaceState::Running || checkpointIndex != NextCheckpoint)
    {
        return false;
    }

    LastSectorSeconds = ElapsedSeconds - LastCheckpointSeconds;
    LastCheckpointSeconds = ElapsedSeconds;
    ++NextCheckpoint;
    if (NextCheckpoint == CheckpointCount)
    {
        NextCheckpoint = 0;
        ++CompletedLaps;
        LastLapSeconds = ElapsedSeconds - LapStartSeconds;
        if (LastLapSeconds > 0.0 &&
            (BestLapSeconds <= 0.0 || LastLapSeconds < BestLapSeconds))
        {
            BestLapSeconds = LastLapSeconds;
        }
        LapStartSeconds = ElapsedSeconds;
        if (CompletedLaps >= RequiredLaps)
        {
            State = RaceState::Finished;
        }
    }

    return true;
}
} // namespace latebrake
