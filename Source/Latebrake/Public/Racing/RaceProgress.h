#pragma once

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
    double GetElapsedSeconds() const noexcept { return ElapsedSeconds; }
    double GetCurrentLapSeconds() const noexcept { return ElapsedSeconds - LapStartSeconds; }
    double GetLastLapSeconds() const noexcept { return LastLapSeconds; }
    double GetBestLapSeconds() const noexcept { return BestLapSeconds; }
    double GetSlowestLapSeconds() const noexcept { return SlowestLapSeconds; }
    double GetLastSectorSeconds() const noexcept { return LastSectorSeconds; }
    RaceState GetState() const noexcept { return State; }
    bool IsFinished() const noexcept { return State == RaceState::Finished; }

private:
    int CheckpointCount = 4;
    int RequiredLaps = 3;
    int NextCheckpoint = 0;
    int CompletedLaps = 0;
    double ElapsedSeconds = 0.0;
    double LapStartSeconds = 0.0;
    double LastCheckpointSeconds = 0.0;
    double LastLapSeconds = 0.0;
    double BestLapSeconds = 0.0;
    double SlowestLapSeconds = 0.0;
    double LastSectorSeconds = 0.0;
    RaceState State = RaceState::Running;
};
} // namespace latebrake
