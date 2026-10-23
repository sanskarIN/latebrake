#pragma once

#include "Racing/RaceClockReadiness.h"

namespace latebrake
{
// A gate callback cannot score on behalf of a missing local player.
inline bool CanScorePlayerCheckpoint(
    const RaceProgress& race, bool circuitReady,
    bool worldPaused, bool playerPawnReady) noexcept
{
    return playerPawnReady &&
           CanScoreCheckpoint(race, circuitReady, worldPaused);
}

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
