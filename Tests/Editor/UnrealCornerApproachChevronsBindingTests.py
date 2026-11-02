"""Actual Unreal GameMode binding for optional corner-approach decals.

Source policy verification only; never asserts a successful UE/UHT build,
material render, hardware run or playable packaged game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODE_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
MODE_CPP = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Track/CornerApproachChevrons.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
NAV = ROOT / "mkdocs.yml"


class UnrealCornerApproachChevronsBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = MODE_H.read_text(encoding="utf-8")
        cls.cpp = MODE_CPP.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_real_game_mode_consumes_original_turn_plan(self):
        self.assertIn(
            "void BuildCornerApproachChevrons(UStaticMesh* Mesh);", self.header
        )
        self.assertIn('#include "Track/CornerApproachChevrons.h"', self.cpp)
        self.assertIn('#include "Track/CircuitLayout.h"', self.policy)
        build = self.cpp.split(
            "void ALatebrakeRaceGameMode::BuildCircuit()", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1
        )[0]
        self.assertIn("BuildCornerApproachChevrons(DecorationMesh);", build)
        self.assertLess(
            build.index("BuildBrakingReferenceBoards(DecorationMesh);"),
            build.index("BuildCornerApproachChevrons(DecorationMesh);")
        )
        self.assertLess(
            build.index("BuildCornerApproachChevrons(DecorationMesh);"),
            build.index("// Visual-only pit beacon")
        )

    def test_optional_actor_is_cosmetic_and_fail_closed(self):
        body = self.cpp.split(
            "void ALatebrakeRaceGameMode::BuildCornerApproachChevrons(", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::BuildCircuit()", 1
        )[0]
        for marker in (
            "NewObject<UInstancedStaticMeshComponent>(",
            "Holder->SetActorEnableCollision(false);",
            "Marks->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
            "Marks->SetGenerateOverlapEvents(false);",
            "Marks->SetCanEverAffectNavigation(false);",
            "Marks->SetCastShadow(false);",
            "latebrake::CornerChevronPieceCount",
            "latebrake::PlannedCornerApproachChevron(Index)",
            "Marks->AddInstance(Pose, false)",
            "LATEBRAKE_CORNER_CHEVRONS_READY",
            "LATEBRAKE_CORNER_CHEVRONS_INCOMPLETE",
            "Holder->Destroy();",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, body)
        for forbidden in (
            "SpawnBlock(", "Race.PassCheckpoint(",
            "Race.Advance(", "SaveRecords(", "AddTimePenalty(",
            "Audit.RecordCollisionBlock(", "Audit.RecordCheckpoint(",
            "bCircuitReady =", "EarnCredits(",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, body)

    def test_compiled_geometry_and_site_documentation(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        self.assertIn("CornerApproachChevronsTests.cpp", cmake)
        self.assertIn("NAME CornerApproachChevrons", cmake)
        self.assertIn("UNREAL_CORNER_APPROACH_CHEVRONS_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
