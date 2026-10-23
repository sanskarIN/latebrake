#pragma once

#include "Racing/RaceProgress.h"
#include "Racing/PlayerRaceClockReadiness.h"

#include <limits>

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

} // namespace latebrake
