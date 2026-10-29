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
                 sample.RivalPlanarSpeedCmPerSecond) > 1.0)
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

    // Fail benignly when the player is behind, beside, stopped far away,
    // in another lane, or not being caught. Keep the rival's existing
    // corner and checkpoint steering: no off-track evasive lunges.
    constexpr double LaneCorridorCm = 225.0;
    constexpr double MaxAwarenessDistanceCm = 1450.0;
    constexpr double EmergencyDistanceCm = 600.0;
    if (forwardDistance <= 0.0 ||
        forwardDistance >= MaxAwarenessDistanceCm ||
        lateralDistance >= LaneCorridorCm ||
        sample.RivalPlanarSpeedCmPerSecond < 450.0 ||
        closingSpeed < 130.0)
    {
        return command;
    }

    // Strong approach to an occupied lane: prioritize slowing over
    // simulated contact. This is not collision prediction: moving cars,
    // walls, track curvature and overtakes still need Unreal playtests.
    // Distance alone is too late for high closing speeds: a rival moving
    // much faster than the player may traverse the remaining gap before
    // the next ordinary near-distance brake threshold. The projection is
    // deliberately conservative and valid ONLY inside the same-lane,
    // positive-forward, finite-velocity guards above.
    constexpr double ConservativeContactClearanceCm = 380.0;
    constexpr double CriticalTimeToContactSeconds = 0.75;
    const double availableGap = std::max(
        0.0, forwardDistance - ConservativeContactClearanceCm);
    const double timeToContact = availableGap / closingSpeed;
    const bool nearEmergency =
        forwardDistance < EmergencyDistanceCm && closingSpeed >= 230.0;
    const bool predictedEmergency =
        closingSpeed >= 500.0 &&
        timeToContact <= CriticalTimeToContactSeconds;
    if (nearEmergency || predictedEmergency)
    {
        command.Throttle = 0.0;
        command.Brake = true;
        return command;
    }

    // Smooth distance-based motor reduction across the remaining zone;
    // never creates extra throttle, brake false, reverse, or steering.
    const double scaled = (forwardDistance - 110.0) /
        (MaxAwarenessDistanceCm - 110.0);
    const double permittedThrottle = std::clamp(
        0.15 + 0.55 * scaled, 0.15, 0.70);
    command.Throttle = std::min(command.Throttle, permittedThrottle);
    return command;
}
} // namespace latebrake
