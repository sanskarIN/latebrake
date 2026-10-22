#include "Racing/RaceProgress.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
RaceProgress::RaceProgress(int checkpointCount, int requiredLaps) noexcept
    : CheckpointCount(std::clamp(checkpointCount, 1, MaxCheckpoints))
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
    SlowestLapSeconds = 0.0;
    LastSectorSeconds = 0.0;
    LastSectorDeltaSeconds = 0.0;
    LastSectorIndex = -1;
    LastSectorCompared = false;
    BestSectorSeconds.fill(0.0);
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

bool RaceProgress::AddTimePenalty(double penaltySeconds) noexcept
{
    if (State != RaceState::Running || !std::isfinite(penaltySeconds) ||
        penaltySeconds <= 0.0 ||
        penaltySeconds > std::numeric_limits<double>::max() - ElapsedSeconds)
    {
        return false;
    }

    ElapsedSeconds += penaltySeconds;
    return true;
}

bool RaceProgress::PassCheckpoint(int checkpointIndex) noexcept
{
    if (State != RaceState::Running || checkpointIndex != NextCheckpoint)
    {
        return false;
    }

    LastSectorSeconds = ElapsedSeconds - LastCheckpointSeconds;
    LastSectorIndex = checkpointIndex;
    LastSectorCompared = false;
    LastSectorDeltaSeconds = 0.0;
    // Use only strictly positive measured splits. A gate may be touched at
    // t=0, but that cannot become a zero-second unbeatable sector record.
    if (std::isfinite(LastSectorSeconds) && LastSectorSeconds > 0.0)
    {
        double& best = BestSectorSeconds[static_cast<std::size_t>(checkpointIndex)];
        if (best > 0.0)
        {
            LastSectorCompared = true;
            LastSectorDeltaSeconds = LastSectorSeconds - best;
        }
        if (best <= 0.0 || LastSectorSeconds < best)
        {
            best = LastSectorSeconds;
        }
    }
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
        if (LastLapSeconds > SlowestLapSeconds)
        {
            SlowestLapSeconds = LastLapSeconds;
        }
        LapStartSeconds = ElapsedSeconds;
        if (CompletedLaps >= RequiredLaps)
        {
            State = RaceState::Finished;
        }
    }

    return true;
}

double RaceProgress::GetBestSectorSeconds(int sector) const noexcept
{
    if (sector < 0 || sector >= CheckpointCount) return 0.0;
    return BestSectorSeconds[static_cast<std::size_t>(sector)];
}

double RaceProgress::GetTheoreticalBestLapSeconds() const noexcept
{
    double sum = 0.0;
    for (int sector = 0; sector < CheckpointCount; ++sector)
    {
        const double best = BestSectorSeconds[static_cast<std::size_t>(sector)];
        if (!std::isfinite(best) || best <= 0.0 ||
            best > std::numeric_limits<double>::max() - sum)
        {
            return 0.0;
        }
        sum += best;
    }
    return sum;
}
} // namespace latebrake
