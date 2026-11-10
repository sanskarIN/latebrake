#pragma once

#include "AI/RivalRecoveryOccupancy.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// One physical human and five *actual* engine-rigid-body AI cars. These
// are observed immediately before each admitted countdown frame, never
// reconstructed from the static planned start poses. Developer opt-in
// only; normal retail one-rival starts do not call this policy.
inline constexpr double LiveGridMaxPoseErrorCm = 125.0;
inline constexpr double LiveGridMaxHeadingErrorDegrees = 18.0;
inline constexpr double LiveGridMaxPlanarSpeedCmS = 250.0;

enum class LiveGridFailure
{
    None,
    MissingPhysicalCar,
    InvalidPhysics,
    DuplicateActorIdentity,
    DisplacedFromAssignedSlot,
    MovingBeforeGo,
    OffPavement,
    BarrierOverlap,
    CarOverlap
};

struct LiveGridValidation final
{
    LiveGridFailure Failure = LiveGridFailure::MissingPhysicalCar;
    int Slot = -1;
    int OtherSlot = -1;
    constexpr bool Safe() const noexcept
    {
        return Failure == LiveGridFailure::None;
    }
};

// Source geometry is a *necessary* guard, not actual UE collision proof.
// In particular an Actor pointer or a stale "ready" flag alone does not
// demonstrate the physical six-car grid still exists at the actual GO
// instant. Reject body displacements, fast motion and pairwise padded
// overlaps from the current engine-observed chassis XY/yaw/velocity.
// Never move cars, steer, brake, score laps, charge recoveries or save.
inline LiveGridValidation EvaluateLiveSixCarStartingGrid(
    const std::array<RecoveryObstacle, StartGridCapacity>& observed,
    int observedCount) noexcept
{
    if (observedCount != StartGridCapacity)
    {
        return {LiveGridFailure::MissingPhysicalCar, -1, -1};
    }
    for (int slot = 0; slot < StartGridCapacity; ++slot)
    {
        const RecoveryObstacle& actor =
            observed[static_cast<std::size_t>(slot)];
        if (!actor.Verified || actor.Slot != slot ||
            actor.ActorToken == 0)
        {
            return {LiveGridFailure::MissingPhysicalCar, slot, -1};
        }
        if (!IsValidStartGridPose(actor.Pose) ||
            !std::isfinite(actor.VelocityX) ||
            !std::isfinite(actor.VelocityY))
        {
            return {LiveGridFailure::InvalidPhysics, slot, -1};
        }
        for (int other = 0; other < slot; ++other)
        {
            const RecoveryObstacle& prior =
                observed[static_cast<std::size_t>(other)];
            if (prior.ActorToken == actor.ActorToken)
            {
                return {LiveGridFailure::DuplicateActorIdentity, slot, other};
            }
        }
        const CircuitPose plan =
            StartGridPoses[static_cast<std::size_t>(slot)];
        const double distance = std::hypot(
            actor.Pose.Position.X - plan.Position.X,
            actor.Pose.Position.Y - plan.Position.Y);
        const double yawError = std::abs(std::remainder(
            actor.Pose.HeadingDegrees - plan.HeadingDegrees, 360.0));
        if (!std::isfinite(distance) ||
            !std::isfinite(yawError) ||
            distance > LiveGridMaxPoseErrorCm ||
            yawError > LiveGridMaxHeadingErrorDegrees)
        {
            return {LiveGridFailure::DisplacedFromAssignedSlot, slot, -1};
        }
        const double speed = std::hypot(
            actor.VelocityX, actor.VelocityY);
        if (!std::isfinite(speed) ||
            speed > LiveGridMaxPlanarSpeedCmS)
        {
            return {LiveGridFailure::MovingBeforeGo, slot, -1};
        }
        if (!StartGridFootprintOnPavement(actor.Pose))
        {
            return {LiveGridFailure::OffPavement, slot, -1};
        }
        if (StartGridFootprintTouchesBarrier(actor.Pose) ||
            EvaluateCircuitSpawnPose(actor.Pose) ==
                CircuitSpawnDecision::BarrierOverlap)
        {
            return {LiveGridFailure::BarrierOverlap, slot, -1};
        }
        for (int other = 0; other < slot; ++other)
        {
            if (StartGridFootprintsOverlap(
                    actor.Pose,
                    observed[static_cast<std::size_t>(other)].Pose))
            {
                return {LiveGridFailure::CarOverlap, slot, other};
            }
        }
    }
    return {LiveGridFailure::None, -1, -1};
}
} // namespace latebrake
