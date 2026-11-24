#pragma once

#include "Track/CircuitStartGrid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Bounded observation input for the existing human car, and later up to
// four other physically-present rivals. No engine pointers or heap memory.
// True road collision remains authoritative and requires actual Unreal QA.
inline constexpr int MaxRecoveryObstacles = StartGridCapacity - 1;
inline constexpr double RecoveryTrafficForecastSeconds = 0.4;

struct RecoveryObstacle final
{
    bool Verified = false;
    int Slot = -1;
    std::uint64_t ActorToken = 0;
    CircuitPose Pose{};
    double VelocityX = 0.0;  // genuine world cm/s
    double VelocityY = 0.0;
};

enum class RecoveryClearanceReason
{
    Clear,
    InvalidTarget,
    OffPavement,
    BarrierOverlap,
    UnknownOccupancy,
    TrafficCollision
};

struct RecoveryClearanceResult final
{
    RecoveryClearanceReason Reason = RecoveryClearanceReason::UnknownOccupancy;
    int ObstacleSlot = -1;
    bool Safe() const noexcept
    {
        return Reason == RecoveryClearanceReason::Clear;
    }
};

// The proposed recovery chassis must fit wholly within ONE physical
// existing asphalt slab. A paved centre or four individually paved corners
// are insufficient where separate road rectangles meet around dirt.
inline bool RecoveryChassisFitsPhysicalAsphalt(CircuitPose pose) noexcept
{
    if (!IsValidStartGridPose(pose))
    {
        return false;
    }
    const StartGridRect r = MakeStartGridRect(pose);
    const double extX = StartGridProjectionRadius(r, 1.0, 0.0);
    const double extY = StartGridProjectionRadius(r, 0.0, 1.0);
    for (int i = 0; i < CircuitCollisionBlockCount; ++i)
    {
        const CircuitCollisionBlock road = PlannedCollisionBlock(i);
        if (road.Style != CollisionBlockStyle::Asphalt ||
            !IsValidCollisionBlock(road))
        {
            continue;
        }
        if (std::abs(r.X - road.Center.X) + extX <= road.ScaleX * 50.0 &&
            std::abs(r.Y - road.Center.Y) + extY <= road.ScaleY * 50.0)
        {
            return true;
        }
    }
    return false;
}

// Continuous separating-axis intersection between a *static* recovery
// rectangle and a blocker following its measured linear planar velocity.
// This checks the full 0..0.4s interval, including fast movements between
// discrete rendered frames. Rotational motion is not predicted.
inline bool PredictedRecoveryFootprintsOverlap(
    CircuitPose recoveryPose, CircuitPose blockerPose,
    double blockerVelocityX, double blockerVelocityY) noexcept
{
    if (!IsValidStartGridPose(recoveryPose) ||
        !IsValidStartGridPose(blockerPose) ||
        !std::isfinite(blockerVelocityX) ||
        !std::isfinite(blockerVelocityY) ||
        std::hypot(blockerVelocityX, blockerVelocityY) > 15000.0)
    {
        return true; // Unknown physics must not clear a live teleport.
    }
    const StartGridRect target = MakeStartGridRect(recoveryPose);
    const StartGridRect blocker = MakeStartGridRect(blockerPose);
    const double dx = blocker.X - target.X;
    const double dy = blocker.Y - target.Y;
    double enter = 0.0;
    double leave = RecoveryTrafficForecastSeconds;
    const std::array<CircuitPoint, 4> axes{{
        {target.ForwardX, target.ForwardY},
        {target.RightX, target.RightY},
        {blocker.ForwardX, blocker.ForwardY},
        {blocker.RightX, blocker.RightY}
    }};
    for (const CircuitPoint axis : axes)
    {
        const double distance = dx * axis.X + dy * axis.Y;
        const double speed =
            blockerVelocityX * axis.X + blockerVelocityY * axis.Y;
        const double radius =
            StartGridProjectionRadius(target, axis.X, axis.Y) +
            StartGridProjectionRadius(blocker, axis.X, axis.Y);
        if (std::abs(speed) < 1.0e-9)
        {
            if (std::abs(distance) > radius)
            {
                return false;
            }
            continue;
        }
        const double t0 = (-radius - distance) / speed;
        const double t1 = (radius - distance) / speed;
        enter = std::max(enter, std::min(t0, t1));
        leave = std::min(leave, std::max(t0, t1));
        if (enter > leave)
        {
            return false;
        }
    }
    return true; // Edge contact is unsafe, including time-zero overlap.
}

inline RecoveryClearanceResult EvaluateRivalRecoveryClearance(
    CircuitPose proposed,
    const std::array<RecoveryObstacle, MaxRecoveryObstacles>& obstacles,
    int observedCount) noexcept
{
    if (!IsValidStartGridPose(proposed))
    {
        return {RecoveryClearanceReason::InvalidTarget, -1};
    }
    if (!RecoveryChassisFitsPhysicalAsphalt(proposed))
    {
        return {RecoveryClearanceReason::OffPavement, -1};
    }
    if (StartGridFootprintTouchesBarrier(proposed) ||
        EvaluateCircuitSpawnPose(proposed) ==
            CircuitSpawnDecision::BarrierOverlap)
    {
        return {RecoveryClearanceReason::BarrierOverlap, -1};
    }
    // One actual human player is mandatory; other opponents can be added
    // only after separately verifying every physical actor's existence.
    if (observedCount < 1 || observedCount > MaxRecoveryObstacles)
    {
        return {RecoveryClearanceReason::UnknownOccupancy, -1};
    }
    for (int i = 0; i < observedCount; ++i)
    {
        const RecoveryObstacle& other =
            obstacles[static_cast<std::size_t>(i)];
        if (!other.Verified || other.Slot < 0 ||
            other.Slot >= StartGridCapacity ||
            other.ActorToken == 0 ||
            !IsValidStartGridPose(other.Pose) ||
            !std::isfinite(other.VelocityX) ||
            !std::isfinite(other.VelocityY) ||
            std::hypot(other.VelocityX, other.VelocityY) > 15000.0)
        {
            return {RecoveryClearanceReason::UnknownOccupancy, other.Slot};
        }
        for (int j = 0; j < i; ++j)
        {
            const RecoveryObstacle& prior =
                obstacles[static_cast<std::size_t>(j)];
            if (prior.Slot == other.Slot ||
                prior.ActorToken == other.ActorToken)
            {
                return {RecoveryClearanceReason::UnknownOccupancy, other.Slot};
            }
        }
        if (PredictedRecoveryFootprintsOverlap(
                proposed, other.Pose, other.VelocityX, other.VelocityY))
        {
            return {RecoveryClearanceReason::TrafficCollision, other.Slot};
        }
    }
    return {RecoveryClearanceReason::Clear, -1};
}
} // namespace latebrake
