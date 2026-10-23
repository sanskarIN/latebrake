#include "Racing/RaceClockReadiness.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "Race clock readiness FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceProgress progress(1, 1);
    Require(!CanScoreCheckpoint(progress, false, false) &&
            !CanScoreCheckpoint(progress, false, true) &&
            !CanScoreCheckpoint(progress, true, true) &&
            CanScoreCheckpoint(progress, true, false),
            "only a ready, running, unpaused race can accept checkpoints");
    Require(!AdvanceRaceClockIfReady(progress, false, false, 20.0),
            "incomplete circuit is never a timed race");
    Require(progress.GetElapsedSeconds() == 0.0 &&
            progress.GetCompletedLaps() == 0,
            "blocked startup cannot silently accumulate elapsed time");
    Require(!AdvanceRaceClockIfReady(progress, false, true, 100.0) &&
            !AdvanceRaceClockIfReady(progress, true, true, 12.0),
            "paused or incomplete world cannot advance a race");
    Require(!AdvanceRaceClockIfReady(progress, true, false, 0.0) &&
            !AdvanceRaceClockIfReady(progress, true, false, -10.0) &&
            !AdvanceRaceClockIfReady(progress, true, false,
                std::numeric_limits<double>::infinity()) &&
            !AdvanceRaceClockIfReady(progress, true, false,
                std::numeric_limits<double>::quiet_NaN()),
            "malformed Unreal frame times fail closed");
    Require(progress.GetElapsedSeconds() == 0.0,
            "invalid advances never poison the clock");

    Require(AdvanceRaceClockIfReady(progress, true, false, 3.5),
            "only genuine ready, unpaused positive frame time advances");
    Require(progress.GetElapsedSeconds() == 3.5,
            "first real elapsed frame duration is precise");
    Require(!AdvanceRaceClockIfReady(progress, true, true, 5.0),
            "pause cannot add timing delta");
    Require(progress.GetElapsedSeconds() == 3.5,
            "pause preserves exact previous elapsed state");
    Require(AdvanceRaceClockIfReady(progress, true, false, 2.5) &&
            progress.GetElapsedSeconds() == 6.0,
            "resumption adds only actual active race frame time");

    Require(!CanScoreCheckpoint(progress, true, true) &&
            CanScoreCheckpoint(progress, true, false),
            "pause suppresses scoring without poisoning the running race");
    Require(progress.PassCheckpoint(0) && progress.IsFinished(),
            "one valid ordered finish uses real elapsed time");
    Require(!CanScoreCheckpoint(progress, true, false) &&
            !CanScoreCheckpoint(progress, false, false),
            "finished races never accept another checkpoint");
    Require(progress.GetBestLapSeconds() == 6.0,
            "a result is measured from active race time only");
    Require(!AdvanceRaceClockIfReady(progress, true, false, 100.0) &&
            progress.GetElapsedSeconds() == 6.0,
            "finished race clock never restarts on ticking");

    progress.Reset();
    Require(!progress.IsFinished() &&
            CanScoreCheckpoint(progress, true, false) &&
            !CanScoreCheckpoint(progress, false, false) &&
            progress.GetElapsedSeconds() == 0.0,
            "restart fully resets race time");
    Require(!AdvanceRaceClockIfReady(progress, false, false, 4.0) &&
            progress.GetElapsedSeconds() == 0.0,
            "restart cannot bypass a failed circuit startup");

    std::cout << "Race clock startup and pause contract passed.\n";
    return EXIT_SUCCESS;
}
