#include "Track/CircuitStartupAudit.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Startup audit FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void AddAllBlocks(latebrake::CircuitStartupAudit& audit)
{
    for (int i = 0; i < latebrake::CircuitStartupAudit::RequiredCollisionBlocks; ++i)
        audit.RecordCollisionBlock(true);
}

void AddAllGates(latebrake::CircuitStartupAudit& audit)
{
    for (int i = 0; i < latebrake::CircuitGateCount; ++i)
        audit.RecordCheckpoint(i, true);
}
} // namespace

int main()
{
    using namespace latebrake;
    static_assert(CircuitStartupAudit::RequiredSceneryGroups == 7);
    static_assert(CircuitStartupAudit::RequiredSceneryInstances == 93);
    CircuitStartupAudit audit;
    Require(!audit.Ready() && !audit.CollisionReady() &&
            !audit.SceneryReady() && !audit.CheckpointsReady(),
            "default-constructed audit rejects empty map");

    AddAllBlocks(audit);
    Require(audit.CollisionReady() && !audit.Ready(),
            "nine ground/road/barrier blocks alone are insufficient");
    AddAllGates(audit);
    Require(audit.CheckpointsReady() && !audit.Ready(),
            "four correct gate actors alone are insufficient");
    audit.SetScenery(7, 93);
    Require(audit.SceneryReady() && audit.Ready(),
            "all required startup categories pass together");
    Require(audit.CollisionBlockCount() == 9 &&
            audit.CheckpointCount() == 4 &&
            audit.CheckpointFailuresCount() == 0,
            "valid dimensions are preserved");

    CircuitStartupAudit missingRoad;
    for (int i = 0; i < 9; ++i) missingRoad.RecordCollisionBlock(i != 3);
    AddAllGates(missingRoad);
    missingRoad.SetScenery(7, 93);
    Require(!missingRoad.Ready() && !missingRoad.CollisionReady() &&
            missingRoad.CollisionBlockAttempts() == 9 &&
            missingRoad.CollisionBlockCount() == 8,
            "one missing collision block fails even if gates and scenery pass");

    CircuitStartupAudit duplicateBlock;
    AddAllBlocks(duplicateBlock);
    duplicateBlock.RecordCollisionBlock(true);
    AddAllGates(duplicateBlock);
    duplicateBlock.SetScenery(7, 93);
    Require(!duplicateBlock.Ready(),
            "extra created collision block cannot disguise bad configuration");

    CircuitStartupAudit missingGate;
    AddAllBlocks(missingGate);
    for (int i = 0; i < 4; ++i) missingGate.RecordCheckpoint(i, i != 2);
    missingGate.SetScenery(7, 93);
    Require(!missingGate.Ready() && !missingGate.CheckpointsReady(),
            "one failed gate spawn prevents ready marker");

    CircuitStartupAudit wrongGate;
    AddAllBlocks(wrongGate);
    wrongGate.RecordCheckpoint(0, true);
    wrongGate.RecordCheckpoint(1, true);
    wrongGate.RecordCheckpoint(1, true);
    wrongGate.RecordCheckpoint(3, true);
    wrongGate.SetScenery(7, 93);
    Require(!wrongGate.Ready() && wrongGate.CheckpointFailuresCount() == 1,
            "duplicate index cannot impersonate missing checkpoint");
    CircuitStartupAudit invalidGate;
    AddAllBlocks(invalidGate);
    invalidGate.RecordCheckpoint(-1, true);
    invalidGate.RecordCheckpoint(1, true);
    invalidGate.RecordCheckpoint(2, true);
    invalidGate.RecordCheckpoint(3, true);
    invalidGate.SetScenery(7, 93);
    Require(!invalidGate.Ready() && invalidGate.CheckpointCount() == 3,
            "out-of-range gate index is not admitted");

    CircuitStartupAudit poorScenery;
    AddAllBlocks(poorScenery);
    AddAllGates(poorScenery);
    poorScenery.SetScenery(7, 92);
    Require(!poorScenery.Ready() && !poorScenery.SceneryReady(),
            "missing instance blocks startup ready");
    poorScenery.SetScenery(7, 94);
    Require(!poorScenery.Ready(), "extra instance is not the expected asset plan");
    poorScenery.SetScenery(6, 93);
    Require(!poorScenery.Ready(), "missing scenery group blocks startup ready");
    poorScenery.SetScenery(7, 93);
    Require(poorScenery.Ready(), "only exact expected plan passes");

    std::cout << "Startup geometry/gate/scenery audit passed.\n";
    return EXIT_SUCCESS;
}
