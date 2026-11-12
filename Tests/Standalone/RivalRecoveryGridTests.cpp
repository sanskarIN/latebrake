#include "AI/RivalRecoveryGrid.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "RivalRecoveryGrid FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    static_assert(StartGridCapacity == 6);
    static_assert(CircuitGateCount == 4);
    Require(ArePlannedRivalRecoveryLanesSafe(),
            "all 20 real opponent recovery cells are on asphalt and clear");
    Require(PlannedRivalRecoveryPose(-1, 1).Position.X == 0.0 &&
            PlannedRivalRecoveryPose(0, 0).Position.Y == 0.0 &&
            PlannedRivalRecoveryPose(4, 2).Position.X == 0.0 &&
            PlannedRivalRecoveryPose(2, 6).Position.Y == 0.0,
            "invalid actor/gate never falls back to a real occupied recovery");
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPose original = RivalRecovery(gate);
        const CircuitPose primary = PlannedRivalRecoveryPose(gate, 1);
        Require(original.Position.X == primary.Position.X &&
                original.Position.Y == primary.Position.Y &&
                original.HeadingDegrees == primary.HeadingDegrees,
                "legacy one-rival physical recovery remains exactly unchanged");
        for (int slot = 1; slot < StartGridCapacity; ++slot)
        {
            const CircuitPose pose = PlannedRivalRecoveryPose(gate, slot);
            Require(RecoveryChassisFitsPhysicalAsphalt(pose),
                    "padded chassis completely inside one real asphalt slab");
            Require(EvaluateCircuitSpawnPose(pose) ==
                CircuitSpawnDecision::Safe &&
                !StartGridFootprintTouchesBarrier(pose),
                "planned recovery footprint clears all original barriers");
            for (int prior = 1; prior < slot; ++prior)
            {
                Require(!StartGridFootprintsOverlap(
                    pose, PlannedRivalRecoveryPose(gate, prior)),
                    "two AI actors cannot share overlapping candidate recovery cells");
            }
            if (gate == 0 && slot >= 2)
            {
                const CircuitPose start =
                    StartGridPoses[static_cast<std::size_t>(slot)];
                Require(pose.Position.X == start.Position.X &&
                        pose.Position.Y == start.Position.Y &&
                        pose.HeadingDegrees == start.HeadingDegrees,
                        "gate-zero supplemental recovery retains original lane");
            }
        }
    }

    // Gate 1 is the ascending east segment, gate 2 is the westbound north
    // straight, gate 3 is the descending west segment. All supplemental
    // recovery headings must point along their proper physical road.
    for (int slot = 2; slot < StartGridCapacity; ++slot)
    {
        Require(PlannedRivalRecoveryPose(1, slot).HeadingDegrees == 90.0 &&
                PlannedRivalRecoveryPose(2, slot).HeadingDegrees == 180.0 &&
                PlannedRivalRecoveryPose(3, slot).HeadingDegrees == -90.0,
                "each future rival resumes toward the next actual corner");
    }
    // All four extra AI actors must have a distinct landing when 5 cars
    // simultaneously need recovery at one real gate. Exhaustively test all
    // pairings; these are physical-space checks, not teleport approvals.
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        for (int slotA = 1; slotA < StartGridCapacity; ++slotA)
        {
            for (int slotB = 1; slotB < StartGridCapacity; ++slotB)
            {
                if (slotA == slotB) continue;
                Require(!StartGridFootprintsOverlap(
                    PlannedRivalRecoveryPose(gate, slotA),
                    PlannedRivalRecoveryPose(gate, slotB)),
                    "pairwise candidate footprints remain separated");
            }
        }
    }
    std::cout << "20 distinct next-gate opponent recovery cells passed.\n";
    return EXIT_SUCCESS;
}
