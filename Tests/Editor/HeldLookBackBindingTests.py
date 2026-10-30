"""Checks real Unreal held rear-view input ownership, not UE rendering."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class HeldLookBackBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.input = source("Config/DefaultInput.ini")
        cls.policy = source("Source/Latebrake/Public/Vehicles/HeldLookBackCamera.h")
        cls.car_h = source("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.car = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")

    def test_hold_bindings_release_even_when_world_is_paused(self):
        self.assertIn('ActionName="LookBack",Key=V', self.input)
        self.assertIn('ActionName="LookBack",Key=Gamepad_LeftShoulder', self.input)
        self.assertIn('BindAction(TEXT("LookBack"), IE_Pressed', self.car)
        self.assertIn('TEXT("LookBack"), IE_Released, this, &ALatebrakeCarPawn::LookBackReleased', self.car)
        self.assertIn("LookBackRelease.bExecuteWhenPaused = true;", self.car)
        self.assertIn("&ALatebrakeCarPawn::LookBackPressed", self.car)
        self.assertIn("&ALatebrakeCarPawn::LookBackReleased", self.car)

    def test_local_race_authority_required_before_turning_camera(self):
        start = self.car.index("void ALatebrakeCarPawn::LookBackPressed()")
        stop = self.car.index("void ALatebrakeCarPawn::LookBackReleased()", start)
        body = self.car[start:stop]
        for marker in (
            "IsLocallyControlled()", "Mode->IsRaceStartGreen()",
            "Mode->IsMenuOpen()", "UGameplayStatics::IsGamePaused(this)",
            "Mode->IsManualRacePauseActive()",
            "Mode->IsFocusLossGuardActive()",
            "Mode->GetRace().IsFinished()",
            "IsRacePhysicsReady()",
            "LookBackCamera.Press(",
            "ApplyCameraPreset();",
        ):
            self.assertIn(marker, body)
        self.assertIn("LookBackCamera.SpringArmYawDegrees()", self.car)
        self.assertIn("CameraBoom->SetRelativeRotation(", self.car)
        self.assertIn("latebrake::HeldLookBackCamera LookBackCamera;", self.car_h)

    def test_all_input_interruptions_clear_held_camera(self):
        clear = self.car.split(
            "void ALatebrakeCarPawn::ClearDriverInputs()", 1
        )[1].split("void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        self.assertIn("LookBackReleased();", clear)
        self.assertIn("ClearDriverInputs();", self.car.split(
            "void ALatebrakeCarPawn::HandleAppInputInterruption()", 1
        )[1].split("void ALatebrakeCarPawn::HandleAppFocusLost()", 1)[0])
        self.assertIn("LookBackReleased();", self.car.split(
            "void ALatebrakeCarPawn::TogglePause()", 1
        )[1].split("void ALatebrakeCarPawn::TouchPressed(", 1)[0])
        self.assertIn("LookBackReleased();", self.car_h)
        self.assertIn("LookBackCamera.Reset();", self.car)

    def test_existing_camera_cycle_and_menu_navigation_stay_intact(self):
        start = self.car.index("void ALatebrakeCarPawn::ChangeCamera()")
        end = self.car.index("void ALatebrakeCarPawn::LookBackPressed()", start)
        cycle = self.car[start:end]
        self.assertIn("CameraViews.Next();", cycle)
        self.assertIn("ApplyCameraPreset();", cycle)
        menu = self.car.split(
            "void ALatebrakeCarPawn::MenuToggle()", 1
        )[1].split("void ALatebrakeCarPawn::MenuNext()", 1)[0]
        self.assertIn("ClearDriverInputs();", menu)
        preset = self.car.split(
            "void ALatebrakeCarPawn::ApplyCameraPreset()", 1
        )[1].split("void ALatebrakeCarPawn::ChangeCamera()", 1)[0]
        self.assertIn("latebrake::GetCameraPreset(CameraViews.Current())", preset)
        self.assertIn("latebrake::IsSafeCameraPreset(Requested)", preset)
        self.assertIn("LookBackCamera.SpringArmYawDegrees()", preset)
        self.assertIn("CameraBoom->bDoCollisionTest = View.CollisionProbeEnabled;", preset)
        self.assertIn("FollowCamera->SetFieldOfView(", preset)

    def test_portable_policy_has_no_gameplay_authority(self):
        for forbidden in (
            "AddForce(", "SetPhysics", "PassCheckpoint(", "SaveGame",
            "UCLASS(", "Award", "World", "GameMode",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("HeldLookBackCameraTests.cpp", self.cmake)
        self.assertIn("NAME HeldLookBackCamera", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)


if __name__ == "__main__":
    unittest.main()
