"""Engine-source camera preset bindings, not UE runtime or pixel QA."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
POLICY = ROOT / "Source/Latebrake/Public/Vehicles/CameraPresets.h"
PAWN_H = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
PAWN_CPP = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
LAYOUT = ROOT / "Source/Latebrake/Public/UI/TouchControlLayout.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
CPPTEST = ROOT / "Tests/Standalone/CameraPresetsTests.cpp"


class CameraPresetBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.pawn_h = PAWN_H.read_text(encoding="utf-8")
        cls.pawn = PAWN_CPP.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.layout = LAYOUT.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.test = CPPTEST.read_text(encoding="utf-8")

    def test_four_real_camera_presets_and_unreal_spring_arm_wiring(self):
        for fragment in (
            "CameraView::Chase", "CameraView::CloseChase",
            "CameraView::Hood", "CameraView::High",
            "CameraViewCycle", "IsSafeCameraPreset(",
            "CollisionProbeEnabled", "CameraLagEnabled",
        ):
            self.assertIn(fragment, self.policy)
        self.assertIn('"Vehicles/CameraPresets.h"', self.pawn_h)
        self.assertIn("CameraViews.Current()", self.pawn_h)
        self.assertIn("CameraViews.Next();", self.pawn)
        self.assertIn("ApplyCameraPreset();", self.pawn)
        for fragment in (
            "CameraBoom->TargetArmLength =",
            "CameraBoom->SetRelativeRotation(",
            "CameraBoom->SocketOffset =",
            "CameraBoom->CameraLagSpeed =",
            "CameraBoom->bEnableCameraLag =",
            "CameraBoom->bDoCollisionTest =",
            "IsSafeCameraPreset(Requested)",
        ):
            self.assertIn(fragment, self.pawn)
        self.assertNotIn("bNearCamera", self.pawn + self.pawn_h)
        self.assertIn(
            'BindAction(TEXT("Camera"), IE_Pressed, this, '
            '&ALatebrakeCarPawn::ChangeCamera)', self.pawn)

    def test_android_cam_reuses_actual_press_only_menu_browse_pad(self):
        self.assertIn("TouchRect MenuBrowse;", self.layout)
        self.assertIn("layout.MenuBrowse.Contains(x, y)", self.layout)
        self.assertIn("DrawPad(Pads.MenuBrowse, TEXT(\"CAM\"), false);",
                      self.hud)
        self.assertIn(
            "case latebrake::TouchControl::MenuBrowse:\n"
            "        ChangeCamera();", self.pawn.replace(
                "        // This separate existing upper-middle pad is BROWSE in menus\n"
                "        // but CAM during a live race. It never acquires drive input.\n"
                "        // Only a genuinely new press can change the view.\n", ""))
        self.assertIn(
            "else if (Region == latebrake::TouchControl::MenuBrowse) "
            "CareerNext();", self.pawn)
        self.assertLess(self.pawn.index("if (!bAllowAcquire)"),
                        self.pawn.index("case latebrake::TouchControl::MenuBrowse:\n"))
        self.assertIn("TouchPressAdmission::NewPress", self.pawn)
        self.assertIn("Mode->IsFocusLossGuardActive()", self.pawn)

    def test_live_race_hud_label_after_authoritative_clock(self):
        self.assertIn("CameraViewName(Car->GetCameraView())", self.hud)
        live = self.hud[self.hud.index("const auto Line = [&]"):]
        self.assertLess(
            live.index('TEXT("TIME  %02d:%05.2f")'),
            live.index('TEXT("CAMERA %s | C TO CYCLE")'))
        self.assertIn("if (!bCompact)", live)

    def test_test_target_strict_and_camera_policy_read_only(self):
        for fragment in ("CameraPresetsTests.cpp", "NAME CameraPresets",
                         "cxx_std_20", "/W4 /WX", "-Wpedantic -Werror"):
            self.assertIn(fragment, self.cmake)
        for fragment in ("std::numeric_limits<double>::quiet_NaN()",
                         "cycle.Reset()", "CameraView::High"):
            self.assertIn(fragment, self.test)
        for forbidden in ("AddForce(", "PassCheckpoint(", "SaveGame",
                          "SetAutonomousControls(", "AddTimePenalty("):
            self.assertNotIn(forbidden, self.policy)


if __name__ == "__main__":
    unittest.main()
