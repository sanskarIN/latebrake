#include "Racing/ManualRacePauseGuard.h"
#include "Racing/AppFocusRaceGuard.h"
#include "Racing/CheckpointSessionTrace.h"
#include "Racing/PlayerRaceClockReadiness.h"
#include "Racing/RecoveryCharge.h"
#include "Racing/RaceSessionEpoch.h"
#include "Vehicles/PlayerDriveReadiness.h"
#include "AI/RivalDriveReadiness.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool yes, const char* label)
{
    if (!yes)
    {
        std::cerr << "Manual race PAUSE FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    RaceProgress race(4, 3);
    ManualRacePauseGuard pause;
    AppFocusRaceGuard focus;
    RaceSessionEpoch epoch;
    CheckpointSessionTrace gate;
    constexpr bool trackReady = true, driverReady = true;
    const auto position = CircuitGate(0);
    const CircuitGateSample approach{{position.X - 90., position.Y}, 150.};
    const CircuitGateSample crossed{{position.X + 90., position.Y}, 150.};

    Require(!pause.IsSuspended(), "start with no manual latch");
    Require(AdvancePlayerRaceClockIfReady(race, trackReady, false,
                driverReady, 1.0), "normal race clock advances");
    Require(!gate.SampleDetailed(0, 0, epoch.Current(),
                approach, 300.0, 0.0).Crossed(),
            "pre-pause checkpoint sample establishes baseline");

    // Unreal rejects the requested pause, continuing to tick the world.
    // Independent manual intent still blocks every gameplay consumer.
    const bool requestedPause = pause.OnToggleRequested(false);
    Require(requestedPause && pause.IsSuspended(),
            "first manual PAUSE latches before engine call");
    const bool actualUnrealPaused = false; // simulated failed SetGamePaused
    epoch.OnPauseTransition();
    const double frozenAt = race.GetElapsedSeconds();
    const bool inactive = actualUnrealPaused || pause.IsSuspended() ||
                          focus.IsSuspended();
    Require(!AdvancePlayerRaceClockIfReady(
                race, trackReady, inactive, driverReady, 30.0) &&
            race.GetElapsedSeconds() == frozenAt,
            "failed Unreal manual pause must freeze clock");
    Require(!CanScorePlayerCheckpoint(race, trackReady, inactive, driverReady),
            "failed manual pause must reject checkpoints");
    int recoveries = 0;
    Require(!TryChargePlayerRecoveryIfReady(race, recoveries, trackReady,
                inactive, driverReady) &&
            recoveries == 0 && race.GetElapsedSeconds() == frozenAt,
            "failed manual pause must deny paid recoveries atomically");
    Require(!CanDriveRival(race, EventKind::Circuit, trackReady,
                inactive, driverReady, false),
            "rival cannot continue driving while manual pause is latched");
    Require(!CanApplyPlayerDrive({true, trackReady, false,
                actualUnrealPaused || pause.IsSuspended(), false, false}),
            "player force/torque are denied while world ignores pause");
    Require(!gate.SampleDetailed(0, 0, epoch.Current(),
                crossed, 300.0, 0.0).Crossed(),
            "pre-pause checkpoint position cannot score after failed pause");

    // A fresh deliberate PAUSE press resumes without incorrectly toggling
    // an already unpaused world into paused state.
    Require(!pause.OnToggleRequested(false) && !pause.IsSuspended(),
            "second PAUSE clears independent fallback and requests unpause");
    epoch.OnPauseTransition();
    Require(!gate.SampleDetailed(0, 0, epoch.Current(),
                approach, 300.0, 0.0).Crossed(),
            "resume establishes a new gate baseline");
    Require(gate.SampleDetailed(0, 0, epoch.Current(),
                crossed, 300.0, 0.0).Crossed(),
            "fresh forward crossing is still accepted");
    Require(AdvancePlayerRaceClockIfReady(race, true, false, true, 2.0),
            "clock resumes after explicit manual resume");
    Require(CanApplyPlayerDrive({true, true, false, false, false, false}),
            "player can drive after legitimate resumed race");

    // If engine PAUSE succeeds, the same fallback latch still protects
    // against a surprise external automatic world unpause.
    Require(pause.OnToggleRequested(false) && pause.IsSuspended(),
            "second pause cycle latches");
    Require(!CanApplyPlayerDrive({true, true, false,
                pause.IsSuspended(), false, false}),
            "surprise world unpause still leaves player input gated");
    Require(pause.IsSuspended(), "OS world unpause cannot clear manual intent");
    Require(!pause.OnToggleRequested(true) && !pause.IsSuspended(),
            "unpause requests clear the latch");
    Require(!CanScorePlayerCheckpoint(race, true, true, true),
            "failed engine UNPAUSE still blocks via world paused flag");
    Require(!pause.OnToggleRequested(true) && !pause.IsSuspended(),
            "repeated request while engine remains paused retries unpause");

    // An explicit menu return/restart may clear manual intent. A passive
    // background/foreground event has no API for clearing it.
    Require(pause.OnToggleRequested(false), "third pause cycle");
    pause.OnExplicitResume();
    Require(!pause.IsSuspended(), "explicit menu return releases fallback");
    Require(pause.OnToggleRequested(false), "pause after menu return");
    pause.OnExplicitResume();
    race.Reset();
    epoch.OnRaceRestart();
    Require(!pause.IsSuspended() && race.GetElapsedSeconds() == 0.,
            "accepted race restart removes old pause state");

    // Focus-loss latch is independent: resuming manual PAUSE alone must
    // never silently unlock focus-loss without the explicit GameMode action.
    Require(focus.OnFocusLost(race, false, false) ==
                FocusLossTransition::SuspendedNeedsWorldPause,
            "focus suspension can occur independently");
    Require(pause.OnToggleRequested(false), "manual PAUSE still works while focus latched");
    pause.OnExplicitResume();
    Require(focus.IsSuspended(),
            "manual guard reset alone never unlocks app focus guard");

    std::cout << "Manual race pause fallback simulation passed (NO Unreal).\n";
    return EXIT_SUCCESS;
}
