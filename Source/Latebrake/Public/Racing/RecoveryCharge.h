#pragma once

#include "Racing/RaceProgress.h"

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
} // namespace latebrake
