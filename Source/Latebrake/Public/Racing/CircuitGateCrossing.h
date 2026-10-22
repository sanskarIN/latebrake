#pragma once

#include "Racing/GateValidation.h"
#include "Track/CircuitLayout.h"

#include <cmath>

namespace latebrake
{
// Frame-to-frame samples used by the four fixed starter-circuit gates.
// This is a geometric *crossing* check, not a sphere overlap: touching
// the leading rim of a checkpoint may no longer award a sector.
struct CircuitGateSample final
{
    CircuitPoint XY{};
    double Z = 0.0;
};

inline constexpr double GatePlaneHalfLengthCm = 600.0;
inline constexpr double GateLateralHalfWidthCm = 500.0;
inline constexpr double GateCenterHeightCm = 38.0;
inline constexpr double GateVerticalHalfHeightCm = 380.0;
inline constexpr double GateMaxFrameTravelCm = 1200.0;

// The car must travel from the APPROACH side to the DEPARTURE side of
// the line, in the expected race direction, with the interpolated crossing
// point inside the physical checkpoint corridor and height window.
//
// Require both frame endpoints near the gate and a plausible displacement.
// This rejects long jumps/teleports in a single tick; it does not replace
// real physics or installed-game collision QA.
inline bool CrossesCircuitGatePlane(int gateIndex,
                                   CircuitGateSample previous,
                                   CircuitGateSample current,
                                   double velocityX,
                                   double velocityY) noexcept
{
    if (!IsCircuitGate(gateIndex) ||
        !std::isfinite(previous.XY.X) || !std::isfinite(previous.XY.Y) ||
        !std::isfinite(previous.Z) ||
        !std::isfinite(current.XY.X) || !std::isfinite(current.XY.Y) ||
        !std::isfinite(current.Z) ||
        !CanPassCircuitGate(gateIndex, velocityX, velocityY))
    {
        return false;
    }

    const double deltaX = current.XY.X - previous.XY.X;
    const double deltaY = current.XY.Y - previous.XY.Y;
    const double deltaZ = current.Z - previous.Z;
    if (!std::isfinite(deltaX) || !std::isfinite(deltaY) ||
        !std::isfinite(deltaZ) ||
        std::hypot(std::hypot(deltaX, deltaY), deltaZ) > GateMaxFrameTravelCm)
    {
        return false;
    }

    const CircuitPoint gate = CircuitGate(gateIndex);
    // The four fixed normals are +X, +Y, -X, -Y in race order.
    const double direction = gateIndex < 2 ? 1.0 : -1.0;
    const bool horizontal = gateIndex == 0 || gateIndex == 2;
    const double previousAlong =
        direction * ((horizontal ? previous.XY.X : previous.XY.Y) -
                     (horizontal ? gate.X : gate.Y));
    const double currentAlong =
        direction * ((horizontal ? current.XY.X : current.XY.Y) -
                     (horizontal ? gate.X : gate.Y));
    if (!(previousAlong < 0.0 && currentAlong >= 0.0) ||
        previousAlong < -GatePlaneHalfLengthCm ||
        currentAlong > GatePlaneHalfLengthCm)
    {
        return false;
    }

    // Interpolate the lateral and vertical coordinates precisely at the
    // gate plane, avoiding a false hit at its front edge or outside the lane.
    const double t = -previousAlong / (currentAlong - previousAlong);
    const double previousLateral = horizontal ? previous.XY.Y - gate.Y
                                             : previous.XY.X - gate.X;
    const double currentLateral = horizontal ? current.XY.Y - gate.Y
                                            : current.XY.X - gate.X;
    const double crossingLateral =
        previousLateral + t * (currentLateral - previousLateral);
    const double crossingZ = previous.Z + t * (current.Z - previous.Z);
    return std::isfinite(t) && t >= 0.0 && t <= 1.0 &&
        std::isfinite(crossingLateral) &&
        std::abs(crossingLateral) <= GateLateralHalfWidthCm &&
        std::isfinite(crossingZ) &&
        std::abs(crossingZ - GateCenterHeightCm) <= GateVerticalHalfHeightCm;
}
} // namespace latebrake
