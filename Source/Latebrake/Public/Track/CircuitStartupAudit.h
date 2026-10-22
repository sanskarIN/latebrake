#pragma once

#include "Track/CircuitLayout.h"
#include "Track/CircuitCollisionPlan.h"
#include "Track/DecorationBatchPlan.h"

#include <cstdint>

namespace latebrake
{
// Finite, engine-independent startup acceptance policy. A map is NOT ready
// merely because all four gate actors exist: the nine collision-bearing
// ground/road/barrier blocks and seven batches of 93 scenery instances
// must also be accounted for. This reports C++ creation counts; it does
// not prove pixels rendered or physics actually worked on any device.
class CircuitStartupAudit final
{
public:
    static constexpr int RequiredCollisionBlocks = CircuitCollisionBlockCount;
    static constexpr int RequiredSceneryGroups = DecorationBatchCount;
    static constexpr int RequiredSceneryInstances = DecorationPieceCount;

    void RecordCollisionBlock(bool created) noexcept
    {
        ++CollisionAttempts;
        if (created) ++CollisionCreated;
    }

    void RecordCheckpoint(int index, bool created) noexcept
    {
        ++CheckpointAttempts;
        if (!created || !IsCircuitGate(index))
        {
            ++CheckpointFailures;
            return;
        }
        const std::uint32_t bit = 1u << static_cast<unsigned>(index);
        if ((CheckpointMask & bit) != 0u)
        {
            ++CheckpointFailures;
            return;
        }
        CheckpointMask |= bit;
        ++CheckpointsCreated;
    }

    // The rendering code aggregates this data independently of the
    // race gate logic, and cannot authorize readiness by hiding a failure.
    void SetScenery(int groups, int instances) noexcept
    {
        SceneryGroups = groups;
        SceneryInstances = instances;
    }

    bool CollisionReady() const noexcept
    {
        return CollisionAttempts == RequiredCollisionBlocks &&
               CollisionCreated == RequiredCollisionBlocks;
    }

    bool SceneryReady() const noexcept
    {
        return SceneryGroups == RequiredSceneryGroups &&
               SceneryInstances == RequiredSceneryInstances;
    }

    bool CheckpointsReady() const noexcept
    {
        return CheckpointAttempts == CircuitGateCount &&
               CheckpointsCreated == CircuitGateCount &&
               CheckpointFailures == 0 &&
               CheckpointMask == ((1u << CircuitGateCount) - 1u);
    }

    bool Ready() const noexcept
    {
        return CollisionReady() && SceneryReady() && CheckpointsReady();
    }

    int CollisionBlockAttempts() const noexcept { return CollisionAttempts; }
    int CollisionBlockCount() const noexcept { return CollisionCreated; }
    int CheckpointAttemptsCount() const noexcept { return CheckpointAttempts; }
    int CheckpointCount() const noexcept { return CheckpointsCreated; }
    int CheckpointFailuresCount() const noexcept { return CheckpointFailures; }
    int SceneryGroupCount() const noexcept { return SceneryGroups; }
    int SceneryInstanceCount() const noexcept { return SceneryInstances; }

private:
    int CollisionAttempts = 0;
    int CollisionCreated = 0;
    int CheckpointAttempts = 0;
    int CheckpointsCreated = 0;
    int CheckpointFailures = 0;
    int SceneryGroups = 0;
    int SceneryInstances = 0;
    std::uint32_t CheckpointMask = 0;
};
} // namespace latebrake
