#pragma once

#include "Racing/RaceProgress.h"

#include <cmath>

namespace latebrake
{
// Both timing and checkpoint scoring must observe the same session state.
// Keeping this rule independent of Unreal makes pause/finish behavior
// directly testable and prevents overlap callbacks awarding a paused race.
inline bool CanScoreCheckpoint(
    const RaceProgress& race, bool circuitReady, bool worldPaused) noexcept
{
    return circuitReady && !worldPaused && !race.IsFinished();
}

// Gate the real shared RaceProgress clock on a complete circuit. This
// function has no Unreal dependencies and makes no claim about actual
// installed-game frame time or platform scheduling.
inline bool AdvanceRaceClockIfReady(
    RaceProgress& race, bool circuitReady,
    bool worldPaused, double deltaSeconds) noexcept
{
    if (!circuitReady || worldPaused || race.IsFinished() ||
        !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0)
    {
        return false;
    }

    const double before = race.GetElapsedSeconds();
    race.Advance(deltaSeconds);
    // A mathematically positive increment might be unrepresentable
    // at an extreme clock; do not report progress that never happened.
    return race.GetElapsedSeconds() > before;
}
} // namespace latebrake
