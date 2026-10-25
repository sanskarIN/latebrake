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
    // Pure policy mirror of Unreal's verified, generation-aware weak actor
    // identity comparison. A replacement pawn can be fully simulated but
    // is NOT the one that passed the last explicit grid teleport.
    bool sameVerifiedPawn = true;

    const auto Advance = [&](double seconds) {
        return AdvancePlayerRaceClockIfReady(
            race, true, false, gate.CanRace(sameVerifiedPawn), seconds);
    };
    const auto CanPass = [&] {
        return CanScorePlayerCheckpoint(race, true, false, gate.CanRace(sameVerifiedPawn));
    };
    const auto CanDrive = [&] {
        return CanApplyPlayerDrive({true, true, false, false, false,
                                    race.IsFinished(), gate.CanRace(sameVerifiedPawn)});
    };
    const auto TryRecover = [&] {
        bool callbackCalled = false;
        const bool charged = TryChargePlayerRecoveryAfterVerifiedTeleport(
            race, recoveries, true, false, gate.CanRace(sameVerifiedPawn),
            [&] { callbackCalled = true; return true; });
        return charged || callbackCalled;
    };

    Require(gate.CanRace(sameVerifiedPawn) &&
            gate.CurrentState() == RestartPlacementGuard::State::InitialSpawn,
            "initial engine spawn is eligible subject to physical pawn readiness");
    Require(!gate.OnRestartPlacementResult(true) && gate.CanRace(sameVerifiedPawn),
            "unsolicited result cannot authorize a new restart");
    Require(Advance(3.0) && CanPass() &&
            race.GetElapsedSeconds() == 3.0,
            "initial valid spawn can time a real race");

    gate.OnRestartRequested();
    Require(!gate.CanRace(sameVerifiedPawn) &&
            gate.CurrentState() == RestartPlacementGuard::State::AwaitingPlacement,
            "new restart invalidates prior authorization before teleport");
    Require(!CanDrive() &&
            !Advance(10.0) && !CanPass() && !TryRecover() &&
            race.GetElapsedSeconds() == 3.0 && recoveries == 0,
            "unresolved grid placement blocks clock gate and paid recovery");
    Require(!gate.OnRestartPlacementResult(false) &&
            !gate.CanRace(sameVerifiedPawn) &&
            gate.CurrentState() == RestartPlacementGuard::State::RejectedPlacement,
            "rejected Unreal move latches denial");
    Require(!gate.OnRestartPlacementResult(true) && !gate.CanRace(sameVerifiedPawn),
            "late unsolicited success cannot bypass an earlier rejection");
    Require(!Advance(30.0) && !CanPass() && !TryRecover() &&
            race.GetElapsedSeconds() == 3.0 && recoveries == 0,
            "a healthy pre-restart chassis cannot keep earning time or fees");
    // The unguarded Unreal RECOVER path was using chassis readiness alone.
    // Model that bug explicitly: physical readiness remains true, but the
    // authoritative combined driver gate has to veto the staged teleport.
    constexpr bool healthyOldPositionChassis = true;
    bool attemptedPhysicsMove = false;
    const bool paidDespiteRejectedGrid = TryChargePlayerRecoveryAfterVerifiedTeleport(
        race, recoveries, true, false,
        healthyOldPositionChassis && gate.CanRace(sameVerifiedPawn),
        [&] { attemptedPhysicsMove = true; return true; });
    Require(!paidDespiteRejectedGrid && !attemptedPhysicsMove &&
            recoveries == 0 && race.GetElapsedSeconds() == 3.0,
            "failed grid denies even an otherwise healthy chassis recovery callback");

    gate.OnRestartRequested();
    Require(!gate.CanRace(sameVerifiedPawn) &&
            gate.CurrentState() == RestartPlacementGuard::State::AwaitingPlacement,
            "explicit retry starts a new guarded attempt");
    Require(gate.OnRestartPlacementResult(true) && gate.CanRace(sameVerifiedPawn) &&
            gate.CurrentState() == RestartPlacementGuard::State::VerifiedPlacement,
            "verified retry restores eligibility");
    Require(!gate.OnRestartPlacementResult(false) && gate.CanRace(sameVerifiedPawn),
            "late rejection cannot override a verified result");
    Require(CanDrive() && Advance(2.0) && CanPass() &&
            race.GetElapsedSeconds() == 5.0,
            "only active time after a verified retry accrues");
    Require(TryRecover() && recoveries == 1 &&
            race.GetElapsedSeconds() == 10.0,
            "verified restart allows exactly one physically acknowledged paid recovery");
    Require(CanDrive() && CanPass(),
            "paid recovery does not revoke a verified restart placement");
    const double verifiedClock = race.GetElapsedSeconds();
    const int verifiedRecoveries = recoveries;
    sameVerifiedPawn = false; // Unreal possessed a DIFFERENT physical pawn.
    Require(!gate.CanRace(sameVerifiedPawn) && !CanDrive() &&
            !Advance(120.0) && !CanPass() && !TryRecover() &&
            race.GetElapsedSeconds() == verifiedClock &&
            recoveries == verifiedRecoveries,
            "new healthy pawn cannot inherit old vehicle's verified restart");
    Require(gate.CurrentState() == RestartPlacementGuard::State::VerifiedPlacement,
            "losing possession does not forge a new teleport result");
    sameVerifiedPawn = true;
    Require(gate.CanRace(sameVerifiedPawn) && CanDrive() && CanPass(),
            "original verified pawn retains authorization if it is re-possessed");
    sameVerifiedPawn = false;
    gate.OnRestartRequested();
    Require(!gate.CanRace(sameVerifiedPawn) &&
            gate.OnRestartPlacementResult(true) &&
            !gate.CanRace(sameVerifiedPawn),
            "verified engine placement alone does not authorize another pawn");
    sameVerifiedPawn = true;
    Require(gate.CanRace(sameVerifiedPawn),
            "new pawn can race only after its own verified restart");


    gate.OnRestartRequested();
    gate.OnRestartRequested();
    Require(!gate.CanRace(sameVerifiedPawn) &&
            gate.CurrentState() == RestartPlacementGuard::State::AwaitingPlacement,
            "another restart invalidates previously verified authorization");
    Require(!gate.OnRestartPlacementResult(false) &&
            !gate.CanRace(sameVerifiedPawn), "repeated restart failure stays denied");
    gate.OnRestartRequested();
    Require(gate.OnRestartPlacementResult(true) && gate.CanRace(sameVerifiedPawn),
            "new explicit restart can repair failed placement");

    std::cout << "Restart placement gate tests passed (no Unreal).\n";
    return EXIT_SUCCESS;
}
