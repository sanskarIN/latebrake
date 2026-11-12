#pragma once

#include "AI/RivalPilot.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace latebrake
{
// Single-player prototype courtesy braking, never a competitive placement,
// overtake planner or player-car control system. Distances in Unreal cm;
// heading in radians; velocities are *world* planar cm/second.
struct RivalTrafficObservation final
{
    double RivalX = 0.0;
    double RivalY = 0.0;
    double RivalHeadingRadians = 0.0;
    double RivalPlanarSpeedCmPerSecond = 0.0;
    double PlayerX = 0.0;
    double PlayerY = 0.0;
    double PlayerWorldVelocityX = 0.0;
    double PlayerWorldVelocityY = 0.0;
    bool PlayerReady = false;
    // Genuine rival WORLD velocity, needed to project actual forward closing.
    // Planar speed magnitude alone overestimates contact during lateral slip.
    double RivalWorldVelocityX = 0.0;
    double RivalWorldVelocityY = 0.0;
    // Previous qualified physical chassis sample: the SAME grip, tire,
    // damage, brake-fade and handling forecast used by rival corner braking.
    // Zero means missing authority, never an invented dry-road brake.
    double QualifiedBrakeDecelerationCmPerSecondSquared = 0.0;
};

inline PilotCommand YieldRivalToPlayer(
    PilotCommand command, const RivalTrafficObservation& sample) noexcept
{
    // Unknown or broken physics must never grant rival propulsion.
    // Do not treat missing player readiness as a harmless empty road.
    if (!sample.PlayerReady ||
        !std::isfinite(command.Throttle) ||
        !std::isfinite(command.Steering) ||
        command.Throttle < -1.0 || command.Throttle > 1.0 ||
        command.Steering < -1.0 || command.Steering > 1.0 ||
        !std::isfinite(sample.RivalX) || !std::isfinite(sample.RivalY) ||
        !std::isfinite(sample.PlayerX) || !std::isfinite(sample.PlayerY) ||
        !std::isfinite(sample.RivalHeadingRadians) ||
        std::abs(sample.RivalHeadingRadians) > 2.0 * std::numbers::pi ||
        !std::isfinite(sample.RivalPlanarSpeedCmPerSecond) ||
        sample.RivalPlanarSpeedCmPerSecond < 0.0 ||
        sample.RivalPlanarSpeedCmPerSecond > 15000.0 ||
        !std::isfinite(sample.PlayerWorldVelocityX) ||
        !std::isfinite(sample.PlayerWorldVelocityY) ||
        std::hypot(sample.PlayerWorldVelocityX, sample.PlayerWorldVelocityY) >
            15000.0 ||
        !std::isfinite(sample.RivalWorldVelocityX) ||
        !std::isfinite(sample.RivalWorldVelocityY) ||
        std::hypot(sample.RivalWorldVelocityX, sample.RivalWorldVelocityY) >
            15000.0 ||
        std::abs(std::hypot(sample.RivalWorldVelocityX,
                            sample.RivalWorldVelocityY) -
                 sample.RivalPlanarSpeedCmPerSecond) > 1.0 ||
        !std::isfinite(sample.QualifiedBrakeDecelerationCmPerSecondSquared) ||
        sample.QualifiedBrakeDecelerationCmPerSecondSquared < 1.0 ||
        sample.QualifiedBrakeDecelerationCmPerSecondSquared > 3000.0)
    {
        return {}; // default PilotCommand brakes and has no throttle
    }
    if (command.Arrived || command.Brake)
    {
        // Keep prior route arrival, braking and steering policy authoritative.
        return command;
    }

    const double aheadX = sample.PlayerX - sample.RivalX;
    const double aheadY = sample.PlayerY - sample.RivalY;
    const double facingX = std::cos(sample.RivalHeadingRadians);
    const double facingY = std::sin(sample.RivalHeadingRadians);
    const double forwardDistance = aheadX * facingX + aheadY * facingY;
    const double lateralDistance = std::abs(
        aheadY * facingX - aheadX * facingY);
    const double playerForwardSpeed =
        sample.PlayerWorldVelocityX * facingX +
        sample.PlayerWorldVelocityY * facingY;
    const double rivalForwardSpeed =
        sample.RivalWorldVelocityX * facingX +
        sample.RivalWorldVelocityY * facingY;
    const double closingSpeed =
        rivalForwardSpeed - playerForwardSpeed;
    if (!std::isfinite(forwardDistance) ||
        !std::isfinite(lateralDistance) ||
        !std::isfinite(closingSpeed))
    {
        return {};
    }

    // Use real prior-frame grip/brake-fade authority for early following
    // assessment, not a fixed 14.5-metre range at every closing velocity.
    // This is a deliberately bounded 1-D lane policy, NOT a guarantee
    // against collision or a replacement for Chaos contact resolution.
    constexpr double LaneCorridorCm = 225.0;
    constexpr double MaxAwarenessDistanceCm = 1450.0;
    constexpr double MaximumPredictiveAwarenessCm = 6500.0;
    constexpr double EmergencyDistanceCm = 600.0;
    constexpr double ConservativeContactClearanceCm = 380.0;
    constexpr double EmergencyFootprintDistanceCm = 280.0;
    constexpr double CriticalTimeToContactSeconds = 0.75;
    constexpr double ReactionReserveSeconds = 0.50;
    constexpr double PredictiveThrottleBufferCm = 500.0;

    // Model the closing velocity rather than total chassis speed to
    // preserve legitimate following/passing at nearly matching speeds.
    // A suddenly stationary or reversing player increases measured
    // closing speed and the required runway at the next physics sample.
    const double closing = std::max(0.0, closingSpeed);
    const double stoppingDistance =
        ConservativeContactClearanceCm +
        closing * ReactionReserveSeconds +
        closing * closing /
            (2.0 * sample.QualifiedBrakeDecelerationCmPerSecondSquared);
    const double awarenessDistance = std::clamp(
        std::max(MaxAwarenessDistanceCm,
                 stoppingDistance + PredictiveThrottleBufferCm),
        MaxAwarenessDistanceCm, MaximumPredictiveAwarenessCm);
    const bool occupiedFootprint =
        forwardDistance < EmergencyFootprintDistanceCm &&
        rivalForwardSpeed > 1.0;
    // The 65m cap applies to *gentle throttle awareness*, not to a
    // physically required brake: a badly faded car may require much
    // longer than 65m to decelerate at the observed closing speed.
    // Rejecting it solely on the awareness cap would be unsafe.
    const bool insufficientBrakingRunway =
        closingSpeed >= 500.0 && forwardDistance <= stoppingDistance;
    if (forwardDistance <= 0.0 ||
        (forwardDistance >= awarenessDistance &&
         !insufficientBrakingRunway) ||
        lateralDistance >= LaneCorridorCm ||
        (sample.RivalPlanarSpeedCmPerSecond < 450.0 &&
         !occupiedFootprint) ||
        (closingSpeed < 130.0 && !occupiedFootprint))
    {
        return command;
    }

    const double availableGap = std::max(
        0.0, forwardDistance - ConservativeContactClearanceCm);
    const double timeToContact = closingSpeed > 0.0
        ? availableGap / closingSpeed : -1.0;
    const bool nearEmergency =
        forwardDistance < EmergencyDistanceCm && closingSpeed >= 230.0;
    const bool predictedEmergency = closingSpeed >= 500.0 &&
        timeToContact >= 0.0 &&
        timeToContact <= CriticalTimeToContactSeconds;
    if (occupiedFootprint || nearEmergency || predictedEmergency ||
        insufficientBrakingRunway)
    {
        command.Throttle = 0.0;
        command.Brake = true;
        return command;
    }

    // Back off throttle across the approach band before the physical
    // stopping envelope. Keep the planner's original steering/corner
    // brake, and never create extra throttle or motor input.
    const double throttleStart = std::min(
        stoppingDistance, MaxAwarenessDistanceCm - 110.0);
    const double scaled = (forwardDistance - throttleStart) /
        std::max(1.0, awarenessDistance - throttleStart);
    const double permittedThrottle = std::clamp(
        0.15 + 0.55 * scaled, 0.15, 0.70);
    command.Throttle = std::min(command.Throttle, permittedThrottle);
    return command;
}
} // namespace latebrake
