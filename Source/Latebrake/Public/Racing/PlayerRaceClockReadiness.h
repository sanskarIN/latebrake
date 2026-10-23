#pragma once

#include "Racing/RaceClockReadiness.h"

namespace latebrake
{
// A complete circuit alone does not establish an active player race.
// Unreal supplies this boolean after looking up its current local player
// pawn; no Unreal types or platform dependencies are needed in the policy.
inline bool AdvancePlayerRaceClockIfReady(
    RaceProgress& race, bool circuitReady, bool worldPaused,
    bool playerPawnReady, double deltaSeconds) noexcept
{
    return playerPawnReady &&
           AdvanceRaceClockIfReady(race, circuitReady, worldPaused, deltaSeconds);
}
} // namespace latebrake
