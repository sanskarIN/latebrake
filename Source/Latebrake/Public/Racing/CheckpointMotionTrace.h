#pragma once

#include "Racing/CircuitGateCrossing.h"

#include <cmath>
#include <cstdint>

namespace latebrake
{
// The checkpoint plane validator only sees frame-to-frame positions. A
// legitimate RecoverTo() may relocate the player by *less* than the old
// 1200cm jump limit and accidentally cross the gate in the right direction.
//
// Each player-authorized discontinuity increments a separate generation.
// The first sample from a new generation establishes a baseline without
// awarding a checkpoint; only movement WITHIN that generation can count.
// There are no allocations, physics traces or Unreal SDK dependencies.
class CheckpointMotionTrace final
{
public:
    void Reset() noexcept
    {
        HasPrevious = false;
        Previous = {};
        PreviousGeneration = 0;
    }

    bool HasBaseline() const noexcept { return HasPrevious; }

    bool Sample(int gateIndex, std::uint64_t generation,
                CircuitGateSample position,
                double velocityX, double velocityY) noexcept
    {
        if (!std::isfinite(position.XY.X) ||
            !std::isfinite(position.XY.Y) || !std::isfinite(position.Z))
        {
            // A malformed physics position cannot form one end of a
            // future genuine crossing. Rebase on next valid frame.
            Reset();
            return false;
        }

        if (!HasPrevious || PreviousGeneration != generation)
        {
            Previous = position;
            PreviousGeneration = generation;
            HasPrevious = true;
            return false;
        }

        const bool crossed = CrossesCircuitGatePlane(
            gateIndex, Previous, position, velocityX, velocityY);
        Previous = position;
        return crossed;
    }

private:
    CircuitGateSample Previous{};
    std::uint64_t PreviousGeneration = 0;
    bool HasPrevious = false;
};
} // namespace latebrake
