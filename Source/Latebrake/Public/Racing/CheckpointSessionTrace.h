#pragma once

#include "Racing/CheckpointMotionTrace.h"

#include <cstdint>

namespace latebrake
{
// Each menu pause/resume is a discontinuity in the player's observed
// movement. The next location is a baseline, never a gate crossing.
// This is deliberately independent of Unreal, allocation-free and in
// addition to the pawn's recovery/teleport generation.
class CheckpointSessionTrace final
{
public:
    void Reset() noexcept
    {
        Trace.Reset();
        SessionRevision = 0;
        HasSession = false;
    }

    bool HasBaseline() const noexcept { return Trace.HasBaseline(); }

    bool IsNewSession(std::uint64_t revision) const noexcept
    {
        return !HasSession || revision != SessionRevision;
    }

    CheckpointTraceResult SampleDetailed(
        int gateIndex, std::uint64_t teleportGeneration,
        std::uint64_t sessionRevision, CircuitGateSample position,
        double velocityX, double velocityY) noexcept
    {
        if (IsNewSession(sessionRevision))
        {
            Trace.Reset();
            SessionRevision = sessionRevision;
            HasSession = true;
        }
        return Trace.SampleDetailed(gateIndex, teleportGeneration,
                                    position, velocityX, velocityY);
    }

private:
    CheckpointMotionTrace Trace;
    std::uint64_t SessionRevision = 0;
    bool HasSession = false;
};
} // namespace latebrake
