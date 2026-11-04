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
WINDOWS_LAUNCHER = ROOT / "scripts/Run-WindowsPlaytest.ps1"
WINDOWS_QA = ROOT / "docs/WINDOWS_PERSONAL_PLAYTEST.md"
MATERIAL_AUTHOR = ROOT / "scripts/Unreal/AuthorRacingLineMaterial.py"
ENGINE_CONFIG = ROOT / "Config/DefaultEngine.ini"


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
        # A newly constructed map owns exactly one optional guide. Editor
        # re-entry must discard the previous actor and restore default ON.
        self.assertIn("if (IsValid(RacingLineActor))", build)
        self.assertIn("RacingLineActor->Destroy();", build)
        self.assertIn("RacingLineActor = nullptr;", build)
        self.assertIn("bRacingLineVisible = true;", build)
        self.assertLess(
            build.index("RacingLineActor->Destroy();"),
            build.index("BuildRacingLine(DecorationMesh);"),
        )
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
            "Root->IsRegistered()",
            "Instances->IsRegistered()",
            "Instances->GetStaticMesh() != Mesh",
            "Instances->GetCollisionEnabled() != ECollisionEnabled::NoCollision",
            "Sections[0]->GetInstanceCount()",
            "Sections[1]->GetInstanceCount()",
            "!Holder->GetActorEnableCollision()",
            "reason=root_registration",
            "reason=component_registration",
            "Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
            "Instances->SetGenerateOverlapEvents(false);",
            "Instances->SetCanEverAffectNavigation(false);",
            "Instances->SetCastShadow(false);",
            "latebrake::RacingLineMarkCount",
            "latebrake::PlannedRacingLineMark(Index)",
            "LoadObject<UMaterialInterface>",
            "/Game/Materials/M_RacingLineGuide.M_RacingLineGuide",
            "LATEBRAKE_RACING_LINE_MATERIAL authored=%s",
            "Instances->SetMaterial(0, AuthoredMaterial)",
            "latebrake::IsValidRacingLinePaintFootprint(Mark)",
            "reason=off_asphalt index=%d",
            "Mark.HeadingDegrees",
            "Mark.LengthCm / latebrake::RacingLineCubeSizeCm",
            "latebrake::RacingLinePaintCenterZCm",
            "latebrake::RacingLinePaintWidthCm",
            "latebrake::RacingLinePaintThicknessCm",
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
        self.assertIn("bool IsRacingLineAvailable() const", self.header)
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
        self.assertIn("#if !UE_BUILD_SHIPPING", toggle)
        self.assertIn("LATEBRAKE_RACING_LINE_VISIBILITY visible=%s", toggle)
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
        for required in (
            "RacingLineStraightPieceCount",
            "RacingLineCornerPieceCount",
            "int32 AddedStraight = 0;",
            "int32 AddedCorner = 0;",
            "straight=%d corner=%d",
            "Holder->Destroy();",
        ):
            self.assertIn(required, guide)
        self.assertIn(
            "Section == 0 ? latebrake::RacingLineStraightPieceCount",
            guide,
        )
        self.assertIn("RACING LINE %s | L TO TOGGLE", hud)
        self.assertIn("Mode->IsRacingLineVisible()", hud)
        self.assertIn("Mode->IsRacingLineAvailable()", hud)
        self.assertIn("RACING LINE UNAVAILABLE", hud)
        self.assertLess(
            hud.index('TEXT("CAMERA %s | C TO CYCLE")'),
            hud.index('TEXT("RACING LINE %s | L TO TOGGLE")'),
        )

    def test_windows_runtime_receipt_observes_guide_without_qualifying_gameplay(self):
        runner = WINDOWS_LAUNCHER.read_text(encoding="utf-8")
        self.assertIn("[switch]$RequireRacingLineToggle", runner)
        self.assertIn("if ($RequireRacingLineToggle) {", runner)
        self.assertIn("git -C $sourceRoot rev-parse --verify HEAD", runner)
        self.assertIn("$manifestMetadata.source_commit -cne $checkoutSha[0]", runner)
        self.assertIn("Strict guide QA source mismatch", runner)
        self.assertLess(
            runner.index("Strict guide QA source mismatch"),
            runner.index("Launching real candidate:"),
        )
        self.assertIn("$racingLineSourceLogPass = $false", runner)
        self.assertIn("Verify-RacingLineRuntimeLog.py", runner)
        self.assertIn("$guideArgs += '--require-toggle'", runner)
        self.assertIn("$guideArgs += '--require-authored-material'", runner)
        self.assertIn("racing_line_toggle_audit_requested", runner)
        self.assertIn("racing_line_source_log_markers_pass", runner)
        self.assertIn("racing_line_source_log_error", runner)
        self.assertIn("if ($RequireRacingLineToggle -and", runner)
        self.assertIn("(-not $racingLineSourceLogPass -or -not $startupLogMarkersPass)", runner)
        self.assertLess(
            runner.index("$report | ConvertTo-Json"),
            runner.index('throw "Required Win64 guide/track source-log audit failed.'),
        )
        self.assertIn("gameplay_verified = $false", runner)
        # Actual hardware evidence is NOT provided by parser exit status.
        self.assertNotIn("gameplay_verified = $racingLineSourceLogPass", runner)
        self.assertTrue(WINDOWS_QA.is_file())

    def test_authored_material_is_editor_owned_not_a_fabricated_binary(self):
        author = MATERIAL_AUTHOR.read_text(encoding="utf-8")
        config = ENGINE_CONFIG.read_text(encoding="utf-8")
        self.assertIn('ASSET_PATH = f"{ASSET_DIRECTORY}/{ASSET_NAME}"', author)
        self.assertIn('MATUSAGE_INSTANCED_STATIC_MESHES', author)
        self.assertIn('MaterialExpressionVectorParameter', author)
        self.assertIn('MP_BASE_COLOR', author)
        self.assertIn('set_base_material_usage(', author)
        self.assertIn('recompile_material(material)', author)
        self.assertIn('save_loaded_asset(material, False)', author)
        self.assertIn('get_vector_parameter_names(material)', author)
        self.assertIn('asset_subsystem.does_asset_exist(ASSET_PATH)', author)
        self.assertIn('if not engine.startswith("5.8.")', author)
        self.assertIn('main()', author)
        self.assertIn('+DirectoriesToAlwaysCook=(Path="/Game/Materials")', config)
        # A future genuine UE-authored asset must NOT fail source-only CI.
        # Only the licensed Editor serializer, not raw Python bytes, may
        # create the .uasset in the actual project.
        self.assertNotIn("write_bytes(", author)
        self.assertNotIn("open(ASSET_FILE, \"wb\")", author)

    def test_portable_geometry_and_qa_are_registered(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        self.assertIn("RacingLineLayoutTests.cpp", cmake)
        self.assertIn("NAME RacingLineLayout", cmake)
        # Geometry uses the real physical road slabs, not a cosmetic Z
        # constant that silently diverges from the road during development.
        self.assertIn('#include "Track/CircuitCollisionPlan.h"', self.policy)
        self.assertIn("PlannedCollisionBlock(1).Height", self.policy)
        self.assertIn("RacingLineAsphaltTopCm", self.policy)
        self.assertIn("RacingLinePaintSurfaceGapCm", self.policy)
        self.assertIn("RacingLinePaintCenterZCm", self.policy)
        self.assertIn("IsOnPhysicalRacingLineAsphalt(", self.policy)
        self.assertIn("IsValidRacingLinePaintFootprint(", self.policy)
        self.assertIn("PlannedCollisionBlock(index)", self.policy)
        self.assertIn("longitudinal = -2; longitudinal <= 2", self.policy)
        self.assertIn("lateral = -2; lateral <= 2", self.policy)
        self.assertIn("RACING_LINE_QA.md", nav)
        validator = ROOT / "scripts/Verify-RacingLineRuntimeLog.py"
        tests = ROOT / "Tests/Editor/RacingLineRuntimeLogTests.py"
        self.assertTrue(validator.is_file())
        self.assertTrue(tests.is_file())
        self.assertIn("LATEBRAKE_RACING_LINE_READY", validator.read_text(encoding="utf-8"))
        self.assertIn("require_toggle", validator.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
