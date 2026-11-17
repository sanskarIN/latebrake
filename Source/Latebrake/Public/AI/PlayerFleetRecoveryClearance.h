#pragma once

#include "AI/RivalRecoveryOccupancy.h"

#include <array>
#include <cstddef>

namespace latebrake
{
// Opt-in experimental physical six-car recovery policy, NEVER a shipping
// player-recovery fee or teleport policy. The caller provides FIVE genuinely
// observed opponent bodies, in fixed grid order 1..5, from the current
// game-thread snapshot. Slot zero is the human and must not be a blocker
// during its own teleport.
//
// Uses the SAME bounded 0.4s swept OBB contact test already used by AI
// recovery; a missing car, inconsistent identity, or malformed physics
// cannot be treated as empty road. This is a preflight: Unreal's actual
// TryRecoverTo result is still required by the existing paid transaction.
inline constexpr int PlayerFleetRecoveryRequiredRivals =
    StartGridCapacity - 1;

inline RecoveryClearanceResult EvaluatePlayerFleetRecoveryClearance(
    CircuitPose proposed,
    const std::array<RecoveryObstacle, MaxRecoveryObstacles>& rivals,
    int observedCount) noexcept
{
    if (observedCount != PlayerFleetRecoveryRequiredRivals)
    {
        return {RecoveryClearanceReason::UnknownOccupancy, -1};
    }
    // The actual runtime collector emits one primary plus four distinct
    // supplemental actors in 1..5 order. Requiring ALL five specifically
    // prevents a stale, duplicate or missing rival from authorizing a
    // speculative paid player teleport.
    for (int i = 0; i < PlayerFleetRecoveryRequiredRivals; ++i)
    {
        const RecoveryObstacle& other =
            rivals[static_cast<std::size_t>(i)];
        if (!other.Verified || other.Slot != i + PrimaryRivalGridSlot ||
            other.ActorToken == 0)
        {
            return {RecoveryClearanceReason::UnknownOccupancy, other.Slot};
        }
    }
    return EvaluateRivalRecoveryClearance(
        proposed, rivals, PlayerFleetRecoveryRequiredRivals);
}
} // namespace latebrake
