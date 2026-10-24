#pragma once

#include <array>
#include "Racing/RecentTimedLaps.h"

// Portable, engine-independent race progression logic.
// This header is deliberately free of Unreal dependencies so the same rules
// can be tested with a standard C++20 compiler in GitHub Actions.
namespace latebrake
{
enum class RaceState
{
    Running,
    Finished
};

class RaceProgress final
{
public:
    // Fixed-capacity split storage avoids allocations inside portable/Unreal race ticks.
    static constexpr int MaxCheckpoints = 256;

    explicit RaceProgress(int checkpointCount = 4, int requiredLaps = 3) noexcept;

    void Reset() noexcept;
    void Advance(double deltaSeconds) noexcept;
    // Only accepts valid positive penalties while racing (e.g. recovery).
    bool AddTimePenalty(double penaltySeconds) noexcept;
    bool PassCheckpoint(int checkpointIndex) noexcept;

    int GetCheckpointCount() const noexcept { return CheckpointCount; }
    int GetRequiredLaps() const noexcept { return RequiredLaps; }
    int GetNextCheckpoint() const noexcept { return NextCheckpoint; }
    int GetCompletedLaps() const noexcept { return CompletedLaps; }
    // Count laps with a strictly positive finite measured duration. An
    // immediate t=0 checkpoint pass is not evidence of a timed lap.
    int GetTimedLapCount() const noexcept { return TimedLapCount; }
    // Allocation-free last eight positive lap durations. The returned lap
    // number is the physical completed-lap index (not a best-lap rank).
    TimedLapRecord GetRecentTimedLap(int offset) const noexcept
    {
        return RecentLaps.GetRecent(offset);
    }
    int GetRecentTimedLapCount() const noexcept { return RecentLaps.Size(); }
    double GetElapsedSeconds() const noexcept { return ElapsedSeconds; }
    // Count only successfully charged time penalties; independent of the
    // caller's optional recovery telemetry. Penalties remain part of lap/split time.
    int GetTimePenaltyCount() const noexcept { return TimePenaltyCount; }
    double GetTotalPenaltySeconds() const noexcept { return TotalPenaltySeconds; }
    double GetCurrentLapSeconds() const noexcept { return ElapsedSeconds - LapStartSeconds; }
    double GetLastLapSeconds() const noexcept { return LastLapSeconds; }
    double GetBestLapSeconds() const noexcept { return BestLapSeconds; }
    double GetSlowestLapSeconds() const noexcept { return SlowestLapSeconds; }
    double GetLastSectorSeconds() const noexcept { return LastSectorSeconds; }
    double GetCurrentSectorSeconds() const noexcept { return ElapsedSeconds - LastCheckpointSeconds; }
    int GetLastSectorIndex() const noexcept { return LastSectorIndex; }
    // Negative delta means the last split beat the previous best for that
    // exact sector, NOT another gate or a prior session.
    bool HasLastSectorComparison() const noexcept { return LastSectorCompared; }
    double GetLastSectorDeltaSeconds() const noexcept { return LastSectorDeltaSeconds; }
    double GetBestSectorSeconds(int sector) const noexcept;
    // Sum of best observed valid splits in THIS run only. Returns 0 until
    // every sector was timed; this is not a persistent all-time record.
    double GetTheoreticalBestLapSeconds() const noexcept;
    RaceState GetState() const noexcept { return State; }
    bool IsFinished() const noexcept { return State == RaceState::Finished; }

private:
    int CheckpointCount = 4;
    int RequiredLaps = 3;
    int NextCheckpoint = 0;
    int CompletedLaps = 0;
    int TimedLapCount = 0;
    RecentTimedLaps RecentLaps;
    double ElapsedSeconds = 0.0;
    int TimePenaltyCount = 0;
    double TotalPenaltySeconds = 0.0;
    double LapStartSeconds = 0.0;
    double LastCheckpointSeconds = 0.0;
    double LastLapSeconds = 0.0;
    double BestLapSeconds = 0.0;
    double SlowestLapSeconds = 0.0;
    double LastSectorSeconds = 0.0;
    double LastSectorDeltaSeconds = 0.0;
    int LastSectorIndex = -1;
    bool LastSectorCompared = false;
    std::array<double, MaxCheckpoints> BestSectorSeconds{};
    RaceState State = RaceState::Running;
};
} // namespace latebrake
