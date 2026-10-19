#include "Racing/RaceProgress.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using latebrake::RaceProgress;
    using latebrake::RaceState;

    RaceProgress race(4, 3);
    Check(race.GetState() == RaceState::Running, "new race starts running");
    Check(race.GetCompletedLaps() == 0, "new race has zero completed laps");
    Check(!race.PassCheckpoint(1), "cannot bypass first checkpoint");
    Check(!race.PassCheckpoint(-1), "reject negative checkpoint");
    Check(!race.PassCheckpoint(4), "reject out-of-range checkpoint");
    Check(race.PassCheckpoint(0), "accept first checkpoint");
    Check(!race.PassCheckpoint(0), "duplicate checkpoint ignored");
    Check(race.GetNextCheckpoint() == 1, "next checkpoint increments");

    race.Advance(1.25);
    race.Advance(-1.0);
    race.Advance(std::numeric_limits<double>::infinity());
    race.Advance(std::numeric_limits<double>::quiet_NaN());
    Check(std::abs(race.GetElapsedSeconds() - 1.25) < 1e-9, "timer rejects invalid deltas");

    for (int lap = 0; lap < 3; ++lap)
    {
        for (int checkpoint = lap == 0 ? 1 : 0; checkpoint < 4; ++checkpoint)
        {
            Check(race.PassCheckpoint(checkpoint), "accept ordered checkpoint");
        }
        Check(race.GetCompletedLaps() == lap + 1, "completed lap count advances");
        Check(race.IsFinished() == (lap == 2), "finish only on required lap");
    }

    const double finishTime = race.GetElapsedSeconds();
    race.Advance(20.0);
    Check(race.GetElapsedSeconds() == finishTime, "finished race clock is frozen");
    Check(!race.PassCheckpoint(0), "finished race rejects extra checkpoints");
    race.Reset();
    Check(race.GetElapsedSeconds() == 0.0, "reset clock");
    Check(race.GetNextCheckpoint() == 0, "reset checkpoint");
    Check(!race.IsFinished(), "reset finishes");
    Check(race.GetCompletedLaps() == 0, "reset lap count");

    RaceProgress single(0, 0);
    Check(single.GetCheckpointCount() == 1, "clamp invalid checkpoint count");
    Check(single.GetRequiredLaps() == 1, "clamp invalid lap count");
    Check(single.PassCheckpoint(0), "single checkpoint race passes");
    Check(single.IsFinished(), "single checkpoint race finishes");

    std::cout << "All Latebrake portable race tests passed.\n";
    return EXIT_SUCCESS;
}
