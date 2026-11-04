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
PAWN_H = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
PAWN_CPP = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
INPUT = ROOT / "Config/DefaultInput.ini"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"


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

    def test_local_player_controls_optional_guide_without_race_mutation(self):
        pawn_h = PAWN_H.read_text(encoding="utf-8")
        pawn_cpp = PAWN_CPP.read_text(encoding="utf-8")
        mapped = INPUT.read_text(encoding="utf-8")
        hud = HUD.read_text(encoding="utf-8")
        self.assertIn("void ToggleRacingLineView();", self.header)
        self.assertIn("bool IsRacingLineVisible() const", self.header)
        self.assertIn("TObjectPtr<AActor> RacingLineActor;", self.header)
        self.assertIn("bool bRacingLineVisible = true;", self.header)
        self.assertIn("void ToggleRacingLine();", pawn_h)
        self.assertEqual(
            mapped.count('+ActionMappings=(ActionName="RacingLineView",Key=L)'),
            1,
        )
        self.assertEqual(
            pawn_cpp.count(
                'BindAction(TEXT("RacingLineView"), IE_Pressed, this, '
                '&ALatebrakeCarPawn::ToggleRacingLine)'
            ), 1,
        )
        input_method = pawn_cpp.split(
            "void ALatebrakeCarPawn::ToggleRacingLine()", 1
        )[1].split("void ALatebrakeCarPawn::ToggleBestLapGhost()", 1)[0]
        self.assertIn("IsLocallyControlled()", input_method)
        self.assertIn("Mode->ToggleRacingLineView();", input_method)
        toggle = self.source.split(
            "void ALatebrakeRaceGameMode::ToggleRacingLineView()", 1
        )[1].split("void ALatebrakeRaceGameMode::BuildRacingLine(", 1)[0]
        for guard in (
            "!IsValid(RacingLineActor)", "!bCircuitReady",
            "!HasLocalPlayerDriver()", "Menu.IsOpen()",
            "AppFocusGuard.IsSuspended()", "ManualPause.IsSuspended()",
            "Race.IsFinished()", "UGameplayStatics::IsGamePaused(this)",
        ):
            self.assertIn(guard, toggle)
        self.assertIn(
            "RacingLineActor->SetActorHiddenInGame(!bRacingLineVisible);",
            toggle,
        )
        for forbidden in (
            "Race.Advance(", "Race.PassCheckpoint(", "SaveRecords(",
            "AddForce(", "EarnCredits(", "RegisterCheckpoint(",
        ):
            self.assertNotIn(forbidden, toggle)
        guide = self.source.split(
            "void ALatebrakeRaceGameMode::BuildRacingLine(", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::BuildCornerApproachChevrons(", 1
        )[0]
        self.assertIn("RacingLineActor = Holder;", guide)
        self.assertIn("RACING LINE %s | L TO TOGGLE", hud)
        self.assertIn("Mode->IsRacingLineVisible()", hud)
        self.assertLess(
            hud.index('TEXT("CAMERA %s | C TO CYCLE")'),
            hud.index('TEXT("RACING LINE %s | L TO TOGGLE")'),
        )

    def test_portable_geometry_and_qa_are_registered(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        self.assertIn("RacingLineLayoutTests.cpp", cmake)
        self.assertIn("NAME RacingLineLayout", cmake)
        self.assertIn("RACING_LINE_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
