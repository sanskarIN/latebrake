#pragma once

#include "Track/CircuitLayout.h"

namespace latebrake
{
// A guidance-only readout of direct distance to the NEXT authoritative
// checkpoint. Never substitutes for directional swept checkpoint scoring.
enum class RouteHint
{
    Unavailable, Straight, Left, Right, TurnAround
};

struct CircuitRouteAdvice final
{
    bool Valid = false;
    double DirectDistanceMeters = 0.0;
    double HeadingErrorDegrees = 0.0;
    RouteHint Hint = RouteHint::Unavailable;
    bool Approaching = false;
};

CircuitRouteAdvice AdviseCircuitRoute(
    int nextGate, CircuitPoint position,
    double headingDegrees, bool eligible) noexcept;
} // namespace latebrake
