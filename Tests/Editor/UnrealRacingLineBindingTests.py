"""Source-level wiring for the optional Unreal starter-circuit racing line.

This is not an Unreal Editor/UHT, material, cooked-map or device test.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
SOURCE = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Track/RacingLineLayout.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
NAV = ROOT / "mkdocs.yml"


class UnrealRacingLineBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.source = SOURCE.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_actual_circuit_spawns_visual_only_line(self):
        self.assertIn("void BuildRacingLine(UStaticMesh* Mesh);", self.header)
        self.assertIn('#include "Track/RacingLineLayout.h"', self.source)
        self.assertIn('#include "Track/CircuitLayout.h"', self.policy)
        build = self.source.split(
            "void ALatebrakeRaceGameMode::BuildCircuit()", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1
        )[0]
        self.assertIn("BuildRacingLine(DecorationMesh);", build)
        self.assertLess(
            build.index("BuildCornerApproachChevrons(DecorationMesh);"),
            build.index("BuildRacingLine(DecorationMesh);"),
        )
        self.assertLess(
            build.index("BuildRacingLine(DecorationMesh);"),
            build.index("// Visual-only pit beacon"),
        )

    def test_visual_actor_cannot_modify_racing_rules(self):
        code = self.source.split(
            "void ALatebrakeRaceGameMode::BuildRacingLine(", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::BuildCornerApproachChevrons(", 1
        )[0]
        for expected in (
            "NewObject<UInstancedStaticMeshComponent>",
            "Holder->SetActorEnableCollision(false);",
            "Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
            "Instances->SetGenerateOverlapEvents(false);",
            "Instances->SetCanEverAffectNavigation(false);",
            "Instances->SetCastShadow(false);",
            "latebrake::RacingLineMarkCount",
            "latebrake::PlannedRacingLineMark(Index)",
            "Mark.HeadingDegrees",
            "Mark.LengthCm / 100.0",
            "Sections[Section]->AddInstance(Pose, false)",
            "LATEBRAKE_RACING_LINE_READY",
            "LATEBRAKE_RACING_LINE_INCOMPLETE",
            "Holder->Destroy();",
        ):
            with self.subTest(expected=expected):
                self.assertIn(expected, code)
        for forbidden in (
            "SpawnBlock(", "Race.PassCheckpoint(", "Race.Advance(",
            "SaveRecords(", "AddTimePenalty(", "Audit.RecordCollisionBlock(",
            "Audit.RecordCheckpoint(", "bCircuitReady =",
            "EarnCredits(", "AddForce(", "SetSimulatePhysics(true)",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, code)

    def test_portable_geometry_and_qa_are_registered(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        self.assertIn("RacingLineLayoutTests.cpp", cmake)
        self.assertIn("NAME RacingLineLayout", cmake)
        self.assertIn("RACING_LINE_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
