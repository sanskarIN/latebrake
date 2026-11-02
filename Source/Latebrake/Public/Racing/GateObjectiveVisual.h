#pragma once

#include "Track/CircuitLayout.h"

namespace latebrake
{
// Read-only cosmetic plan. Never grants a checkpoint, touches a save,
// changes vehicle forces, or implies that a real UE map is cooked.
struct GateObjectiveVisual final
{
    bool Visible = false;
    bool Highlighted = false;
    double RadiusScale = 0.0;
    double HeightScale = 0.0;
};

constexpr GateObjectiveVisual PlanGateObjectiveVisual(
    int gateIndex, int nextCheckpoint, int checkpointCount,
    bool circuitReady, bool raceFinished) noexcept
{
    // Invalid or incomplete map configurations must never display a
    // misleading active objective. The official circuit has four gates.
    if (!circuitReady || raceFinished || checkpointCount != CircuitGateCount ||
        gateIndex < 0 || gateIndex >= checkpointCount ||
        nextCheckpoint < 0 || nextCheckpoint >= checkpointCount)
    {
        return {};
    }
    const bool selected = gateIndex == nextCheckpoint;
    // The other three gates remain spatially visible for navigation.
    return {true, selected, selected ? 14.0 : 8.0,
            selected ? 0.16 : 0.08};
}
} // namespace latebrake
