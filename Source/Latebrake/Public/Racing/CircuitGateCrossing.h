#pragma once

#include "Racing/GateValidation.h"
#include "Track/CircuitLayout.h"

#include <cmath>

namespace latebrake
{
// Frame-to-frame samples used by the four fixed starter-circuit gates.
// Geometry uses centimetres; Unreal-independent to permit deterministic CI.
struct CircuitGateSample final
{
    CircuitPoint XY{};
    double Z = 0.0;
};

// The old sphere is a visual marker, not a frame-sample acceptance radius.
inline constexpr double GateVisualTriggerRadiusCm = 600.0;
inline constexpr double GateLateralHalfWidthCm = 500.0;
inline constexpr double GateCenterHeightCm = 38.0;
inline constexpr double GateVerticalHalfHeightCm = 380.0;
inline constexpr double GateMaxFrameTravelCm = 1200.0;

// Diagnostic reason for a failed or accepted directional crossing. Stable
// names can be included in actual engine/device QA logs without replacing
// a boolean gameplay decision or allocating on the four gate actor ticks.
enum class CircuitGateDecision
{
    Crossed,
    InvalidGate,
    InvalidSample,
    InvalidVelocity,
    NotMovingForward,
    ExcessiveFrameTravel,
    NoForwardPlaneCrossing,
    OutsideLateralCorridor,
    OutsideVerticalCorridor
};

inline constexpr const char* CircuitGateDecisionName(
    CircuitGateDecision decision) noexcept
{
    switch (decision)
    {
    case CircuitGateDecision::Crossed: return "crossed";
    case CircuitGateDecision::InvalidGate: return "invalid-gate";
    case CircuitGateDecision::InvalidSample: return "invalid-sample";
    case CircuitGateDecision::InvalidVelocity: return "invalid-velocity";
    case CircuitGateDecision::NotMovingForward: return "not-moving-forward";
    case CircuitGateDecision::ExcessiveFrameTravel: return "excessive-frame-travel";
    case CircuitGateDecision::NoForwardPlaneCrossing: return "no-forward-crossing";
    case CircuitGateDecision::OutsideLateralCorridor: return "outside-lane";
    case CircuitGateDecision::OutsideVerticalCorridor: return "outside-height";
    }
    return "unknown";
}

// Report the *first* reason why a sampled crossing was not accepted.
// A real 3D swept segment must cross forward through the checkpoint
// plane at a valid interpolated lateral position and vehicle height.
// Total 3D displacement stays bounded at 1200cm; authorized teleport
// generation resets are checked separately in CheckpointMotionTrace.
inline CircuitGateDecision EvaluateCircuitGatePlane(
    int gateIndex, CircuitGateSample previous, CircuitGateSample current,
    double velocityX, double velocityY) noexcept
{
    if (!IsCircuitGate(gateIndex))
    {
        return CircuitGateDecision::InvalidGate;
    }
    if (!std::isfinite(previous.XY.X) || !std::isfinite(previous.XY.Y) ||
        !std::isfinite(previous.Z) ||
        !std::isfinite(current.XY.X) || !std::isfinite(current.XY.Y) ||
        !std::isfinite(current.Z))
    {
        return CircuitGateDecision::InvalidSample;
    }
    if (!std::isfinite(velocityX) || !std::isfinite(velocityY))
    {
        return CircuitGateDecision::InvalidVelocity;
    }
    if (!CanPassCircuitGate(gateIndex, velocityX, velocityY))
    {
        return CircuitGateDecision::NotMovingForward;
    }

    const double deltaX = current.XY.X - previous.XY.X;
    const double deltaY = current.XY.Y - previous.XY.Y;
    const double deltaZ = current.Z - previous.Z;
    if (!std::isfinite(deltaX) || !std::isfinite(deltaY) ||
        !std::isfinite(deltaZ) ||
        std::hypot(std::hypot(deltaX, deltaY), deltaZ) > GateMaxFrameTravelCm)
    {
        return CircuitGateDecision::ExcessiveFrameTravel;
    }

    const CircuitPoint gate = CircuitGate(gateIndex);
    const double direction = gateIndex < 2 ? 1.0 : -1.0;
    const bool horizontal = gateIndex == 0 || gateIndex == 2;
    const double previousAlong =
        direction * ((horizontal ? previous.XY.X : previous.XY.Y) -
                     (horizontal ? gate.X : gate.Y));
    const double currentAlong =
        direction * ((horizontal ? current.XY.X : current.XY.Y) -
                     (horizontal ? gate.X : gate.Y));
    if (!(previousAlong < 0.0 && currentAlong >= 0.0))
    {
        return CircuitGateDecision::NoForwardPlaneCrossing;
    }

    const double t = -previousAlong / (currentAlong - previousAlong);
    if (!std::isfinite(t) || t < 0.0 || t > 1.0)
    {
        return CircuitGateDecision::NoForwardPlaneCrossing;
    }
    const double previousLateral = horizontal ? previous.XY.Y - gate.Y
                                             : previous.XY.X - gate.X;
    const double currentLateral = horizontal ? current.XY.Y - gate.Y
                                            : current.XY.X - gate.X;
    const double crossingLateral =
        previousLateral + t * (currentLateral - previousLateral);
    if (!std::isfinite(crossingLateral) ||
        std::abs(crossingLateral) > GateLateralHalfWidthCm)
    {
        return CircuitGateDecision::OutsideLateralCorridor;
    }

    const double crossingZ = previous.Z + t * (current.Z - previous.Z);
    if (!std::isfinite(crossingZ) ||
        std::abs(crossingZ - GateCenterHeightCm) > GateVerticalHalfHeightCm)
    {
        return CircuitGateDecision::OutsideVerticalCorridor;
    }
    return CircuitGateDecision::Crossed;
}

// Preserve the existing public gameplay contract. Diagnostic classification
// must NEVER loosen checkpoint acceptance, ordering or teleport protection.
inline bool CrossesCircuitGatePlane(int gateIndex,
                                   CircuitGateSample previous,
                                   CircuitGateSample current,
                                   double velocityX,
                                   double velocityY) noexcept
{
    return EvaluateCircuitGatePlane(gateIndex, previous, current,
                                   velocityX, velocityY) ==
           CircuitGateDecision::Crossed;
}
} // namespace latebrake
