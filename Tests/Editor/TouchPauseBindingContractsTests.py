"""Source-binding policy checks, NOT an Unreal compilation or Android test.

Portable TouchPressTracker tests prove what a release callback does; this
static contract confirms that the real Unreal pawn actually binds the
callback to execute when the world is paused. Only licensed UE Editor and
physical devices can verify that the engine delivers every OS touch event.
"""

import re
import unittest
from pathlib import Path

PAWN_SOURCE = (
    Path(__file__).resolve().parents[2]
    / "Source" / "Latebrake" / "Private" / "Vehicles" / "LatebrakeCarPawn.cpp"
)

RELEASE_WHILE_PAUSED = re.compile(
    r"FInputTouchBinding\s*&\s*(\w+)\s*=\s*"
    r"PlayerInputComponent->BindTouch\s*\(\s*IE_Released\s*,"
    r"\s*this\s*,\s*&ALatebrakeCarPawn::TouchReleased\s*\)\s*;"
    r"\s*\1\.bExecuteWhenPaused\s*=\s*true\s*;",
    re.DOTALL,
)

PRESS_WHILE_PAUSED = re.compile(
    r"FInputTouchBinding\s*&\s*(\w+)\s*=\s*"
    r"PlayerInputComponent->BindTouch\s*\(\s*IE_Pressed\s*,"
    r"\s*this\s*,\s*&ALatebrakeCarPawn::TouchPressed\s*\)\s*;"
    r"\s*\1\.bExecuteWhenPaused\s*=\s*true\s*;",
    re.DOTALL,
)


class TouchPauseBindingContractsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = PAWN_SOURCE.read_text(encoding="utf-8")

    def test_pressed_and_released_callbacks_execute_while_paused(self):
        self.assertRegex(self.source, PRESS_WHILE_PAUSED)
        self.assertRegex(self.source, RELEASE_WHILE_PAUSED)

    def test_actual_release_callback_frees_press_tracker(self):
        # Runtime policy tests would be meaningless if Unreal never
        # forwarded the release to the same physical-touch tracker.
        method = self.source.split(
            "void ALatebrakeCarPawn::TouchReleased(", 1
        )[1].split("\nvoid ALatebrakeCarPawn::", 1)[0]
        self.assertIn("TouchDrive.Release(", method)
        self.assertIn("TouchPresses.Release(", method)
        self.assertNotIn("TouchPresses.Reset(", method)

    def test_cannot_accidentally_remove_paused_release_flag(self):
        self.assertIsNone(
            RELEASE_WHILE_PAUSED.search(
                self.source.replace(
                    "ReleasedTouch.bExecuteWhenPaused = true;",
                    "ReleasedTouch.bExecuteWhenPaused = false;",
                )
            )
        )

    def test_input_binding_checks_do_not_accept_wrong_event(self):
        altered = self.source.replace(
            "IE_Released, this, &ALatebrakeCarPawn::TouchReleased",
            "IE_Repeat, this, &ALatebrakeCarPawn::TouchReleased",
        )
        self.assertIsNone(RELEASE_WHILE_PAUSED.search(altered))


if __name__ == "__main__":
    unittest.main()
