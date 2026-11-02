"""Source binding for real UE world-space braking references.

This does not run UHT, compile Unreal or render/cook the map.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
GM_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
GM_CPP = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Track/BrakingReferenceBoards.h"


class UnrealBrakingReferenceBoardsBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = GM_H.read_text(encoding="utf-8")
        cls.cpp = GM_CPP.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_real_unreal_mode_calls_original_geometry_plan(self):
        self.assertIn("BuildBrakingReferenceBoards(UStaticMesh* Mesh);", self.header)
        self.assertIn('#include "Track/BrakingReferenceBoards.h"', self.cpp)
        body = self.cpp.split(
            "void ALatebrakeRaceGameMode::BuildBrakingReferenceBoards(", 1)[1].split(
            "void ALatebrakeRaceGameMode::BuildCircuit(", 1)[0]
        for token in (
            "NewObject<UInstancedStaticMeshComponent>(",
            "latebrake::BrakingBoardMarkCount",
            "latebrake::PlannedBrakingReferenceBoard(Index)",
            "Boards->AddInstance(Pose, false)",
            "LATEBRAKE_BRAKING_BOARDS_READY",
            "LATEBRAKE_BRAKING_BOARDS_INCOMPLETE",
            "Holder->Destroy()",
        ):
            self.assertIn(token, body)
        build = self.cpp.split("void ALatebrakeRaceGameMode::BuildCircuit(", 1)[1].split(
            "void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1)[0]
        self.assertIn("BuildBrakingReferenceBoards(DecorationMesh);", build)
        self.assertLess(build.index("BuildBrakingReferenceBoards(DecorationMesh);"),
                        build.index("// Visual-only pit beacon"))

    def test_visual_marker_cannot_block_player_or_change_race_awards(self):
        body = self.cpp.split(
            "void ALatebrakeRaceGameMode::BuildBrakingReferenceBoards(", 1)[1].split(
            "void ALatebrakeRaceGameMode::BuildCircuit(", 1)[0]
        for token in (
            "Holder->SetActorEnableCollision(false);",
            "Boards->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
            "Boards->SetGenerateOverlapEvents(false);",
            "Boards->SetCanEverAffectNavigation(false);",
            "Boards->SetCastShadow(false);",
        ):
            self.assertIn(token, body)
        for prohibited in (
            "SpawnBlock(", "Race.PassCheckpoint(",
            "Race.Advance(", "SaveRecords(", "AddTimePenalty(",
            "EarnCredits(", "TryRecoverTo(",
        ):
            self.assertNotIn(prohibited, body)
            self.assertNotIn(prohibited, self.policy)
        self.assertIn('#include "Track/CircuitLayout.h"', self.policy)

    def test_regressions_are_compiled_and_published(self):
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
        nav = (ROOT / "mkdocs.yml").read_text(encoding="utf-8")
        self.assertIn("BrakingReferenceBoardsTests.cpp", cmake)
        self.assertIn("NAME BrakingReferenceBoards", cmake)
        self.assertIn("UNREAL_BRAKING_REFERENCE_BOARDS_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
