#pragma once

#include "Track/CircuitMap.h"

namespace latebrake
{
// These are advisory HUD-only projections; never use them for gate
// validation, race classification, AI waypoints or race rewards.
struct RadarMarker final
{
    bool Valid = false;
    double U = 0.0;
    double V = 0.0;
};

struct PlayerRadarMarker final
{
    RadarMarker Position;
    // North-up SCREEN direction. The Y axis points downward.
    double DirectionX = 1.0;
    double DirectionY = 0.0;
};

struct RaceMiniMapSnapshot final
{
    PlayerRadarMarker Player;
    RadarMarker Rival;
    RadarMarker NextGate;
    bool DirectRivalSeparationValid = false;
    double DirectRivalSeparationMeters = 0.0;
};

RaceMiniMapSnapshot BuildRaceMiniMapSnapshot(
    CircuitPoint playerPosition, double playerHeadingDegrees,
    bool playerReady, int nextGate, bool rivalEnabled,
    CircuitPoint rivalPosition, bool rivalReady) noexcept;
} // namespace latebrake
