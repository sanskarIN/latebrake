#pragma once

#include "AI/RivalPilot.h"
#include "Track/CircuitLayout.h"

namespace latebrake
{
// Modifies only rival throttle/brake for a known approaching 90-degree
// circuit corner. Does NOT award rival laps or bypass route recovery.
PilotCommand AnticipateRivalCorner(
    PilotCommand commanded, int nextGate,
    CircuitPoint carPosition, double planarSpeedCmPerSecond) noexcept;
} // namespace latebrake
