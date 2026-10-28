#pragma once

#include <cmath>

namespace latebrake
{
// A cosmetic 3D world-space distance to the ghost pose sampled at the SAME
// elapsed lap time. It is not a signed racing gap, position or pace delta.
struct GhostSeparationInput final
{
    bool ReplayAuthorized = false;
    bool BeaconVisible = false;
    double PlayerX = 0.0;
    double PlayerY = 0.0;
    double PlayerZ = 0.0;
    double GhostX = 0.0;
    double GhostY = 0.0;
    double GhostZ = 0.0;
};

inline bool TryComputeGhostSeparationMeters(
    const GhostSeparationInput& input, double& outMeters) noexcept
{
    outMeters = 0.0;
    constexpr double WorldBoundCm = 500000.0;
    constexpr double MaxHudDistanceMeters = 250.0;
    if (!input.ReplayAuthorized || !input.BeaconVisible ||
        !std::isfinite(input.PlayerX) || !std::isfinite(input.PlayerY) ||
        !std::isfinite(input.PlayerZ) || !std::isfinite(input.GhostX) ||
        !std::isfinite(input.GhostY) || !std::isfinite(input.GhostZ) ||
        std::abs(input.PlayerX) > WorldBoundCm ||
        std::abs(input.PlayerY) > WorldBoundCm ||
        std::abs(input.PlayerZ) > WorldBoundCm ||
        std::abs(input.GhostX) > WorldBoundCm ||
        std::abs(input.GhostY) > WorldBoundCm ||
        std::abs(input.GhostZ) > WorldBoundCm)
    {
        return false;
    }
    // Unreal locations are centimeters. Use hypot to avoid avoidable
    // intermediate overflow; never show bogus long-distance telemetry.
    const double separationCm = std::hypot(
        std::hypot(input.PlayerX - input.GhostX,
                   input.PlayerY - input.GhostY),
        input.PlayerZ - input.GhostZ);
    const double meters = separationCm / 100.0;
    if (!std::isfinite(meters) || meters > MaxHudDistanceMeters)
    {
        return false;
    }
    outMeters = meters;
    return true;
}
} // namespace latebrake
