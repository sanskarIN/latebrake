#pragma once

#include <cmath>

namespace latebrake
{
// First circuit order: eastbound, northbound, westbound, southbound.
// Require meaningful forward motion through a gate, not just overlap.
// This pure C++20 helper stays independently testable without Unreal.
inline bool CanPassCircuitGate(int checkpointIndex, double velocityX,
                               double velocityY, double minimumForwardSpeed = 150.0) noexcept
{
    if (checkpointIndex < 0 || checkpointIndex >= 4 ||
        !std::isfinite(velocityX) || !std::isfinite(velocityY) ||
        !std::isfinite(minimumForwardSpeed) || minimumForwardSpeed <= 0.0)
    {
        return false;
    }

    double forwardSpeed = 0.0;
    switch (checkpointIndex)
    {
    case 0: forwardSpeed = velocityX; break;
    case 1: forwardSpeed = velocityY; break;
    case 2: forwardSpeed = -velocityX; break;
    case 3: forwardSpeed = -velocityY; break;
    default: return false;
    }
    return forwardSpeed >= minimumForwardSpeed;
}
} // namespace latebrake
