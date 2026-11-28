#pragma once

#include "AI/RivalPilot.h"
#include "AI/RivalRecoveryOccupancy.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

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
    // Additional prior-Chaos-frame physics for predictive braking only.
    // Legacy immediate traffic does not consume these trailing fields.
    bool BrakeObservationQualified = false;
    double ConservativeBrakeScale = 0.0;
};

enum class FleetImmediateThreat : std::uint8_t
{
    None,Following,SweptContact,Touching
};

struct RivalFleetTrafficResult final
{
    PilotCommand Command{};
    bool Qualified = false;
    bool EmergencyBrake = false;
    int LimitingSlot = -1;
    FleetImmediateThreat Threat=FleetImmediateThreat::None;
    double ContactSeconds=FootprintSweepNoContact;
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
    RivalFleetTrafficResult result{command,true,false,-1};
    for (int i=0;i<sample.BlockerCount;++i)
    {
        const auto& other=sample.Blockers[static_cast<std::size_t>(i)];
        const double dx=other.Pose.Position.X-sample.Self.Pose.Position.X;
        const double dy=other.Pose.Position.Y-sample.Self.Pose.Position.Y;
        const double ahead=dx*forwardX+dy*forwardY;
        const double lateral=std::abs(dy*forwardX-dx*forwardY);
        const double vx=other.VelocityX-sample.Self.VelocityX;
        const double vy=other.VelocityY-sample.Self.VelocityY;
        const double closing=-(vx*forwardX+vy*forwardY);
        if(!std::isfinite(ahead) || !std::isfinite(lateral) || !std::isfinite(closing))
            return {};

        // An actual pair may have twice the valid single-body speed.
        // Do not use the stationary recovery wrapper's 15000 cm/s
        // blocker bound for this relative-motion collision calculation.
        const auto sweep=SweepPlanarFootprints({sample.Self.Pose,other.Pose,
            vx,vy,FleetTrafficHorizonSeconds,0.0});
        if(!sweep.Valid)return {};

        FleetImmediateThreat threat=FleetImmediateThreat::None;
        double contact=FootprintSweepNoContact;
        double ceiling=command.Throttle;
        if(sweep.StartsOverlapping)
        {
            // Already touching: zero traction even when the center of
            // the other car is behind the old courtesy-braking cutoff.
            threat=FleetImmediateThreat::Touching;
            contact=0.0;
            ceiling=0.0;
        }
        else if(ahead>=-FleetTrafficRearSkipCm && sweep.Intersects &&
            (ahead>=-GridPaddedHalfLengthCm || lateral<=FleetTrafficLaneHalfWidthCm))
        {
            threat=FleetImmediateThreat::SweptContact;
            contact=sweep.EntrySeconds;
            ceiling=0.0;
        }
        else if(ahead>0.0 && ahead<FleetTrafficFollowingRangeCm &&
            lateral<FleetTrafficLaneHalfWidthCm && closing>FleetTrafficMinClosingSpeedCmS)
        {
            const double available=std::clamp((ahead-450.0)/
                (FleetTrafficFollowingRangeCm-450.0),0.0,1.0);
            const double permitted=0.15+0.65*available;
            if(permitted<command.Throttle)
            {
                threat=FleetImmediateThreat::Following;
                ceiling=permitted;
            }
        }

        if(threat==FleetImmediateThreat::None)continue;
        // Rank all real threats: touching first, then earliest swept
        // entry, then strongest soft cap. Stable slot resolves only
        // equivalent restrictions. No iteration-order brake reporting.
        bool prefer=static_cast<unsigned>(threat)>static_cast<unsigned>(result.Threat);
        if(threat==result.Threat)
        {
            if(threat==FleetImmediateThreat::Following)
                prefer=ceiling<result.Command.Throttle ||
                    (ceiling==result.Command.Throttle && other.Slot<result.LimitingSlot);
            else
                prefer=contact<result.ContactSeconds ||
                    (contact==result.ContactSeconds && other.Slot<result.LimitingSlot);
        }
        if(!prefer)continue;
        result.Threat=threat;
        result.LimitingSlot=other.Slot;
        result.ContactSeconds=contact;
        result.Command.Throttle=ceiling;
        result.EmergencyBrake=threat==FleetImmediateThreat::Touching ||
            threat==FleetImmediateThreat::SweptContact;
        result.Command.Brake=result.EmergencyBrake;
    }
    return result;
}

} // namespace latebrake
