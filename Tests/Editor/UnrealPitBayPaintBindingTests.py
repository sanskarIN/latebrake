"""Unreal source-only checks for cosmetic ground stripes inside the real pit.

Does not compile UE5.8, render static-mesh instances or prove that the
built-in parent material accepts the optional Color vector parameter.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealPitBayPaintBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.plan = read("Source/Latebrake/Public/Track/PitBayPaintLayout.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_painted_ring_uses_the_single_authoritative_pit_point(self):
        self.assertIn('#include "Racing/RacePitService.h"', self.plan)
        self.assertIn("RacePitBay.X + offset.X", self.plan)
        self.assertIn("RacePitBay.Y + offset.Y", self.plan)
        self.assertIn("PitBayPaintMarkCount == 8", self.plan)
        self.assertIn("NAME PitBayPaint", self.cmake)
        self.assertIn("PitBayPaintTests.cpp", self.cmake)

    def test_world_decor_never_changes_collision_or_checkpoint_readiness(self):
        body = self.mode.split(
            "// Optional visual outline for the ACTUAL physical pit service bay.",
            1
        )[1].split("int32 SpawnedCheckpoints = 0;", 1)[0]
        for wanted in (
            "AActor* PaintHolder", "UInstancedStaticMeshComponent* Marks",
            "Marks->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
            "Marks->SetGenerateOverlapEvents(false);",
            "Marks->SetCanEverAffectNavigation(false);",
            "Marks->SetCastShadow(false);",
            "Marks->PreAllocateInstancesMemory(",
            "latebrake::PitBayPaintMarkCount",
            "latebrake::PlannedPitBayPaintMark(Index)",
            "Marks->AddInstance(Transform, false)",
            "LATEBRAKE_PIT_BAY_PAINT_READY",
            "LATEBRAKE_PIT_BAY_PAINT_INCOMPLETE",
        ):
            self.assertIn(wanted, body)
        for forbidden in (
            "Audit.RecordCollisionBlock(", "Audit.RecordCheckpoint(",
            "bCircuitReady =", "SpawnBlock(", "SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)",
            "SetSimulatePhysics(true)", "SaveRecords(",
            "EarnCredits(", "RacePitService.Observe(", "RacePitServiceHoldSeconds =",
        ):
            self.assertNotIn(forbidden, body)
        self.assertIn("latebrake::CircuitStartupAudit Audit;", self.mode)
        self.assertIn("bCircuitReady = Audit.Ready() && bSpawnLayoutSafe;", self.mode)

    def test_pit_markers_do_not_depend_on_runtime_actor_position(self):
        body = self.mode.split("AActor* PaintHolder", 1)[1].split(
            "int32 SpawnedCheckpoints", 1)[0]
        self.assertIn("FVector(Mark.Center.X, Mark.Center.Y, Mark.Height)", body)
        self.assertIn("static_cast<float>(Mark.YawDegrees)", body)
        self.assertNotIn("GetActorLocation()", body)
        self.assertNotIn("GetCheckpointIndex(", body)


if __name__ == "__main__":
    unittest.main()
