#pragma once

#include "Racing/RaceProgress.h"
#include "Racing/PlayerRaceClockReadiness.h"

#include <limits>
#include <utility>

namespace latebrake
{
// Caller MUST invoke this before any recovery teleport. It returns false
// without changing the count when the race is over, the clock cannot accept
// the entire five-second penalty or the counter cannot advance.
// The ordinary RaceProgress clock (and its current sector) includes the fee.
inline constexpr double RecoveryPenaltySeconds = 5.0;

inline bool TryChargePlayerRecovery(RaceProgress& race,
                                    int& recoveries) noexcept
{
    if (recoveries < 0 ||
        recoveries == std::numeric_limits<int>::max() ||
        !race.AddTimePenalty(RecoveryPenaltySeconds))
    {
        return false;
    }
    ++recoveries;
    return true;
}
 
// A recovery is a race action, not a pause/menu escape hatch. Share the
// actual checkpoint and timer readiness policy so a failed circuit build,
// missing local driver, pause/menu or completed result cannot spend credits
// of race time or teleport a car while competitive timing is suspended.
// The existing charge remains atomic with respect to the recovery counter
// and clock: the caller MUST teleport only on true.
inline bool TryChargePlayerRecoveryIfReady(
    RaceProgress& race, int& recoveries, bool circuitReady,
    bool pausedOrInMenu, bool localDriverReady) noexcept
{
    return CanScorePlayerCheckpoint(
               race, circuitReady, pausedOrInMenu, localDriverReady) &&
           TryChargePlayerRecovery(race, recoveries);
}

// The Unreal move is fallible. Compute the exact five-second charge on an
// independent fixed-capacity RaceProgress snapshot FIRST; commit clock,
// penalty telemetry and recovery count only if the physical placement was
// actually verified. This is a synchronous game-thread transaction; no
// allocations, exceptions, or speculative modifications to the live clock.
//
// The callback must only attempt/verify a vehicle teleport. It must not
// itself modify RaceProgress or the recovery counter.
template <typename TryVerifiedTeleport>
bool TryChargePlayerRecoveryAfterVerifiedTeleport(
    RaceProgress& race, int& recoveries, bool circuitReady,
    bool pausedOrInMenu, bool localDriverReady,
    TryVerifiedTeleport&& tryVerifiedTeleport)
{
    if (!CanScorePlayerCheckpoint(
            race, circuitReady, pausedOrInMenu, localDriverReady))
    {
        return false;
    }

    RaceProgress pendingRace = race;
    int pendingRecoveries = recoveries;
    if (!TryChargePlayerRecovery(pendingRace, pendingRecoveries))
    {
        return false;
    }
    if (!static_cast<bool>(
            std::forward<TryVerifiedTeleport>(tryVerifiedTeleport)()))
    {
        // Engine refused/misplaced vehicle: refund the entire tentative
        // clock charge by never writing it to the live race state.
        return false;
    }

    race = pendingRace;
    recoveries = pendingRecoveries;
    return true;
}

} // namespace latebrake
