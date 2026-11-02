"""Official UE GameMode source binding for cosmetic 3D start gantry.

Only the portable policy is compiled in CI. These assertions do not run UE
5.8, UHT, cook a map, simulate real lights, or qualify the Windows installer.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODE_CPP = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
MODE_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
POLICY = ROOT / "Source/Latebrake/Public/Racing/StartGantryVisual.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class UnrealStartGantryBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = MODE_CPP.read_text(encoding="utf-8")
        cls.header = MODE_H.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_uses_real_unreal_actor_and_existing_phase(self):
        build = self.cpp.split("void ALatebrakeRaceGameMode::BuildStartGantry()", 1)[1].split(
            "void ALatebrakeRaceGameMode::UpdateStartGantryVisual()", 1)[0]
        for text in (
            '"/Engine/BasicShapes/Cube.Cube"', '"/Engine/BasicShapes/Sphere.Sphere"',
            "GetWorld()->SpawnActor<AActor>", "AddInstanceComponent",
            "UPointLightComponent", "SetLightColor(", "SetAttenuationRadius(",
            "SetVisibility(false);", "StartGantryLamps.Add(Light);",
            "StartGantryBulbs.Add(Bulb);",
        ):
            self.assertIn(text, build)
        self.assertNotIn("SpawnBlock(", build)
        self.assertIn("TObjectPtr<AActor> StartGantryActor;", self.header)
        self.assertIn("TArray<TObjectPtr<UPointLightComponent>> StartGantryLamps;", self.header)
        self.assertIn("TArray<TObjectPtr<UStaticMeshComponent>> StartGantryBulbs;", self.header)

    def test_visuals_do_not_obstruct_player_or_authorize_scoring(self):
        build = self.cpp.split("void ALatebrakeRaceGameMode::BuildStartGantry()", 1)[1].split(
            "void ALatebrakeRaceGameMode::UpdateStartGantryVisual()", 1)[0]
        for phrase in (
            "Gantry->SetActorEnableCollision(false);",
            "Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
            "Mesh->SetGenerateOverlapEvents(false);",
            "Light->SetCastShadows(false);",
            "FVector(-1700.0, latebrake::SouthY, 0.0)",
        ):
            self.assertIn(phrase, build)
        for forbidden in ("Race.Advance(", "Race.PassCheckpoint(", "SaveRecords(",
                          "Garage.EarnCredits(", "StartLights.Advance(", "StartLights.Reset()"):
            self.assertNotIn(forbidden, self.policy)
            self.assertNotIn(forbidden, build)

    def test_only_authoritative_phase_activates_lamps(self):
        update = self.cpp.split("void ALatebrakeRaceGameMode::UpdateStartGantryVisual()", 1)[1].split(
            "void ALatebrakeRaceGameMode::BuildCircuit()", 1)[0]
        for word in ("PlanStartGantryVisual(", "StartLights.Phase()",
                     "HasLocalPlayerDriver()", "IsRestartPlacementReady()",
                     "Race.IsFinished()", "Menu.IsOpen()",
                     "AppFocusGuard.IsSuspended()", "ManualPause.IsSuspended()",
                     "UGameplayStatics::IsGamePaused(this)",
                     "SetVisibility(bOn)", "NewMask == LastStartGantryLampMask"):
            self.assertIn(word, update)
        mode_tick = self.cpp.split("void ALatebrakeRaceGameMode::Tick(", 1)[1].split(
            "void ALatebrakeRaceGameMode::SuspendBestLapGhost()", 1)[0]
        self.assertLess(mode_tick.index("StartLights.Advance("),
                        mode_tick.index("UpdateStartGantryVisual();"))
        self.assertLess(mode_tick.index("UpdateStartGantryVisual();"),
                        mode_tick.index("AdvancePlayerRaceClockIfReady("))
        begin = self.cpp.split("void ALatebrakeRaceGameMode::BeginPlay()", 1)[1].split(
            "void ALatebrakeRaceGameMode::StartRaceAuditAttempt", 1)[0]
        self.assertIn("BuildStartGantry();", begin)

    def test_pause_restart_and_focus_loss_hide_stale_signals(self):
        for entry, next_entry in (
            ("CloseMenuForRace", "ToggleMenu"),
            ("ToggleMenu", "ToggleRacePause"),
            ("ToggleRacePause", "PauseForAppInterruption"),
            ("PauseForAppInterruption", "MenuNext"),
            ("RestartRace", "RecoverPlayer")
        ):
            code = self.cpp.split(
                "void ALatebrakeRaceGameMode::" + entry + "(", 1)[1].split(
                "void ALatebrakeRaceGameMode::" + next_entry + "(", 1)[0]
            self.assertIn("UpdateStartGantryVisual();", code)
        self.assertIn("StartLights.Reset();", self.cpp)
        self.assertIn("if (!circuitReady || !playerAndGridReady || suspended || raceFinished)",
                      self.policy)

    def test_policy_has_compiled_test_and_published_manual_qa(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        self.assertIn("StartGantryVisualTests.cpp", cmake)
        self.assertIn("add_test(NAME StartGantryVisual", cmake)
        self.assertTrue((ROOT / "docs/UNREAL_START_GANTRY_QA.md").is_file())
        self.assertIn("UNREAL_START_GANTRY_QA.md",
                      (ROOT / "mkdocs.yml").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
