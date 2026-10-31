#pragma once

#include "Track/CircuitLayout.h"

namespace latebrake
{
// Geometry-only rival steering aim for the fixed, original four-gate loop.
// The actual checkpoint stays at CircuitGate(index): this point is NEVER
// used for awarding laps, recovery placements, timing or player driving.
//
// A physical rival now needs a *directional crossing* to progress. The
// old exact-gate aim plus a 440cm proximity-arrival rule could stop the
// motor before crossing. Aim 550cm THROUGH the gate along the incoming
// straight, inside the circuit's paved 800cm half-width rectangles.
inline constexpr double RivalGateRunThroughDistanceCm = 550.0;

constexpr CircuitPoint RivalGateRunThroughPoint(int nextGate) noexcept
{
    if (!IsCircuitGate(nextGate))
    {
        return {}; // caller must reject invalid gate indexes
    }
    const CircuitPoint gate = CircuitGate(nextGate);
    switch (nextGate)
    {
    case 0: return {gate.X + RivalGateRunThroughDistanceCm, gate.Y};
    case 1: return {gate.X, gate.Y + RivalGateRunThroughDistanceCm};
    case 2: return {gate.X - RivalGateRunThroughDistanceCm, gate.Y};
    case 3: return {gate.X, gate.Y - RivalGateRunThroughDistanceCm};
    default: return {};
    }
}
} // namespace latebrake
