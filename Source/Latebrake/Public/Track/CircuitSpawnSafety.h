#pragma once

#include "Track/CircuitCollisionPlan.h"
#include "Track/CircuitLayout.h"

#include <cmath>

namespace latebrake
{
enum class CircuitSpawnDecision
{
    Safe,
    InvalidLocation,
    OffPavement,
    BarrierOverlap
};

// A conservative XY-only preflight for original player/rival starts and
// recovery destinations. Unreal's actual chassis height, tire contact,
// collision channels and physics are NOT verified by this portable test.
inline CircuitSpawnDecision EvaluateCircuitSpawnPose(
    CircuitPose pose, double barrierClearanceCm = 150.0) noexcept
{
    if (!std::isfinite(pose.Position.X) ||
        !std::isfinite(pose.Position.Y) ||
        !std::isfinite(pose.HeadingDegrees) ||
        !std::isfinite(barrierClearanceCm) ||
        barrierClearanceCm < 0.0 || barrierClearanceCm > 10000.0)
    {
        return CircuitSpawnDecision::InvalidLocation;
    }
    if (!IsOnPavedSurface(pose.Position))
        return CircuitSpawnDecision::OffPavement;

    // The stock UE BasicShapes/Cube is 100 cm wide at a scale of 1.0.
    // This verifies planned XY clearance from explicitly collision-bearing
    // barriers; it does not claim a successful real-world collision query.
    for (int i = 0; i < CircuitCollisionBlockCount; ++i)
    {
        const CircuitCollisionBlock block = PlannedCollisionBlock(i);
        if (block.Style != CollisionBlockStyle::Barrier) continue;
        const double halfX = block.ScaleX * 50.0 + barrierClearanceCm;
        const double halfY = block.ScaleY * 50.0 + barrierClearanceCm;
        if (std::abs(pose.Position.X - block.Center.X) <= halfX &&
            std::abs(pose.Position.Y - block.Center.Y) <= halfY)
        {
            return CircuitSpawnDecision::BarrierOverlap;
        }
    }
    return CircuitSpawnDecision::Safe;
}

inline bool AreCircuitSpawnLocationsSafe() noexcept
{
    if (EvaluateCircuitSpawnPose(PlayerStart) != CircuitSpawnDecision::Safe ||
        EvaluateCircuitSpawnPose(RivalStart) != CircuitSpawnDecision::Safe)
        return false;

    for (int i = 0; i < CircuitGateCount; ++i)
    {
        if (EvaluateCircuitSpawnPose(PlayerRecovery(i)) != CircuitSpawnDecision::Safe ||
            EvaluateCircuitSpawnPose(RivalRecovery(i)) != CircuitSpawnDecision::Safe)
            return false;
    }
    return true;
}
} // namespace latebrake
