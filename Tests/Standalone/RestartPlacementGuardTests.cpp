#include "Racing/RestartPlacementGuard.h"
#include "Racing/PlayerRaceClockReadiness.h"
#include "Racing/RecoveryCharge.h"
#include "Vehicles/PlayerDriveReadiness.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool yes, const char* reason)
{
    if (!yes)
    {
        std::cerr << "Restart placement FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    RestartPlacementGuard gate;
    RaceProgress race(4, 1);
    int recoveries = 0;

    const auto Advance = [&](double seconds) {
        return AdvancePlayerRaceClockIfReady(
            race, true, false, gate.CanRace(), seconds);
    };
    const auto CanPass = [&] {
        return CanScorePlayerCheckpoint(race, true, false, gate.CanRace());
    };
    const auto CanDrive = [&] {
        return CanApplyPlayerDrive({true, true, false, false, false,
                                    race.IsFinished(), gate.CanRace()});
    };
    const auto TryRecover = [&] {
        bool callbackCalled = false;
        const bool charged = TryChargePlayerRecoveryAfterVerifiedTeleport(
            race, recoveries, true, false, gate.CanRace(),
            [&] { callbackCalled = true; return true; });
        return charged || callbackCalled;
    };

    Require(gate.CanRace() &&
            gate.CurrentState() == RestartPlacementGuard::State::InitialSpawn,
            "initial engine spawn is eligible subject to physical pawn readiness");
    Require(!gate.OnRestartPlacementResult(true) && gate.CanRace(),
            "unsolicited result cannot authorize a new restart");
    Require(Advance(3.0) && CanPass() &&
            race.GetElapsedSeconds() == 3.0,
            "initial valid spawn can time a real race");

    gate.OnRestartRequested();
    Require(!gate.CanRace() &&
            gate.CurrentState() == RestartPlacementGuard::State::AwaitingPlacement,
            "new restart invalidates prior authorization before teleport");
    Require(!CanDrive() &&
            !Advance(10.0) && !CanPass() && !TryRecover() &&
            race.GetElapsedSeconds() == 3.0 && recoveries == 0,
            "unresolved grid placement blocks clock gate and paid recovery");
    Require(!gate.OnRestartPlacementResult(false) &&
            !gate.CanRace() &&
            gate.CurrentState() == RestartPlacementGuard::State::RejectedPlacement,
            "rejected Unreal move latches denial");
    Require(!gate.OnRestartPlacementResult(true) && !gate.CanRace(),
            "late unsolicited success cannot bypass an earlier rejection");
    Require(!Advance(30.0) && !CanPass() && !TryRecover() &&
            race.GetElapsedSeconds() == 3.0 && recoveries == 0,
            "a healthy pre-restart chassis cannot keep earning time or fees");

    gate.OnRestartRequested();
    Require(!gate.CanRace() &&
            gate.CurrentState() == RestartPlacementGuard::State::AwaitingPlacement,
            "explicit retry starts a new guarded attempt");
    Require(gate.OnRestartPlacementResult(true) && gate.CanRace() &&
            gate.CurrentState() == RestartPlacementGuard::State::VerifiedPlacement,
            "verified retry restores eligibility");
    Require(!gate.OnRestartPlacementResult(false) && gate.CanRace(),
            "late rejection cannot override a verified result");
    Require(CanDrive() && Advance(2.0) && CanPass() &&
            race.GetElapsedSeconds() == 5.0,
            "only active time after a verified retry accrues");

    gate.OnRestartRequested();
    gate.OnRestartRequested();
    Require(!gate.CanRace() &&
            gate.CurrentState() == RestartPlacementGuard::State::AwaitingPlacement,
            "another restart invalidates previously verified authorization");
    Require(!gate.OnRestartPlacementResult(false) &&
            !gate.CanRace(), "repeated restart failure stays denied");
    gate.OnRestartRequested();
    Require(gate.OnRestartPlacementResult(true) && gate.CanRace(),
            "new explicit restart can repair failed placement");

    std::cout << "Restart placement gate tests passed (no Unreal).\n";
    return EXIT_SUCCESS;
}
