#include "AI/RivalRecoveryGrid.h"
#include "AI/SingleRivalRecoverySelection.h"

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
    // The real single-rival selector preserves the historical primary,
    // requires one authentic human, and moves only across the same lane.
    RecoveryObstacle human{true,HumanGridSlot,101,
        {{10000.0,10000.0},0.0},0.0,0.0};
    for(int gate=0;gate<CircuitGateCount;++gate)
    {
        const auto primary=RivalRecovery(gate);
        const auto clear=SelectSingleRivalRecovery(gate,human);
        Require(clear.Safe() &&
            clear.Status==SingleRivalRecoveryStatus::PrimaryClear &&
            clear.EvaluatedCandidates==1 && clear.LateralOffsetCm==0.0 &&
            clear.Pose.Position.X==primary.Position.X &&
            clear.Pose.Position.Y==primary.Position.Y,
            "clear original landing wins deterministically");
        auto blocking=human;
        blocking.Pose=primary;
        const auto alternative=SelectSingleRivalRecovery(gate,blocking);
        Require(alternative.Safe() &&
            alternative.Status==SingleRivalRecoveryStatus::LateralClear &&
            alternative.LateralOffsetCm!=0.0 &&
            alternative.PrimaryReason==RecoveryClearanceReason::TrafficCollision,
            "occupied original landing selects a verified lateral alternative");
        Require((gate%2==0
            ? alternative.Pose.Position.X==primary.Position.X
            : alternative.Pose.Position.Y==primary.Position.Y) &&
            alternative.Pose.HeadingDegrees==primary.HeadingDegrees,
            "alternative never advances longitudinal gate progress");
        Require(!PredictedRecoveryFootprintsOverlap(
            alternative.Pose,blocking.Pose,0.0,0.0),
            "fallback landing clears actual human footprint");
        Require(RecoveryChassisFitsPhysicalAsphalt(alternative.Pose) &&
            !StartGridFootprintTouchesBarrier(alternative.Pose),
            "fallback respects the same physical road and walls");
        const auto repeated=SelectSingleRivalRecovery(gate,blocking);
        Require(repeated.LateralOffsetCm==alternative.LateralOffsetCm,
            "same snapshot selects same alternative");
        for(const double offset:SingleRivalRecoveryOffsetsCm)
        {
            CircuitPose pose{};
            Require(TrySingleRivalRecoveryPose(gate,offset,pose),
                "bounded cardinal alternative generated");
            Require(gate%2==0 ? pose.Position.X==primary.Position.X
                : pose.Position.Y==primary.Position.Y,
                "every candidate preserves exact longitudinal coordinate");
        }
        auto crossing=human;
        crossing.Pose=primary;
        if(gate%2==0)
        {
            crossing.Pose.Position.Y-=900.0;
            crossing.VelocityY=4500.0;
        }
        else
        {
            crossing.Pose.Position.X-=900.0;
            crossing.VelocityX=4500.0;
        }
        const auto swept=SelectSingleRivalRecovery(gate,crossing);
        Require(!swept.Safe() || !PredictedRecoveryFootprintsOverlap(
            swept.Pose,crossing.Pose,crossing.VelocityX,crossing.VelocityY),
            "moving human cannot be swept through a selected landing");
        // Broad deterministic scan verifies that an admitted pose is
        // always one of the configured candidates, clear for this exact
        // human position and velocity, and never beyond the original.
        for(int lateral=-1000;lateral<=1000;lateral+=100)
            for(int speed=-3000;speed<=3000;speed+=1000)
            {
                auto observed=human;
                observed.Pose=primary;
                if(gate%2==0)
                {
                    observed.Pose.Position.Y+=lateral;
                    observed.VelocityY=speed;
                }
                else
                {
                    observed.Pose.Position.X+=lateral;
                    observed.VelocityX=speed;
                }
                const auto pick=SelectSingleRivalRecovery(gate,observed);
                Require(pick.EvaluatedCandidates<=5,"bounded search budget");
                if(!pick.Safe()) continue;
                CircuitPose expected{};
                Require(TrySingleRivalRecoveryPose(gate,pick.LateralOffsetCm,expected) &&
                    expected.Position.X==pick.Pose.Position.X &&
                    expected.Position.Y==pick.Pose.Position.Y,
                    "selected pose belongs to the exact candidate set");
                std::array<RecoveryObstacle,MaxRecoveryObstacles> blockers{};
                blockers[0]=observed;
                Require(EvaluateRivalRecoveryClearance(pick.Pose,blockers,1).Safe(),
                    "every selected landing independently passes clearance");
            }
    }
    Require(!SelectSingleRivalRecovery(-1,human).Safe() &&
        !SelectSingleRivalRecovery(4,human).Safe(),"invalid gate fails closed");
    auto invalidHuman=human;invalidHuman.ActorToken=0;
    Require(SelectSingleRivalRecovery(0,invalidHuman).Status==
        SingleRivalRecoveryStatus::UnknownHuman,"identity is mandatory");
    invalidHuman=human;invalidHuman.Slot=1;
    Require(!SelectSingleRivalRecovery(0,invalidHuman).Safe(),
        "another rival cannot substitute for local human");
    invalidHuman=human;invalidHuman.Verified=false;
    Require(!SelectSingleRivalRecovery(0,invalidHuman).Safe(),
        "unverified human is not empty road");
    invalidHuman=human;
    invalidHuman.VelocityX=std::numeric_limits<double>::infinity();
    Require(!SelectSingleRivalRecovery(0,invalidHuman).Safe(),"invalid measured velocity");
    invalidHuman=human;invalidHuman.VelocityY=15001.0;
    Require(!SelectSingleRivalRecovery(0,invalidHuman).Safe(),"implausible measured velocity");
    CircuitPose rejected{};
    Require(!TrySingleRivalRecoveryPose(0,601.0,rejected) &&
        !TrySingleRivalRecoveryPose(0,std::numeric_limits<double>::quiet_NaN(),rejected),
        "invalid offsets never invent recovery positions");
    std::cout << "20 distinct next-gate cells and live single-rival recovery selection passed.\n";
    return EXIT_SUCCESS;
}
