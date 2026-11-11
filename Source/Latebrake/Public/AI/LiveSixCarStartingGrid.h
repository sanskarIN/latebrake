#pragma once

#include "AI/RivalRecoveryOccupancy.h"
#include "Vehicles/ChassisGroundContactTraction.h"
#include "Vehicles/VehicleRearLampLayout.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// One physical human and five *actual* engine-rigid-body AI cars. These
// are observed immediately before each admitted countdown frame, never
// reconstructed from the static planned start poses. Developer opt-in
// only; normal retail one-rival starts do not call this policy.
inline constexpr double LiveGridMaxPoseErrorCm = 125.0;
inline constexpr double LiveGridMaxHeadingErrorDegrees = 18.0;
inline constexpr double LiveGridMaxPlanarSpeedCmS = 250.0;
// The south straight is the only authored six-car launch lane. Its live
// road top and the actual prototype rigid cube half-height form a narrow,
// auditable vertical envelope, not an invented tire/suspension dimension.
inline constexpr double LiveGridSouthRoadTopCm =
    CircuitCollisionBlocks[1].Height + 50.0 * CircuitCollisionBlocks[1].ScaleZ;
inline constexpr double LiveGridExpectedChassisCenterZCm =
    LiveGridSouthRoadTopCm + VehicleChassisHalfHeightCm;
inline constexpr double LiveGridMaxVerticalDisplacementCm = 45.0;
inline constexpr double LiveGridMaxVerticalSpeedCmS = 100.0;
inline constexpr int LiveGridMinimumGroundProbes = 3;

// Independent, fresh engine-observed vertical/WorldStatic state for each
// existing physical car. A default-constructed entry can NEVER authorize GO.
struct LiveGridVerticalSample final
{
    double ChassisWorldZCm = std::numeric_limits<double>::quiet_NaN();
    ChassisGroundContactSample Ground{};
};

enum class LiveGridFailure
{
    None,
    MissingPhysicalCar,
    InvalidPhysics,
    DuplicateActorIdentity,
    DisplacedFromAssignedSlot,
    MovingBeforeGo,
    UnsafeVerticalPosition,
    InsufficientGroundSupport,
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
    const std::array<LiveGridVerticalSample, StartGridCapacity>& vertical,
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
        // XY occupancy alone cannot prove six cars are actually standing
        // on the original circuit: a body can hover, fall or sit on debris.
        // Use THIS frame's vertical transform and real WorldStatic traces;
        // reject stale, missing or malformed evidence rather than guessing.
        const LiveGridVerticalSample& height =
            vertical[static_cast<std::size_t>(slot)];
        if (!std::isfinite(height.ChassisWorldZCm) ||
            !std::isfinite(height.Ground.ChassisVerticalSpeedCmPerSecond))
        {
            return {LiveGridFailure::InvalidPhysics, slot, -1};
        }
        if (std::abs(height.ChassisWorldZCm -
                     LiveGridExpectedChassisCenterZCm) >
            LiveGridMaxVerticalDisplacementCm)
        {
            return {LiveGridFailure::UnsafeVerticalPosition, slot, -1};
        }
        if (std::abs(height.Ground.ChassisVerticalSpeedCmPerSecond) >
            LiveGridMaxVerticalSpeedCmS)
        {
            return {LiveGridFailure::MovingBeforeGo, slot, -1};
        }
        const ChassisGroundContactOutput support =
            EvaluateChassisGroundContact(height.Ground);
        if (!support.Valid)
        {
            return {LiveGridFailure::InvalidPhysics, slot, -1};
        }
        if (support.SupportingProbes < LiveGridMinimumGroundProbes)
        {
            return {LiveGridFailure::InsufficientGroundSupport, slot, -1};
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
