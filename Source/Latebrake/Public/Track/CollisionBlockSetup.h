#pragma once

#include "Track/CircuitCollisionPlan.h"

namespace latebrake
{
// The GameMode may count a collision Actor as successfully prepared only
// after its actual Unreal component state has been checked. These portable
// decisions are allocation-free and carry no engine dependencies.
enum class CollisionBlockSetupDecision
{
    Ready,
    InvalidPlan,
    ActorMissing,
    MeshComponentMissing,
    UnexpectedMesh,
    CollisionNotConfigured,
    TransformNotApplied
};

inline constexpr const char* CollisionBlockSetupDecisionName(
    CollisionBlockSetupDecision decision) noexcept
{
    switch (decision)
    {
    case CollisionBlockSetupDecision::Ready: return "ready";
    case CollisionBlockSetupDecision::InvalidPlan: return "invalid-plan";
    case CollisionBlockSetupDecision::ActorMissing: return "actor-missing";
    case CollisionBlockSetupDecision::MeshComponentMissing: return "component-missing";
    case CollisionBlockSetupDecision::UnexpectedMesh: return "unexpected-mesh";
    case CollisionBlockSetupDecision::CollisionNotConfigured: return "collision-not-configured";
    case CollisionBlockSetupDecision::TransformNotApplied: return "transform-not-applied";
    }
    return "unknown";
}

// All boolean inputs come from a *real* UE actor/component query at runtime;
// this function does not pretend that a C++ test compiled the UE API or
// observed physics, and refuses to count a merely spawned Actor as ready.
inline CollisionBlockSetupDecision EvaluateCollisionBlockSetup(
    CircuitCollisionBlock plan, bool actorExists, bool componentExists,
    bool expectedMeshBound, bool blockingCollisionEnabled,
    bool scaleApplied) noexcept
{
    if (!IsValidCollisionBlock(plan))
        return CollisionBlockSetupDecision::InvalidPlan;
    if (!actorExists)
        return CollisionBlockSetupDecision::ActorMissing;
    if (!componentExists)
        return CollisionBlockSetupDecision::MeshComponentMissing;
    if (!expectedMeshBound)
        return CollisionBlockSetupDecision::UnexpectedMesh;
    if (!blockingCollisionEnabled)
        return CollisionBlockSetupDecision::CollisionNotConfigured;
    if (!scaleApplied)
        return CollisionBlockSetupDecision::TransformNotApplied;
    return CollisionBlockSetupDecision::Ready;
}
} // namespace latebrake
