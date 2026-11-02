#pragma once

#include "Racing/RaceStartLights.h"

namespace latebrake
{
// Cosmetic render state derived from the ONE authoritative RaceStartLights.
// The signal never controls the start gate, chassis forces, clock or rewards.
struct StartGantryVisual final
{
    bool Active = false;
    int RedBulbs = 0;
    bool GreenBulb = false;
};

// "Three" means three lit reds remaining, "Two" two, "One" one.
// GO is only the existing 0.85-second green announcement; subsequently all
// lamps are dark. Interrupted/unverified sessions never display false GO.
constexpr StartGantryVisual PlanStartGantryVisual(
    RaceStartPhase phase, bool circuitReady,
    bool playerAndGridReady, bool suspended, bool raceFinished) noexcept
{
    if (!circuitReady || !playerAndGridReady || suspended || raceFinished)
        return {};

    switch (phase)
    {
    case RaceStartPhase::Three: return {true, 3, false};
    case RaceStartPhase::Two: return {true, 2, false};
    case RaceStartPhase::One: return {true, 1, false};
    case RaceStartPhase::Go: return {true, 0, true};
    case RaceStartPhase::Racing: return {true, 0, false};
    default: return {};
    }
}
} // namespace latebrake
