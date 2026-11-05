"""Protect Android discrete actions from a first-touch viewport resize event.

Source binding only: does NOT compile Unreal Engine 5.8, simulate OS touch
delivery, verify gamepause, or qualify an APK on physical Android hardware.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
POLICY = (ROOT / "Source/Latebrake/Public/UI/TouchViewportSafety.h").read_text(encoding="utf-8")
TEST = (ROOT / "Tests/Standalone/TouchViewportActionEpochTests.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
DOC = (ROOT / "docs/TOUCH_CONTROLS.md").read_text(encoding="utf-8")


class AndroidViewportActionEpochBindingTests(unittest.TestCase):
    def test_real_touch_policy_runs_before_every_menu_pause_race_action(self):
        source = PAWN.split(
            "void ALatebrakeCarPawn::ApplyTouch(", 1
        )[1].split("void ALatebrakeCarPawn::TouchReleased(", 1)[0]
        changed = source.index("latebrake::NeutralizeTouchOnViewportChange(")
        admitted = source.index(
            "latebrake::SuppressFreshTouchOnInvalidatedViewport("
        )
        hit = source.index("latebrake::HitTestTouchControl(")
        focus = source.index("if (Mode && Mode->IsFocusLossGuardActive())")
        menu = source.index("if (Mode && Mode->IsMenuOpen())")
        paused = source.index("if (UGameplayStatics::IsGamePaused(this)")
        restart = source.index("case latebrake::TouchControl::Restart:")
        recover = source.index("case latebrake::TouchControl::Recover:")
        self.assertLess(changed, admitted)
        self.assertLess(admitted, hit)
        self.assertLess(hit, focus)
        self.assertLess(hit, menu)
        self.assertLess(admitted, paused)
        self.assertLess(admitted, restart)
        self.assertLess(admitted, recover)
        self.assertIn(
            "bAllowAcquire, bViewportInvalidated)", source
        )
        gate = source[admitted:hit]
        self.assertIn("return;", gate)
        self.assertNotIn("TouchPresses.Reset(", source)
        self.assertNotIn("TouchPresses.Release(", source)

    def test_physically_down_action_press_is_not_rearmed(self):
        pressed = PAWN.split(
            "void ALatebrakeCarPawn::TouchPressed(", 1
        )[1].split(
            "void ALatebrakeCarPawn::TouchMoved(", 1
        )[0]
        self.assertIn("latebrake::AdmitTouchPress(", pressed)
        self.assertLess(pressed.index("AdmitTouchPress("),
                        pressed.index("ApplyTouch(FingerIndex, Location, true)"))
        self.assertIn("return freshPress && viewportInvalidated;", POLICY)
        self.assertIn("touch.Clear();", POLICY)
        self.assertIn("smoothedThrottle = 0.f;", POLICY)
        self.assertIn("smoothedSteering = 0.f;", POLICY)
        self.assertNotIn("presses.Reset(", POLICY)
        self.assertNotIn("TouchPresses.Reset(", POLICY)

    def test_100k_source_suite_and_real_device_qa_are_documented(self):
        self.assertIn("NAME TouchViewportActionEpoch", CMAKE)
        self.assertIn("TouchViewportActionEpochTests.cpp", CMAKE)
        self.assertIn("100000", TEST)
        self.assertIn("TouchControl::PitService", TEST)
        self.assertIn("portrait.Recover", TEST)
        self.assertIn("TouchPressAdmission::Duplicate", TEST)
        self.assertIn("first post-rotate RECOVER tap", TEST)
        self.assertIn("viewport-action epoch", DOC.lower())
        self.assertIn("physical Android", DOC)


if __name__ == "__main__":
    unittest.main()
