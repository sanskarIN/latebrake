#pragma once

#include "AI/RivalRecoveryOccupancy.h"

#include <array>
#include <cstddef>

namespace latebrake
{
// Original circuit, exact Unreal centimetres. Slot 0 is the HUMAN and must
// never be selected as an autonomous recovery target. Each row corresponds
// to the *next* real directional gate (0..3); columns 1..5 are distinct
// physical AI cars. Slot 1 preserves the historical single AI recovery.
//
// Additional slots use two lanes and two staggered rows along the segment
// leading to their next gate, so recovering a stalled car does not jump it
// across the circuit to the unrelated start/finish straight. This remains
// only a SOURCE preflight, not real Chaos placement clearance.
inline constexpr std::array<std::array<CircuitPose, StartGridCapacity>,
                            CircuitGateCount> RivalRecoveryGridPoses{{
    std::array<CircuitPose, StartGridCapacity>{{
        {}, RivalRecovery(0), StartGridPoses[2], StartGridPoses[3],
        StartGridPoses[4], StartGridPoses[5]
    }},
    std::array<CircuitPose, StartGridCapacity>{{
        {}, RivalRecovery(1),
        {{2500.0, -1600.0}, 90.0},
        {{3100.0, -1600.0}, 90.0},
        {{2500.0, -2200.0}, 90.0},
        {{3100.0, -2200.0}, 90.0}
    }},
    std::array<CircuitPose, StartGridCapacity>{{
        {}, RivalRecovery(2),
        {{1900.0, 1500.0}, 180.0},
        {{1900.0, 2100.0}, 180.0},
        {{1300.0, 1500.0}, 180.0},
        {{1300.0, 2100.0}, 180.0}
    }},
    std::array<CircuitPose, StartGridCapacity>{{
        {}, RivalRecovery(3),
        {{-3100.0, 1000.0}, -90.0},
        {{-2500.0, 1000.0}, -90.0},
        {{-3100.0, 400.0}, -90.0},
        {{-2500.0, 400.0}, -90.0}
    }}
}};

constexpr CircuitPose PlannedRivalRecoveryPose(
    int nextGate, int gridSlot) noexcept
{
    // Invalid slots/gates return {0,0}: off asphalt and denied by the
    // existing final recovery clearance rule, NOT an occupied fallback.
    return IsCircuitGate(nextGate) &&
        gridSlot >= PrimaryRivalGridSlot &&
        gridSlot < StartGridCapacity
        ? RivalRecoveryGridPoses[static_cast<std::size_t>(nextGate)]
                                [static_cast<std::size_t>(gridSlot)]
        : CircuitPose{};
}

inline bool ArePlannedRivalRecoveryLanesSafe() noexcept
{
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        for (int slot = PrimaryRivalGridSlot;
             slot < StartGridCapacity; ++slot)
        {
            const CircuitPose p = PlannedRivalRecoveryPose(gate, slot);
            if (!RecoveryChassisFitsPhysicalAsphalt(p) ||
                StartGridFootprintTouchesBarrier(p) ||
                EvaluateCircuitSpawnPose(p) != CircuitSpawnDecision::Safe)
            {
                return false;
            }
            for (int earlier = PrimaryRivalGridSlot;
                 earlier < slot; ++earlier)
            {
                if (StartGridFootprintsOverlap(
                    p, PlannedRivalRecoveryPose(gate, earlier)))
                {
                    return false;
                }
            }
        }
    }
    return true;
}
} // namespace latebrake
