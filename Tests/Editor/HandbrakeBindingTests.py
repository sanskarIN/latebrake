"""Source bindings for prototype human handbrake; not Unreal runtime QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class HandbrakeBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = source("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy = source("Source/Latebrake/Public/Vehicles/HandbrakeDrivePolicy.h")
        cls.input = source("Config/DefaultInput.ini")
        cls.hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")

    def test_distinct_braking_actions_keep_old_inputs(self):
        for fragment in ('ActionName="Handbrake",Key=LeftShift',
                         'ActionName="Handbrake",Key=Gamepad_RightThumbstick',
                         'ActionName="Brake",Key=SpaceBar',
                         'ActionName="Brake",Key=Gamepad_FaceButton_Bottom'):
            self.assertIn(fragment, self.input)
        for fragment in ('BindAction(TEXT("Handbrake"), IE_Pressed',
                         'TEXT("Handbrake"), IE_Released',
                         'HandbrakeRelease.bExecuteWhenPaused = true'):
            self.assertIn(fragment, self.pawn)
        self.assertIn('bool IsHandbrakeHeld() const noexcept', self.header)
        self.assertIn('bool bHandbrakeHeld = false;', self.header)

    def test_only_real_human_can_press_when_race_eligible(self):
        section = self.pawn.split('void ALatebrakeCarPawn::HandbrakePressed()', 1)[1].split(
            'void ALatebrakeCarPawn::HandbrakeReleased()', 1)[0]
        for required in ('IsLocallyControlled()',
                         'Cast<APlayerController>(GetController())',
                         'Mode->IsRaceStartGreen()', 'Mode->IsCircuitReadyForGameplay()',
                         'Mode->IsRestartPlacementReady()', '!Mode->IsMenuOpen()',
                         '!Mode->IsFocusLossGuardActive()', 'IsRacePhysicsReady()'):
            self.assertIn(required, section)

    def test_motor_and_real_chassis_damping_use_same_safe_policy(self):
        tick = self.pawn.split('void ALatebrakeCarPawn::Tick(', 1)[1].split(
            'void ALatebrakeCarPawn::ApplySignalLights(', 1)[0]
        for required in ('SelectHandbrakeDriveProfile(', 'BrakeProfile.Throttle',
                         'BrakeProfile.Braking', 'BrakeProfile.LateralGripRate',
                         'BrakeProfile.ForwardResistanceRate',
                         'FrameStableDampingRate(GripRate, DeltaSeconds)',
                         'FrameStableDampingRate(BrakeRate, DeltaSeconds)'):
            self.assertIn(required, tick)
        self.assertLess(tick.index('SelectHandbrakeDriveProfile('),
                        tick.index('latebrake::StabilizeCornerThrottle('))
        self.assertIn('NAME HandbrakeDrivePolicy', self.cmake)
        self.assertIn('HandbrakeDrivePolicyTests.cpp', self.cmake)

    def test_pause_recovery_restart_clear_transient_brake(self):
        clear = self.pawn.split('void ALatebrakeCarPawn::ClearDriverInputs()', 1)[1].split(
            'void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()', 1)[0]
        self.assertIn('bHandbrakeHeld = false;', clear)
        for method, next_method in (
            ('RecoverPressed', 'RestartPressed'),
            ('RestartPressed', 'ApplyCameraPreset'),
            ('TogglePause', 'TouchPressed'),
        ):
            section = self.pawn.split(
                f'void ALatebrakeCarPawn::{method}()', 1)[1].split(
                f'void ALatebrakeCarPawn::{next_method}', 1)[0]
            self.assertIn('HandbrakeReleased();', section)

    def test_one_hud_row_and_no_score_authority(self):
        self.assertIn('Car->IsHandbrakeHeld() && !bCompact', self.hud)
        self.assertIn('SpeedLine + TEXT(" | HANDBRAKE")', self.hud)
        self.assertIn('Line(FString::Printf(TEXT("TIME  %02d:%05.2f")', self.hud)
        for forbidden in ('RaceProgress', 'SaveGame', 'AwardCredits',
                          'PassCheckpoint', 'UCLASS(', 'SetPhysicsLinearVelocity'):
            self.assertNotIn(forbidden, self.policy)


if __name__ == "__main__":
    unittest.main()
