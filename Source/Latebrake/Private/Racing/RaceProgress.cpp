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
    TimedLapCount = 0;
    RecentLaps.Reset();
    ElapsedSeconds = 0.0;
    TimePenaltyCount = 0;
    TotalPenaltySeconds = 0.0;
    LapPenaltyStartSeconds = 0.0;
    LastLapPenaltySeconds = 0.0;
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
        TimePenaltyCount == std::numeric_limits<int>::max() ||
        penaltySeconds > std::numeric_limits<double>::max() - ElapsedSeconds)
    {
        return false;
    }

    // IEEE-754 doubles lose sub-second and eventually whole-second
    // precision as the clock grows. A mathematically positive fee can
    // round to zero (or undercharge) at extreme elapsed values. Never
    // authorize RECOVER unless the clock actually advances by at least
    // the requested number of seconds.
    const double updated = ElapsedSeconds + penaltySeconds;
    if (!std::isfinite(updated) || updated - ElapsedSeconds < penaltySeconds)
    {
        return false;
    }
    // Account for the ACTUAL representable increment (which can round up),
    // and reject a charge whose audit total cannot represent that increment.
    // This is an all-or-nothing transaction: rejected charges never change
    // the race clock, count or telemetry, and cannot authorize a teleport.
    const double charged = updated - ElapsedSeconds;
    if (charged > std::numeric_limits<double>::max() - TotalPenaltySeconds)
    {
        return false;
    }
    const double newTotal = TotalPenaltySeconds + charged;
    if (!std::isfinite(newTotal) ||
        newTotal - TotalPenaltySeconds < charged || newTotal > updated)
    {
        return false;
    }
    ElapsedSeconds = updated;
    TotalPenaltySeconds = newTotal;
    ++TimePenaltyCount;
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
        // Snapshot penalty provenance at the same ordered finish gate as
        // the measured lap: the next lap begins at zero charged fees.
        LastLapPenaltySeconds = TotalPenaltySeconds - LapPenaltyStartSeconds;
        LapPenaltyStartSeconds = TotalPenaltySeconds;
        if (std::isfinite(LastLapSeconds) && LastLapSeconds > 0.0)
        {
            // Existing best/slowest semantics stay unchanged for legitimate
            // lap times, while the explicit count ignores zero-duration
            // laps which would otherwise make pace spread misleading.
            ++TimedLapCount;
            RecentLaps.Record(CompletedLaps, LastLapSeconds);
            if (BestLapSeconds <= 0.0 || LastLapSeconds < BestLapSeconds)
            {
                BestLapSeconds = LastLapSeconds;
            }
            if (LastLapSeconds > SlowestLapSeconds)
            {
                SlowestLapSeconds = LastLapSeconds;
            }
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
