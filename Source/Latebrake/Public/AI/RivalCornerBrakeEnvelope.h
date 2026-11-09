#pragma once

#include "AI/RivalPilot.h"
#include "Track/CircuitLayout.h"
#include "Vehicles/GripLimitedBraking.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Forecasts whether the REAL physical rival can slow for the NEXT
// verified 90-degree gate at the current measured speed. This remains
// a source-level approximate arcade guide, not wheel torque or
// actual Chaos tire contact and not a racing-line/overtaking system.
struct RivalCornerBrakeSample final
{
    int NextGate = -1;
    CircuitPoint Position{};
    double PlanarSpeedCmPerSecond = 0.0;
    double SurfaceGrip = 1.0;
    double TireGrip = 1.0;
    double ImpactGrip = 1.0;
    double BrakeHeatMultiplier = 1.0;
    bool ArcadeHandling = true;
    bool PriorPhysicsQualified = false;
    bool UseContactCapacityCeiling = false;
    double ContactDecelerationCeiling = 0.0;
    double ReactionSeconds = 0.0;
};

struct RivalCornerBrakeResult final
{
    PilotCommand Command{};
    bool Valid = false;
    bool InboundCorner = false;
    bool AnticipatoryBrake = false;
    double BrakeEnvelopeSpeedCmPerSecond = 0.0;
    double SafeDecelerationCmPerSecondSquared = 0.0;
};

// Maximum signed inbound distance on the original paved straight, not a
// radial distance that also matches the wrong lane or already-passed gate.
inline constexpr double RivalEarlyBrakeMaximumApproachCm = 5500.0;
inline constexpr double RivalEarlyBrakeMinimumApproachCm = 440.0;
inline constexpr double RivalEarlyBrakeMaximumCrossTrackCm = 620.0;
inline constexpr double RivalCornerExitSpeedCmPerSecond = 1550.0;
inline constexpr double RivalCornerRunThroughBufferCm = 550.0;
inline constexpr double RivalBrakeForecastSafetyScale = 0.76;

// Use the same approximate longitudinal braking base as the actual
// player's/rival's source-level GripLimitedBraking policy, derated
// conservatively to account for the untested Chaos integration.
inline RivalCornerBrakeResult ApplyRivalCornerBrakeEnvelope(
    PilotCommand command, RivalCornerBrakeSample sample) noexcept
{
    if (!IsCircuitGate(sample.NextGate) ||
        !std::isfinite(sample.Position.X) ||
        !std::isfinite(sample.Position.Y) ||
        !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
        sample.PlanarSpeedCmPerSecond < 0.0 ||
        sample.PlanarSpeedCmPerSecond > 10000.0 ||
        !std::isfinite(sample.SurfaceGrip) ||
        sample.SurfaceGrip < 0.05 || sample.SurfaceGrip > 2.0 ||
        !std::isfinite(sample.TireGrip) ||
        sample.TireGrip < 0.05 || sample.TireGrip > 2.0 ||
        !std::isfinite(sample.ImpactGrip) ||
        sample.ImpactGrip < 0.05 || sample.ImpactGrip > 2.0 ||
        !std::isfinite(sample.BrakeHeatMultiplier) ||
        sample.BrakeHeatMultiplier < 0.05 ||
        sample.BrakeHeatMultiplier > 1.0 ||
        (sample.UseContactCapacityCeiling &&
            (!std::isfinite(sample.ContactDecelerationCeiling) ||
             sample.ContactDecelerationCeiling < 0.0 ||
             sample.ContactDecelerationCeiling > 5000.0 ||
             !std::isfinite(sample.ReactionSeconds) ||
             sample.ReactionSeconds < 0.0 || sample.ReactionSeconds > 1.0)) ||
        !std::isfinite(command.Throttle) ||
        command.Throttle < 0.0 || command.Throttle > 1.0 ||
        !std::isfinite(command.Steering) ||
        std::abs(command.Steering) > 1.0 ||
        !sample.PriorPhysicsQualified)
    {
        // A malformed chassis, brake/road condition or route must never
        // accidentally accelerate the autonomous car toward a wall.
        return {{}, false, false, false, 0.0, 0.0};
    }

    const double effectiveGrip = std::clamp(
        sample.SurfaceGrip * sample.TireGrip * sample.ImpactGrip,
        0.10, 2.0);
    const double nominalSafeDeceleration =
        (sample.ArcadeHandling ? ArcadeServiceBrakeDeceleration
                               : SimulationServiceBrakeDeceleration) *
        effectiveGrip * sample.BrakeHeatMultiplier *
        RivalBrakeForecastSafetyScale;
    const double safeBrakeDeceleration = sample.UseContactCapacityCeiling
        ? std::min(nominalSafeDeceleration, sample.ContactDecelerationCeiling)
        : nominalSafeDeceleration;
    if (!std::isfinite(safeBrakeDeceleration))
    {
        return {};
    }

    if (sample.UseContactCapacityCeiling && safeBrakeDeceleration <= 1.0e-6 && !command.Arrived)
    {
        command.Throttle = 0.0;
        command.Brake = true;
    }
    const auto ordinary = [&]() noexcept -> RivalCornerBrakeResult
    {
        return {command, true, false, false,
                0.0, safeBrakeDeceleration};
    };
    if (command.Arrived || command.Brake ||
        !IsOnPavedSurface(sample.Position))
    {
        return ordinary();
    }

    const CircuitPoint corner = CircuitGate(sample.NextGate);
    const CircuitPoint prior = CircuitGate(
        (sample.NextGate + CircuitGateCount - 1) % CircuitGateCount);
    const CircuitPoint successor = CircuitGate(
        (sample.NextGate + 1) % CircuitGateCount);
    const double incomingX = corner.X - prior.X;
    const double incomingY = corner.Y - prior.Y;
    const double outgoingX = successor.X - corner.X;
    const double outgoingY = successor.Y - corner.Y;
    const double length = std::hypot(incomingX, incomingY);
    const double exitLength = std::hypot(outgoingX, outgoingY);
    if (!std::isfinite(length) || length <= 0.0 ||
        !std::isfinite(exitLength) || exitLength <= 0.0)
    {
        return {};
    }

    const double toX = corner.X - sample.Position.X;
    const double toY = corner.Y - sample.Position.Y;
    const double inboundRemaining =
        (toX * incomingX + toY * incomingY) / length;
    const double crossTrack =
        std::abs(toX * incomingY - toY * incomingX) / length;
    const double headingDot =
        (incomingX * outgoingX + incomingY * outgoingY) /
        (length * exitLength);
    if (!std::isfinite(inboundRemaining) ||
        !std::isfinite(crossTrack) ||
        !std::isfinite(headingDot))
    {
        return {};
    }
    // Never brake for a gate that was already crossed, a shoulder
    // or an unrelated road segment; preserve the existing AI controls.
    const double maximumApproach = sample.UseContactCapacityCeiling
        ? length : RivalEarlyBrakeMaximumApproachCm;
    if (inboundRemaining < RivalEarlyBrakeMinimumApproachCm ||
        inboundRemaining > maximumApproach ||
        crossTrack > RivalEarlyBrakeMaximumCrossTrackCm ||
        headingDot > 0.86)
    {
        return ordinary();
    }

    const double runway = std::max(0.0,
        inboundRemaining - RivalCornerRunThroughBufferCm -
            (sample.UseContactCapacityCeiling ?
                sample.PlanarSpeedCmPerSecond * sample.ReactionSeconds : 0.0));
    const double targetSpeed = std::sqrt(
        RivalCornerExitSpeedCmPerSecond * RivalCornerExitSpeedCmPerSecond +
        2.0 * safeBrakeDeceleration * runway);
    if (!std::isfinite(targetSpeed))
    {
        return {};
    }
    bool requestedBrake = false;
    if (sample.PlanarSpeedCmPerSecond > targetSpeed + 100.0)
    {
        command.Throttle = 0.0;
        command.Brake = true;
        requestedBrake = true;
    }
    else if (sample.PlanarSpeedCmPerSecond >= targetSpeed - 100.0)
    {
        // Enter the deceleration envelope without making an unnecessary
        // throttle surge; never create more propulsion than the planner.
        command.Throttle = std::min(command.Throttle, 0.10);
    }
    return {command, true, true, requestedBrake,
            targetSpeed, safeBrakeDeceleration};
}
} // namespace latebrake
