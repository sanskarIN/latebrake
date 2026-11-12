#pragma once

#include "AI/RivalPilot.h"
#include "AI/RivalRecoveryOccupancy.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Pure C++20, allocation-free conservative traffic preflight for ONE real
// rival and at most five distinct physical blockers. In the normal 2-car
// game only the human occupies the blocker array. Future additional cars
// must provide actual actor/Chaos observations, never invented positions.
//
// Projection assumes constant short-term XY velocity, not Chaos dynamics,
// future steering, road elevation, or true collision prediction. It can
// only reduce propulsion / add braking; no steering, gate, lap or save access.
inline constexpr double FleetTrafficHorizonSeconds =
    RecoveryTrafficForecastSeconds;
inline constexpr double FleetTrafficFollowingRangeCm = 1550.0;
inline constexpr double FleetTrafficLaneHalfWidthCm = 270.0;
inline constexpr double FleetTrafficRearSkipCm = 470.0;
inline constexpr double FleetTrafficMinClosingSpeedCmS = 120.0;
inline constexpr double FleetTrafficMaxSpeedCmS = 15000.0;

struct RivalFleetTrafficSample final
{
    RecoveryObstacle Self{};
    std::array<RecoveryObstacle, MaxRecoveryObstacles> Blockers{};
    int BlockerCount = 0;
    bool SessionQualified = false;
};

struct RivalFleetTrafficResult final
{
    PilotCommand Command{};
    bool Qualified = false;
    bool EmergencyBrake = false;
    int LimitingSlot = -1;
};

inline bool ValidFleetPhysicalObservation(
    const RecoveryObstacle& actor) noexcept
{
    return actor.Verified &&
        actor.Slot >= HumanGridSlot &&
        actor.Slot < StartGridCapacity &&
        actor.ActorToken != 0 &&
        IsValidStartGridPose(actor.Pose) &&
        std::isfinite(actor.VelocityX) &&
        std::isfinite(actor.VelocityY) &&
        std::hypot(actor.VelocityX, actor.VelocityY) <=
            FleetTrafficMaxSpeedCmS;
}

inline RivalFleetTrafficResult ApplyRivalFleetTraffic(
    PilotCommand command, const RivalFleetTrafficSample& sample) noexcept
{
    const auto malformed = [&]() noexcept
    {
        return !sample.SessionQualified ||
            !ValidFleetPhysicalObservation(sample.Self) ||
            sample.Self.Slot == HumanGridSlot ||
            sample.BlockerCount < 1 ||
            sample.BlockerCount > MaxRecoveryObstacles ||
            !std::isfinite(command.Throttle) ||
            !std::isfinite(command.Steering) ||
            command.Throttle < 0.0 || command.Throttle > 1.0 ||
            command.Steering < -1.0 || command.Steering > 1.0;
    };
    if (malformed())
    {
        return {}; // Default PilotCommand is stopped and braking.
    }
    // No fabricated empty lane: validate *all* physical blockers before
    // making even one throttle decision. Reject duplicate real identities.
    for (int i = 0; i < sample.BlockerCount; ++i)
    {
        const RecoveryObstacle& other =
            sample.Blockers[static_cast<std::size_t>(i)];
        if (!ValidFleetPhysicalObservation(other) ||
            other.Slot == sample.Self.Slot ||
            other.ActorToken == sample.Self.ActorToken)
        {
            return {};
        }
        for (int j = 0; j < i; ++j)
        {
            const RecoveryObstacle& prior =
                sample.Blockers[static_cast<std::size_t>(j)];
            if (prior.Slot == other.Slot ||
                prior.ActorToken == other.ActorToken)
            {
                return {};
            }
        }
    }
    // Preserve a route/corner brake that was already commanded by physics.
    if (command.Brake || command.Arrived)
    {
        return {command, true, false, -1};
    }

    const double heading =
        sample.Self.Pose.HeadingDegrees * (3.14159265358979323846 / 180.0);
    const double forwardX = std::cos(heading);
    const double forwardY = std::sin(heading);
    int limitingSlot = -1;
    for (int i = 0; i < sample.BlockerCount; ++i)
    {
        const RecoveryObstacle& other =
            sample.Blockers[static_cast<std::size_t>(i)];
        const double dx =
            other.Pose.Position.X - sample.Self.Pose.Position.X;
        const double dy =
            other.Pose.Position.Y - sample.Self.Pose.Position.Y;
        const double ahead = dx * forwardX + dy * forwardY;
        const double lateral = std::abs(
            dy * forwardX - dx * forwardY);
        const double relativeVelocityX =
            other.VelocityX - sample.Self.VelocityX;
        const double relativeVelocityY =
            other.VelocityY - sample.Self.VelocityY;
        const double closing =
            -(relativeVelocityX * forwardX +
              relativeVelocityY * forwardY);
        if (!std::isfinite(ahead) || !std::isfinite(lateral) ||
            !std::isfinite(closing))
        {
            return {};
        }
        // Braking for a car well BEHIND us would increase rear-contact
        // risk. Safety for the tail car belongs to that tail car's guard.
        if (ahead < -FleetTrafficRearSkipCm)
        {
            continue;
        }
        const bool predictedContact = PredictedRecoveryFootprintsOverlap(
            sample.Self.Pose, other.Pose,
            relativeVelocityX, relativeVelocityY);
        if (predictedContact &&
            (ahead >= -GridPaddedHalfLengthCm ||
             lateral <= FleetTrafficLaneHalfWidthCm))
        {
            command.Throttle = 0.0;
            command.Brake = true;
            return {command, true, true, other.Slot};
        }

        // Gradual follow demand outside immediate swept collision
        // envelope; only the same lane and physically closing ahead.
        if (ahead > 0.0 &&
            ahead < FleetTrafficFollowingRangeCm &&
            lateral < FleetTrafficLaneHalfWidthCm &&
            closing > FleetTrafficMinClosingSpeedCmS)
        {
            const double available =
                std::clamp((ahead - 450.0) /
                    (FleetTrafficFollowingRangeCm - 450.0), 0.0, 1.0);
            const double permitted = 0.15 + 0.65 * available;
            if (permitted < command.Throttle)
            {
                command.Throttle = permitted;
                limitingSlot = other.Slot;
            }
        }
    }
    // Never return early on a *soft* following cap. A later real car can
    // still be on a swept collision path and must veto ALL propulsion.
    return {command, true, false, limitingSlot};
}
} // namespace latebrake
