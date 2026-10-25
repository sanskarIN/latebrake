#include "Track/RaceMiniMap.h"

#include <cmath>
#include <numbers>

namespace latebrake
{
namespace
{
bool CanShow(CircuitPoint position) noexcept
{
    // Existing CircuitMap clamps out-of-range positions. For *racer*
    // markers that would invent a location at the map edge. Suppress
    // instead until the physical actor returns within visible bounds.
    return std::isfinite(position.X) && std::isfinite(position.Y) &&
           position.X >= -4200.0 && position.X <= 4200.0 &&
           position.Y >= -3000.0 && position.Y <= 3000.0;
}

RadarMarker MakeMarker(CircuitPoint position) noexcept
{
    if (!CanShow(position))
    {
        return {};
    }
    const CircuitMapPoint point = NormalizeCircuitMap(position);
    return {true, point.U, point.V};
}
} // namespace

RaceMiniMapSnapshot BuildRaceMiniMapSnapshot(
    CircuitPoint playerPosition, double playerHeadingDegrees,
    bool playerReady, int nextGate, bool rivalEnabled,
    CircuitPoint rivalPosition, bool rivalReady) noexcept
{
    RaceMiniMapSnapshot result;
    if (IsCircuitGate(nextGate))
    {
        result.NextGate = MakeMarker(CircuitGate(nextGate));
    }
    if (!playerReady || !std::isfinite(playerHeadingDegrees))
    {
        return result;
    }
    result.Player.Position = MakeMarker(playerPosition);
    if (!result.Player.Position.Valid)
    {
        return result;
    }
    const double yaw = std::remainder(playerHeadingDegrees, 360.0);
    const double radians = yaw * std::numbers::pi / 180.0;
    result.Player.DirectionX = std::cos(radians);
    result.Player.DirectionY = -std::sin(radians);
    if (!rivalEnabled || !rivalReady)
    {
        return result;
    }
    result.Rival = MakeMarker(rivalPosition);
    if (!result.Rival.Valid)
    {
        return result;
    }
    const double distanceCm = std::hypot(
        rivalPosition.X - playerPosition.X,
        rivalPosition.Y - playerPosition.Y);
    if (std::isfinite(distanceCm))
    {
        result.DirectRivalSeparationValid = true;
        result.DirectRivalSeparationMeters = distanceCm / 100.0;
    }
    return result;
}
} // namespace latebrake
