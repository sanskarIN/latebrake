#pragma once

#include "Racing/CircuitGateCrossing.h"

#include <cmath>
#include <cstdint>

namespace latebrake
{
// A positional crossing alone cannot distinguish a genuine short drive
// from a paid recovery/restart teleport. Each authorized discontinuity
// increments a generation counter; a fresh generation never scores.
enum class CheckpointTraceDecision
{
    Crossed,
    InitialBaseline,
    TeleportRebase,
    InvalidPosition,
    GateRejected
};

struct CheckpointTraceResult final
{
    CheckpointTraceDecision Trace = CheckpointTraceDecision::InitialBaseline;
    CircuitGateDecision Gate = CircuitGateDecision::NoForwardPlaneCrossing;

    bool Crossed() const noexcept
    {
        return Trace == CheckpointTraceDecision::Crossed;
    }
};

inline constexpr const char* CheckpointTraceDecisionName(
    CheckpointTraceDecision decision) noexcept
{
    switch (decision)
    {
    case CheckpointTraceDecision::Crossed: return "crossed";
    case CheckpointTraceDecision::InitialBaseline: return "initial-baseline";
    case CheckpointTraceDecision::TeleportRebase: return "teleport-rebase";
    case CheckpointTraceDecision::InvalidPosition: return "invalid-position";
    case CheckpointTraceDecision::GateRejected: return "gate-rejected";
    }
    return "unknown";
}

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

    // Opt-in detail for real-device diagnostics. No heap allocations or
    // engine dependencies; doesn't log every frame or reveal player data.
    CheckpointTraceResult SampleDetailed(
        int gateIndex, std::uint64_t generation,
        CircuitGateSample position,
        double velocityX, double velocityY) noexcept
    {
        if (!std::isfinite(position.XY.X) ||
            !std::isfinite(position.XY.Y) || !std::isfinite(position.Z))
        {
            Reset();
            return {CheckpointTraceDecision::InvalidPosition,
                    CircuitGateDecision::InvalidSample};
        }

        // A sample with corrupt velocity cannot establish a trustworthy
        // motion baseline. Even when its position is finite, preserving it
        // can bridge an unobserved/invalid physics interval on the NEXT
        // frame and falsely credit a gate. Re-arm only after a later
        // finite position + finite velocity sample.
        if (!std::isfinite(velocityX) || !std::isfinite(velocityY))
        {
            Reset();
            return {CheckpointTraceDecision::GateRejected,
                    CircuitGateDecision::InvalidVelocity};
        }

        if (!HasPrevious)
        {
            Previous = position;
            PreviousGeneration = generation;
            HasPrevious = true;
            return {CheckpointTraceDecision::InitialBaseline,
                    CircuitGateDecision::NoForwardPlaneCrossing};
        }

        if (PreviousGeneration != generation)
        {
            Previous = position;
            PreviousGeneration = generation;
            return {CheckpointTraceDecision::TeleportRebase,
                    CircuitGateDecision::NoForwardPlaneCrossing};
        }

        const CircuitGateDecision decision = EvaluateCircuitGatePlane(
            gateIndex, Previous, position, velocityX, velocityY);
        Previous = position;
        return {decision == CircuitGateDecision::Crossed
                    ? CheckpointTraceDecision::Crossed
                    : CheckpointTraceDecision::GateRejected,
                decision};
    }

    // Backward-compatible bool API used by Unreal checkpoint actors.
    bool Sample(int gateIndex, std::uint64_t generation,
                CircuitGateSample position,
                double velocityX, double velocityY) noexcept
    {
        return SampleDetailed(gateIndex, generation, position,
                              velocityX, velocityY).Crossed();
    }

private:
    CircuitGateSample Previous{};
    std::uint64_t PreviousGeneration = 0;
    bool HasPrevious = false;
};
} // namespace latebrake
