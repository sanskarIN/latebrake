#include "Vehicles/PlayerDriveReadiness.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "Player driving readiness FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    constexpr PlayerDriveConditions ready{
        true, true, false, false, false, false, true
    };
    static_assert(CanApplyPlayerDrive(ready));
    // The original six-value aggregate is deliberately rejected: forgetting
    // to forward GameMode's grid authorization cannot silently enable torque.
    constexpr PlayerDriveConditions omittedPlacement{
        true, true, false, false, false, false
    };
    static_assert(!CanApplyPlayerDrive(omittedPlacement));
    static_assert(EvaluatePlayerDrive(omittedPlacement) ==
                  PlayerDriveDecision::RestartPlacementUnverified);
    static_assert(!CanApplyPlayerDrive(PlayerDriveConditions{}));
    Require(!CanApplyPlayerDrive(omittedPlacement),
            "omitted seventh bit denies even an otherwise drivable pawn");
    Require(EvaluatePlayerDrive(ready) == PlayerDriveDecision::Allowed,
            "active track, authoritative mode and running race drive normally");

    // Every closed boundary must independently stop propulsion even when
    // SetGamePaused has FAILED and the engine keeps ticking the pawn.
    constexpr PlayerDriveConditions noMode{
        false, true, false, false, false, false, true
    };
    constexpr PlayerDriveConditions missingCircuit{
        true, false, false, false, false, false, true
    };
    constexpr PlayerDriveConditions menuFailedPause{
        true, true, true, false, false, false, true
    };
    constexpr PlayerDriveConditions worldPaused{
        true, true, false, true, false, false, true
    };
    constexpr PlayerDriveConditions osFocusFailedPause{
        true, true, false, false, true, false, true
    };
    constexpr PlayerDriveConditions finished{
        true, true, false, false, false, true, true
    };
    static_assert(!CanApplyPlayerDrive(noMode));
    static_assert(!CanApplyPlayerDrive(missingCircuit));
    static_assert(!CanApplyPlayerDrive(menuFailedPause));
    static_assert(!CanApplyPlayerDrive(worldPaused));
    static_assert(!CanApplyPlayerDrive(osFocusFailedPause));
    static_assert(!CanApplyPlayerDrive(finished));

    Require(EvaluatePlayerDrive(noMode) ==
                PlayerDriveDecision::NoAuthoritativeMode,
            "orphan pawn cannot drive without authoritative GameMode");
    Require(EvaluatePlayerDrive(missingCircuit) ==
                PlayerDriveDecision::CircuitNotReady,
            "missing road/collision/gates cannot enable driving");
    Require(EvaluatePlayerDrive(menuFailedPause) ==
                PlayerDriveDecision::MenuOpen,
            "opened menu stops motion even when Unreal world pause fails");
    Require(EvaluatePlayerDrive(worldPaused) ==
                PlayerDriveDecision::EnginePaused,
            "externally paused world cannot drive");
    Require(EvaluatePlayerDrive(osFocusFailedPause) ==
                PlayerDriveDecision::FocusSuspended,
            "OS focus loss stops motion without engine world pause");
    Require(EvaluatePlayerDrive(finished) ==
                PlayerDriveDecision::RaceFinished,
            "finish cannot keep accelerating while clock is frozen");

    // Source v0.5.51 requires callers to explicitly supply the seventh
    // authorization bit. A six-field initializer is fail-closed.
    auto rejectedRestart = ready;
    rejectedRestart.RestartPlacementReady = false;
    Require(!CanApplyPlayerDrive(rejectedRestart) &&
            EvaluatePlayerDrive(rejectedRestart) ==
                PlayerDriveDecision::RestartPlacementUnverified,
            "healthy physics with rejected grid placement cannot drive");
    rejectedRestart.RestartPlacementReady = true;
    Require(CanApplyPlayerDrive(rejectedRestart),
            "verified restart placement can restore driving authorization");
    for (int combination = 0; combination < 128; ++combination)
    {
        PlayerDriveConditions state{
            (combination & 1) != 0, (combination & 2) != 0,
            (combination & 4) != 0, (combination & 8) != 0,
            (combination & 16) != 0, (combination & 32) != 0,
            (combination & 64) != 0
        };
        const bool allowed = combination == 67;
        Require(CanApplyPlayerDrive(state) == allowed,
                "complete seven-bit driving matrix must fail closed");
    }

    // Exhaustively ensure that every single blocked condition and every
    // combination of blocked states fails closed; only fully ready drives.
    for (int combination = 0; combination < 64; ++combination)
    {
        const PlayerDriveConditions conditions{
            (combination & 1) != 0,
            (combination & 2) != 0,
            (combination & 4) != 0,
            (combination & 8) != 0,
            (combination & 16) != 0,
            (combination & 32) != 0,
            true // explicitly verified initial spawn/grid
        };
        const bool expected = combination == 3;
        Require(CanApplyPlayerDrive(conditions) == expected,
                "every one of the 64 readiness combinations matches policy");
        Require((EvaluatePlayerDrive(conditions) == PlayerDriveDecision::Allowed)
                    == expected,
                "typed decisions and boolean policy must agree");
    }

    // Returning to an active event should re-enable propulsion; policy
    // itself has no sticky latch. GameMode owns explicit OS/manual resume.
    PlayerDriveConditions resume = menuFailedPause;
    resume.MenuOpen = false;
    Require(CanApplyPlayerDrive(resume),
            "explicit menu closure restores race-driving eligibility");
    resume = osFocusFailedPause;
    resume.FocusSuspended = false;
    Require(CanApplyPlayerDrive(resume),
            "explicit focus guard release restores race-driving eligibility");
    resume = finished;
    resume.RaceFinished = false;
    Require(CanApplyPlayerDrive(resume),
            "authoritative race restart restores driving eligibility");

    std::cout << "All player driving readiness states passed.\n";
    return EXIT_SUCCESS;
}
