"""Source binding only: this does not compile/cook or visually inspect UE 5.8."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealCircuitSurfaceDetailBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.game = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.header = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.plan = read("Source/Latebrake/Public/Track/CircuitSurfaceDetail.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_real_unreal_mode_owns_optional_batched_markings(self):
        self.assertIn('#include "Track/CircuitSurfaceDetail.h"', self.game)
        self.assertIn("void BuildCircuitSurfaceDetail(UStaticMesh* Mesh);",
                      self.header)
        section = self.game.split(
            "void ALatebrakeRaceGameMode::BuildCircuitSurfaceDetail(", 1)[1].split(
            "void ALatebrakeRaceGameMode::BuildCircuit()", 1)[0]
        for keyword in (
            "NewObject<UInstancedStaticMeshComponent>",
            "latebrake::PlannedCircuitSurfaceDetail(Index)",
            "latebrake::IsSurfaceDetailOnPhysicalAsphalt(Detail)",
            "latebrake::CircuitSurfaceDetailCount",
            "latebrake::PlannedSurfaceStyleCount(",
            "LATEBRAKE_SURFACE_DETAIL_READY",
            "LATEBRAKE_SURFACE_DETAIL_INCOMPLETE",
            "Holder->Destroy()",
        ):
            self.assertIn(keyword, section)
        circuit = self.game.split(
            "void ALatebrakeRaceGameMode::BuildCircuit()", 1)[1]
        self.assertIn("BuildCircuitSurfaceDetail(DecorationMesh);", circuit)

    def test_nonblocking_and_not_gameplay_authority(self):
        section = self.game.split(
            "void ALatebrakeRaceGameMode::BuildCircuitSurfaceDetail(", 1)[1].split(
            "void ALatebrakeRaceGameMode::BuildCircuit()", 1)[0]
        for keyword in (
            "Holder->SetActorEnableCollision(false);",
            "Marks->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
            "Marks->SetGenerateOverlapEvents(false);",
            "Marks->SetCanEverAffectNavigation(false);",
            "Marks->SetCastShadow(false);",
        ):
            self.assertIn(keyword, section)
        for forbidden in (
            "SpawnBlock(", "Race.PassCheckpoint(", "SaveRecords(",
            "AddTimePenalty(", "Chassis->AddForce(",
        ):
            self.assertNotIn(forbidden, section)
            self.assertNotIn(forbidden, self.plan)
        self.assertIn("CircuitCollisionPlan.h", self.plan)
        self.assertIn("IsSurfaceDetailOnPhysicalAsphalt", self.plan)

    def test_regression_and_real_engine_review_are_required(self):
        self.assertIn("CircuitSurfaceDetailTests.cpp", self.cmake)
        self.assertIn("NAME CircuitSurfaceDetail", self.cmake)
        self.assertIn("SURFACE_DETAIL_QA.md", read("docs/TRACK.md"))


if __name__ == "__main__":
    unittest.main()
