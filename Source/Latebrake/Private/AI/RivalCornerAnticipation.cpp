#include "AI/RivalCornerAnticipation.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
PilotCommand AnticipateRivalCorner(
    PilotCommand commanded, int nextGate,
    CircuitPoint carPosition, double planarSpeedCmPerSecond) noexcept
{
    if (!IsCircuitGate(nextGate) ||
        !std::isfinite(carPosition.X) ||
        !std::isfinite(carPosition.Y) ||
        !std::isfinite(planarSpeedCmPerSecond) ||
        planarSpeedCmPerSecond < 0.0 ||
        !std::isfinite(commanded.Throttle) ||
        !std::isfinite(commanded.Steering) ||
        std::abs(commanded.Throttle) > 1.0 ||
        std::abs(commanded.Steering) > 1.0)
    {
        // Malformed route or simulation state cannot authorize
        // continued AI motor acceleration.
        return {};
    }
    // Preserve existing early stop and active braking decisions.
    if (commanded.Arrived || commanded.Brake)
    {
        return commanded;
    }
    const CircuitPoint corner = CircuitGate(nextGate);
    const double distance = std::hypot(
        corner.X - carPosition.X, corner.Y - carPosition.Y);
    if (!std::isfinite(distance) || distance > 2200.0 ||
        distance < 440.0)
    {
        return commanded;
    }
    // Original four-gate loop: every next gate precedes a right-angle
    // corner. Its successor is the turn exit, not an invented AI shortcut.
    const CircuitPoint next = CircuitGate(
        (nextGate + 1) % CircuitGateCount);
    const CircuitPoint prev = CircuitGate(
        (nextGate + CircuitGateCount - 1) % CircuitGateCount);
    const double ax = corner.X - prev.X;
    const double ay = corner.Y - prev.Y;
    const double bx = next.X - corner.X;
    const double by = next.Y - corner.Y;
    const double aLength = std::hypot(ax, ay);
    const double bLength = std::hypot(bx, by);
    if (!(aLength > 0.0 && bLength > 0.0))
    {
        return {};
    }
    const double dot = (ax * bx + ay * by) /
                       (aLength * bLength);
    if (!std::isfinite(dot))
    {
        return {};
    }
    if (dot > 0.86) // straight-through route, no corner demand
    {
        return commanded;
    }

    // Target decreases continuously from ~2300 to ~1300 cm/s. The
    // existing PlanRivalTurn decides actual steering and arrival.
    const double targetSpeed = std::clamp(
        1100.0 + 0.55 * distance, 1200.0, 2350.0);
    if (planarSpeedCmPerSecond > targetSpeed + 160.0)
    {
        commanded.Throttle = 0.0;
        commanded.Brake = true;
    }
    return commanded;
}
} // namespace latebrake
