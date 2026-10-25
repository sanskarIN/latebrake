#include "Track/CircuitRouteAdvisor.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace latebrake
{
CircuitRouteAdvice AdviseCircuitRoute(
    int nextGate, CircuitPoint position,
    double headingDegrees, bool eligible) noexcept
{
    if (!eligible || !IsCircuitGate(nextGate) ||
        !std::isfinite(position.X) || !std::isfinite(position.Y) ||
        !std::isfinite(headingDegrees) || !IsOnPavedSurface(position))
    {
        return {};
    }
    const CircuitPoint target = CircuitGate(nextGate);
    const double dx = target.X - position.X;
    const double dy = target.Y - position.Y;
    const double distance = std::hypot(dx, dy);
    if (!std::isfinite(distance) || distance > 15000.0)
    {
        return {};
    }
    // A near-zero target vector has no meaningful bearing.
    if (distance <= 50.0)
    {
        return {true, distance / 100.0, 0.0, RouteHint::Straight, true};
    }
    const double desired = std::atan2(dy, dx);
    const double heading = std::remainder(
        headingDegrees, 360.0) * std::numbers::pi / 180.0;
    const double error = std::atan2(
        std::sin(desired - heading),
        std::cos(desired - heading));
    if (!std::isfinite(error))
    {
        return {};
    }
    const double absolute = std::abs(error);
    RouteHint hint = RouteHint::Straight;
    if (absolute > 2.45)
    {
        hint = RouteHint::TurnAround;
    }
    else if (absolute > 0.28)
    {
        hint = error > 0.0 ? RouteHint::Left : RouteHint::Right;
    }
    // Direct range deliberately makes no promised lap distance or ETA.
    return {true, distance / 100.0,
            error * 180.0 / std::numbers::pi, hint,
            distance <= 1800.0};
}
} // namespace latebrake
