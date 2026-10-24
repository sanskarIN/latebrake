#include "Racing/AppFocusRaceGuard.h"
#include "Racing/CheckpointSessionTrace.h"
#include "Racing/PlayerRaceClockReadiness.h"
#include "Racing/RecoveryCharge.h"
#include "Racing/RaceSessionEpoch.h"
#include "AI/RivalDriveReadiness.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool yes, const char* label)
{
    if (!yes) { std::cerr << "App focus race FAIL: " << label << '\n'; std::exit(EXIT_FAILURE); }
}
}

int main()
{
    using namespace latebrake;
    RaceProgress race(4, 3);
    AppFocusRaceGuard guard;
    RaceSessionEpoch epoch;
    CheckpointSessionTrace trace;
    const auto point = CircuitGate(0);
    const CircuitGateSample approach{{point.X - 100., point.Y}, 150.};
    const CircuitGateSample crossed{{point.X + 100., point.Y}, 150.};
    const bool trackReady = true, driverReady = true;

    Require(!guard.IsSuspended(), "new session can race");
    Require(AdvancePlayerRaceClockIfReady(
        race, trackReady, false, driverReady, 1.0), "clock advances before interruption");
    Require(!trace.SampleDetailed(0, 1, epoch.Current(), approach, 300., 0.).Crossed(),
            "establish legitimate pre-pause gate baseline");

    // Simulate an OS world pause request failing: the portable guard is
    // still a hard clock / checkpoint / recovery / rival gate on its own.
    Require(guard.OnFocusLost(race, false, false), "first focus loss needs world pause");
    epoch.OnPauseTransition();
    Require(guard.IsSuspended(), "focus loss latches even if OS pause fails");
    const double stoppedAt = race.GetElapsedSeconds();
    Require(!AdvancePlayerRaceClockIfReady(
        race, trackReady, guard.IsSuspended(), driverReady, 10.0),
        "failed Unreal pause cannot charge player racing time");
    Require(race.GetElapsedSeconds() == stoppedAt, "clock remains frozen");
    Require(!CanScorePlayerCheckpoint(race, trackReady, guard.IsSuspended(), driverReady),
            "focus-lost checkpoint cannot score");
    int recoveries = 0;
    Require(!TryChargePlayerRecoveryIfReady(
        race, recoveries, trackReady, guard.IsSuspended(), driverReady),
        "focus-lost recovery cannot teleport or penalize");
    Require(recoveries == 0 && race.GetElapsedSeconds() == stoppedAt,
            "recovery rejection is atomic");
    Require(!CanDriveRival(race, EventKind::Circuit, trackReady,
                          guard.IsSuspended(), driverReady, false),
            "rival must stop on focus loss even without a paused world");

    Require(!guard.OnFocusLost(race, false, false),
            "duplicate deactivate/background event is idempotent");
    Require(guard.IsSuspended() && epoch.Current() == 1,
            "OS foreground observation does not auto-resume a race");
    Require(!trace.SampleDetailed(0, 1, epoch.Current(), crossed, 300., 0.).Crossed(),
            "post-loss position cannot bridge old checkpoint plane");

    guard.OnExplicitResume();
    epoch.OnPauseTransition();
    Require(!guard.IsSuspended(), "only explicit player resume clears latch");
    Require(AdvancePlayerRaceClockIfReady(
        race, trackReady, guard.IsSuspended(), driverReady, 2.0),
        "race clock resumes after explicit action");
    Require(!trace.SampleDetailed(0, 1, epoch.Current(), approach, 300., 0.).Crossed(),
            "resume starts a fresh motion epoch");
    Require(trace.SampleDetailed(0, 1, epoch.Current(), crossed, 300., 0.).Crossed(),
            "genuine post-resume crossing is possible");
    Require(race.PassCheckpoint(0) && race.GetNextCheckpoint() == 1,
            "real ordered progress still succeeds after resume");

    AppFocusRaceGuard alreadyPaused;
    Require(!alreadyPaused.OnFocusLost(race, false, true) &&
            alreadyPaused.IsSuspended(),
            "already paused world still latches against OS foreground auto-resume");
    Require(!AdvancePlayerRaceClockIfReady(
        race, true, alreadyPaused.IsSuspended(), true, 1.0),
            "OS auto-resuming a paused world cannot charge racing time");
    alreadyPaused.OnExplicitResume();
    Require(!alreadyPaused.IsSuspended(),
            "player can deliberately resume an originally paused race");
    Require(!alreadyPaused.OnFocusLost(race, true, false) &&
            !alreadyPaused.IsSuspended(),
            "already open menu retains its normal pause and input policy");

    RaceProgress finished(1, 1);
    finished.Advance(5.0);
    Require(finished.PassCheckpoint(0) && finished.IsFinished(),
            "completed result fixture");
    AppFocusRaceGuard results;
    Require(!results.OnFocusLost(finished, false, false) &&
            !results.IsSuspended(), "completed result does not get extra pause");
    Require(!AdvancePlayerRaceClockIfReady(finished, true, false, true, 1.0),
            "finished result remains frozen");

    std::cout << "App focus loss race integrity passed (portable simulation).\n";
    return EXIT_SUCCESS;
}
