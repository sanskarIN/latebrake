"""No Unreal runtime here: check real ghost-toggle wiring and guards."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class GhostDisplayBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def read(path):
            return (ROOT / path).read_text(encoding="utf-8")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.mode_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.inputs = read("Config/DefaultInput.ini")
        cls.cpp = read("Source/Latebrake/Public/Racing/GhostDisplayPreference.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_keyboard_gamepad_press_only_and_no_collision_with_existing_actions(self):
        self.assertIn('+ActionMappings=(ActionName="GhostView",Key=G)', self.inputs)
        self.assertIn('+ActionMappings=(ActionName="GhostView",Key=Gamepad_FaceButton_Left)', self.inputs)
        self.assertIn('+ActionMappings=(ActionName="Camera",Key=C)', self.inputs)
        self.assertIn('BindAction(TEXT("GhostView"), IE_Pressed, this, '
                      '&ALatebrakeCarPawn::ToggleBestLapGhost)', self.pawn)
        self.assertIn('void ToggleBestLapGhost();', self.pawn_h)
        handler = self.pawn.split('void ALatebrakeCarPawn::ToggleBestLapGhost()', 1)[1].split(
            'void ALatebrakeCarPawn::TogglePause()', 1
        )[0]
        self.assertIn('Mode->ToggleBestLapGhostView()', handler)
        for forbidden in ('ClearDriverInputs', 'RecoverTo', 'AddForce',
                          'RestartRace', 'SetAutonomousControls'):
            self.assertNotIn(forbidden, handler)

    def test_only_authorized_actual_human_in_race_can_toggle(self):
        toggle = self.mode.split('void ALatebrakeRaceGameMode::ToggleBestLapGhostView()', 1)[1].split(
            'void ALatebrakeRaceGameMode::UpdateBestLapGhostMarker(', 1
        )[0]
        for required in (
            'bCircuitReady', 'StartLights.IsGreen()', '!Race.IsFinished()',
            '!Menu.IsOpen()', '!UGameplayStatics::IsGamePaused(this)',
            '!AppFocusGuard.IsSuspended()', '!ManualPause.IsSuspended()',
            'HasLocalPlayerDriver()', 'GhostDisplay.Toggle(bHumanInputAllowed)',
            '!GhostDisplay.IsEnabled()', 'SetActorHiddenInGame(true)',
        ):
            self.assertIn(required, toggle)
        for forbidden in ('GhostRecorder.ResetRun()', 'GhostRecorder.StartLap()',
                          'Race.Advance(', 'Race.Reset()', 'SaveRecords()',
                          'Garage.EarnCredits(', 'TryRecoverTo('):
            self.assertNotIn(forbidden, toggle)

    def test_render_gate_and_visual_status_both_use_same_preference(self):
        renderer = self.mode.split(
            'void ALatebrakeRaceGameMode::UpdateBestLapGhostMarker(', 1
        )[1].split('AStaticMeshActor* ALatebrakeRaceGameMode::SpawnBlock(', 1)[0]
        self.assertIn('GhostDisplay.CanRender(bReplayAuthorized)', renderer)
        self.assertIn('BestLapGhostMarker->SetActorHiddenInGame(true)', renderer)
        self.assertIn('IsBestLapGhostDisplayEnabled()', self.mode_h)
        self.assertIn('Mode->IsBestLapGhostDisplayEnabled()', self.hud)
        self.assertIn('BEST-LAP GHOST HIDDEN | G SHOW', self.hud)
        self.assertIn('BEST-LAP GHOST %.2fs | G HIDE', self.hud)
        self.assertIn('GhostDisplayPreference GhostDisplay;', self.mode_h)
        self.assertIn('bool CanRender(bool replayEligible) const noexcept', self.cpp)
        self.assertIn('NAME GhostDisplayPreference', self.cmake)


if __name__ == '__main__':
    unittest.main()
