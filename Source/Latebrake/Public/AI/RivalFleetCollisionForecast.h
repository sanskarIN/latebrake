#pragma once

#include "AI/RivalFleetTraffic.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>

namespace latebrake
{
// Bounded, read-only six-car collision forecast. This is an ADDITIONAL
// conservative brake-only guard after the established real-Chaos traffic
// policy; it is not simulated tire physics, a steering/passing planner or
// certification of a collision-free future. The actual actor roster and
// velocities must be freshly observed by the Unreal owner every frame.
//
// Positions are Unreal centimetres, velocities cm/s, headings degrees.
// Proposals use 6 physical actor slots; no heap, IO or engine dependency.
inline constexpr double FleetForecastHorizonSeconds = 1.4;
inline constexpr double FleetForecastContactPaddingCm = 45.0;
inline constexpr double FleetForecastReactionSeconds = 0.24;
inline constexpr double FleetForecastDecelerationCmS2 = 520.0;
inline constexpr double FleetForecastClearanceCm = 500.0;
inline constexpr double FleetForecastFollowingRangeCm = 2550.0;
inline constexpr double FleetForecastLaneHalfWidthCm = 360.0;
inline constexpr double FleetForecastMinimumClosingCmS = 120.0;
inline constexpr double FleetForecastMaximumLongitudinalCm = 7000.0;

enum class FleetRiskKind
{
    Unqualified,
    Clear,
    Following,
    StoppingDistance,
    SweptContact,
    Touching
};

struct FleetSweptContact final
{
    bool Intersects = false;
    double FirstContactSeconds = std::numeric_limits<double>::infinity();
};

struct FleetRiskReport final
{
    FleetRiskKind Kind = FleetRiskKind::Unqualified;
    int LimitingSlot = -1;
    double TimeToContactSeconds = std::numeric_limits<double>::infinity();
    double ForwardGapCm = 0.0;
    double ClosingSpeedCmS = 0.0;
    double ThrottleCeiling = 0.0;

    bool Qualified() const noexcept { return Kind != FleetRiskKind::Unqualified; }
    bool Emergency() const noexcept
    {
        return Kind == FleetRiskKind::Touching ||
               Kind == FleetRiskKind::SweptContact ||
               Kind == FleetRiskKind::StoppingDistance;
    }
};

// The physical actor centre being on road is insufficient: its padded
// rotated chassis may still hang over unpaved terrain or intersect a wall.
// Certify full OBB containment in ONE original collision-bearing asphalt
// slab and exact SAT clearance from all original physical barriers.
// At a junction two slabs form a union; one-slab proof intentionally
// fails closed for some legal diagonal poses until real UE driving QA.
inline bool FleetBodyInsideAuthoredAsphalt(CircuitPose pose) noexcept
{
    if (!IsValidStartGridPose(pose) ||
        !IsOnPavedSurface(pose.Position) ||
        StartGridFootprintTouchesBarrier(pose))
    {
        return false;
    }
    const StartGridRect body = MakeStartGridRect(pose);
    const double projectedX =
        StartGridProjectionRadius(body, 1.0, 0.0);
    const double projectedY =
        StartGridProjectionRadius(body, 0.0, 1.0);
    if (!std::isfinite(projectedX) ||
        !std::isfinite(projectedY))
    {
        return false;
    }
    for (int index = 0; index < CircuitCollisionBlockCount; ++index)
    {
        const CircuitCollisionBlock slab = PlannedCollisionBlock(index);
        if (slab.Style != CollisionBlockStyle::Asphalt ||
            !IsValidCollisionBlock(slab))
        {
            continue;
        }
        // Original UE cube mesh is 100cm wide. Its scaled X/Y half-size
        // is 50cm times the authored block's individual axis scale.
        const double halfX = 50.0 * slab.ScaleX;
        const double halfY = 50.0 * slab.ScaleY;
        if (std::abs(body.X - slab.Center.X) + projectedX <= halfX &&
            std::abs(body.Y - slab.Center.Y) + projectedY <= halfY)
        {
            return true;
        }
    }
    return false;
}

// All six actor tokens must be unique, the slot inventory exact, and all
// observed padded chassis must be inside original asphalt and clear walls.
// This intentionally does NOT assume the sorted order of Unreal's TArray.
// A missing fifth rival is NOT an empty drivable road.
inline bool ValidateSixCarFleet(
    const RivalFleetTrafficSample& sample,
    std::array<std::uint64_t, StartGridCapacity>& roster) noexcept
{
    roster.fill(0);
    if (!sample.SessionQualified ||
        sample.BlockerCount != MaxRecoveryObstacles ||
        !ValidFleetPhysicalObservation(sample.Self) ||
        sample.Self.Slot == HumanGridSlot ||
        !FleetBodyInsideAuthoredAsphalt(sample.Self.Pose))
    {
        return false;
    }

    roster[static_cast<std::size_t>(sample.Self.Slot)] =
        sample.Self.ActorToken;
    for (int i = 0; i < MaxRecoveryObstacles; ++i)
    {
        const RecoveryObstacle& other =
            sample.Blockers[static_cast<std::size_t>(i)];
        if (!ValidFleetPhysicalObservation(other) ||
            !FleetBodyInsideAuthoredAsphalt(other.Pose) ||
            other.Slot == sample.Self.Slot ||
            roster[static_cast<std::size_t>(other.Slot)] != 0)
        {
            return false;
        }
        for (int prior = 0; prior < i; ++prior)
        {
            if (other.ActorToken ==
                sample.Blockers[static_cast<std::size_t>(prior)].ActorToken)
            {
                return false;
            }
        }
        if (other.ActorToken == sample.Self.ActorToken)
        {
            return false;
        }
        roster[static_cast<std::size_t>(other.Slot)] = other.ActorToken;
    }
    return std::all_of(roster.begin(), roster.end(),
                       [](std::uint64_t id) { return id != 0; });
}

// Continuous OBB-v-OBB separating-axis collision window with constant
// measured world velocity. Expands the existing padded chassis by a small
// forecast uncertainty on each projected axis, rather than relying on
// discrete future snapshots that can tunnel past each other at speed.
// The contact time is capped; steering/yaw acceleration are not predicted.
inline FleetSweptContact ForecastFleetContact(
    const RecoveryObstacle& self,
    const RecoveryObstacle& other) noexcept
{
    if (!ValidFleetPhysicalObservation(self) ||
        !ValidFleetPhysicalObservation(other))
    {
        return {true, 0.0}; // malformed input can never mean a clear road
    }
    const StartGridRect a = MakeStartGridRect(self.Pose);
    const StartGridRect b = MakeStartGridRect(other.Pose);
    const double dx = b.X - a.X;
    const double dy = b.Y - a.Y;
    const double vx = other.VelocityX - self.VelocityX;
    const double vy = other.VelocityY - self.VelocityY;
    double start = 0.0;
    double end = FleetForecastHorizonSeconds;

    const std::array<CircuitPoint, 4> axes{{
        {a.ForwardX, a.ForwardY},
        {a.RightX, a.RightY},
        {b.ForwardX, b.ForwardY},
        {b.RightX, b.RightY}
    }};
    for (const CircuitPoint axis : axes)
    {
        const double separation = dx * axis.X + dy * axis.Y;
        const double relativeSpeed = vx * axis.X + vy * axis.Y;
        const double radius =
            StartGridProjectionRadius(a, axis.X, axis.Y) +
            StartGridProjectionRadius(b, axis.X, axis.Y) +
            FleetForecastContactPaddingCm;
        if (!std::isfinite(separation) ||
            !std::isfinite(relativeSpeed) ||
            !std::isfinite(radius))
        {
            return {true, 0.0};
        }
        if (std::abs(relativeSpeed) < 1.0e-9)
        {
            if (std::abs(separation) > radius)
            {
                return {};
            }
            continue;
        }
        const double enter = (-radius - separation) / relativeSpeed;
        const double leave = (radius - separation) / relativeSpeed;
        start = std::max(start, std::min(enter, leave));
        end = std::min(end, std::max(enter, leave));
        if (start > end)
        {
            return {};
        }
    }
    return {true, start};
}

inline int FleetRiskPriority(FleetRiskKind kind) noexcept
{
    switch (kind)
    {
    case FleetRiskKind::Touching: return 5;
    case FleetRiskKind::SweptContact: return 4;
    case FleetRiskKind::StoppingDistance: return 3;
    case FleetRiskKind::Following: return 2;
    case FleetRiskKind::Clear: return 1;
    default: return 0;
    }
}

// A multi-obstacle forecast must validate the ENTIRE roster before
// granting any propulsion. Rank physical contact first, earliest entry
// second, then stable slot number. A low-severity first car must never
// hide a more dangerous fifth car later in the observation array.
inline FleetRiskReport ForecastSixCarRisk(
    const RivalFleetTrafficSample& sample) noexcept
{
    std::array<std::uint64_t, StartGridCapacity> roster{};
    if (!ValidateSixCarFleet(sample, roster) ||
        !sample.BrakeObservationQualified ||
        !std::isfinite(sample.ConservativeBrakeScale) ||
        sample.ConservativeBrakeScale < 0.15 ||
        sample.ConservativeBrakeScale > 1.0)
    {
        // Never infer maximum deceleration without a qualified real
        // prior driving frame. Missing or excessive grip fails closed.
        return {};
    }
    FleetRiskReport result;
    result.Kind = FleetRiskKind::Clear;
    result.ThrottleCeiling = 1.0;

    constexpr double RadPerDegree = std::numbers::pi / 180.0;
    const double heading = std::remainder(
        sample.Self.Pose.HeadingDegrees, 360.0) * RadPerDegree;
    const double forwardX = std::cos(heading);
    const double forwardY = std::sin(heading);
    if (!std::isfinite(forwardX) || !std::isfinite(forwardY))
    {
        return {};
    }
    for (int i = 0; i < MaxRecoveryObstacles; ++i)
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
        const double closing =
            (sample.Self.VelocityX - other.VelocityX) * forwardX +
            (sample.Self.VelocityY - other.VelocityY) * forwardY;
        if (!std::isfinite(ahead) || !std::isfinite(lateral) ||
            !std::isfinite(closing))
        {
            return {};
        }

        FleetRiskKind kind = FleetRiskKind::Clear;
        double ceiling = 1.0;
        double contactTime = std::numeric_limits<double>::infinity();
        // Never apply a sudden courtesy brake because a car behind us
        // is approaching: its own avoidance envelope owns that rear risk.
        // An actual current collision is different and must stop traction.
        const bool physicalOverlap =
            StartGridFootprintsOverlap(sample.Self.Pose, other.Pose);
        const bool otherAhead = ahead >= -GridPaddedHalfLengthCm;
        const FleetSweptContact swept =
            ForecastFleetContact(sample.Self, other);
        if (physicalOverlap)
        {
            kind = FleetRiskKind::Touching;
            ceiling = 0.0;
            contactTime = 0.0;
        }
        else if (otherAhead && swept.Intersects)
        {
            kind = FleetRiskKind::SweptContact;
            ceiling = 0.0;
            contactTime = swept.FirstContactSeconds;
        }
        else if (otherAhead && ahead > 0.0 &&
                 ahead < FleetForecastMaximumLongitudinalCm &&
                 lateral < FleetForecastLaneHalfWidthCm &&
                 closing > FleetForecastMinimumClosingCmS)
        {
            // Estimated baseline is discounted by actual accepted prior
            // tire wear, heat, surface and damage; never scaled upward.
            // This is not measured Chaos stopping distance or ABS proof.
            const double conservativeDeceleration =
                FleetForecastDecelerationCmS2 *
                sample.ConservativeBrakeScale;
            const double brakingDistance =
                closing * FleetForecastReactionSeconds +
                (closing * closing) /
                    (2.0 * conservativeDeceleration) +
                FleetForecastClearanceCm;
            if (ahead <= brakingDistance)
            {
                kind = FleetRiskKind::StoppingDistance;
                ceiling = 0.0;
                contactTime = ahead / closing;
            }
            else if (ahead < FleetForecastFollowingRangeCm)
            {
                kind = FleetRiskKind::Following;
                const double progress = std::clamp(
                    (ahead - FleetForecastClearanceCm) /
                    (FleetForecastFollowingRangeCm -
                     FleetForecastClearanceCm), 0.0, 1.0);
                ceiling = 0.18 + 0.68 * progress;
            }
        }

        if (kind == FleetRiskKind::Clear)
        {
            continue;
        }
        const int priority = FleetRiskPriority(kind);
        const int previous = FleetRiskPriority(result.Kind);
        // Physical emergencies sort by severity, then earliest contact.
        // Two SOFT following risks have infinite contact time, so sorting
        // those by slot BEFORE the throttle limit could replace a stronger
        // restriction with a weaker one when the blocker array is reordered.
        // Always choose the lower safe motor ceiling first for Following;
        // only identical ceilings may use the stable slot as a tie-break.
        bool replace = priority > previous;
        if (priority == previous)
        {
            if (kind == FleetRiskKind::Following)
            {
                replace = ceiling < result.ThrottleCeiling ||
                    (ceiling == result.ThrottleCeiling &&
                     other.Slot < result.LimitingSlot);
            }
            else
            {
                replace = contactTime < result.TimeToContactSeconds ||
                    (contactTime == result.TimeToContactSeconds &&
                     other.Slot < result.LimitingSlot);
            }
        }
        if (replace)
        {
            result.Kind = kind;
            result.LimitingSlot = other.Slot;
            result.TimeToContactSeconds = contactTime;
            result.ForwardGapCm = ahead;
            result.ClosingSpeedCmS = closing;
            result.ThrottleCeiling = ceiling;
        }
    }
    return result;
}

} // namespace latebrake
