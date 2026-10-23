"""Verify Unreal's touch callbacks use the portable residual-safety policy.

This reads source only. It does NOT compile Unreal Engine, simulate Chaos,
or establish that Android delivered actual touch events on a real device.
"""
from pathlib import Path
import re
import unittest

PAWN = (
    Path(__file__).resolve().parents[2]
    / "Source" / "Latebrake" / "Private" / "Vehicles"
    / "LatebrakeCarPawn.cpp"
)


def body(source: str, method: str) -> str:
    signature = f"void ALatebrakeCarPawn::{method}("
    if source.count(signature) != 1:
        raise AssertionError(f"Missing or ambiguous {signature}")
    return source.split(signature, 1)[1].split(
        "\nvoid ALatebrakeCarPawn::", 1
    )[0].split("\nbool ALatebrakeCarPawn::", 1)[0]


class TouchResidualBindingContractsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = PAWN.read_text(encoding="utf-8")

    def test_new_safety_header_is_in_real_unreal_pawn(self):
        self.assertIn('#include "UI/TouchResidualSafety.h"', self.source)

    def test_actual_os_release_clears_smooth_command_and_physical_slot(self):
        release = body(self.source, "TouchReleased")
        self.assertRegex(
            release,
            r"latebrake::CancelTouchOwnerAndOutput\s*\(\s*"
            r"TouchDrive\s*,\s*static_cast<int32>\(FingerIndex\)\s*,"
            r"\s*SmoothedThrottle\s*,\s*SmoothedSteering\s*\)",
        )
        self.assertIn("TouchPresses.Release(", release)
        self.assertNotIn("TouchPresses.Reset(", release)
        self.assertLess(
            release.index("CancelTouchOwnerAndOutput"),
            release.index("TouchPresses.Release("),
        )

    def test_touch_move_and_nonfinite_path_clear_stale_output(self):
        apply_touch = body(self.source, "ApplyTouch")
        self.assertRegex(
            apply_touch,
            r"if\s*\(\s*!FMath::IsFinite\(Location.X\)\s*\|\|"
            r"\s*!FMath::IsFinite\(Location.Y\)\s*\)\s*"
            r"\{\s*(?://[^\n]*\n\s*)*"
            r"latebrake::CancelTouchOwnerAndOutput\s*\(",
        )
        self.assertRegex(
            apply_touch,
            r"if\s*\(!bAllowAcquire\)\s*\{\s*"
            r"(?://[^\n]*\n\s*)*"
            r"latebrake::MoveTouchWithResidualSafety\s*\(",
        )
        self.assertIn("SmoothedThrottle, SmoothedSteering", apply_touch)

    def test_regression_fixtures_reject_unprotected_source(self):
        source = self.source.replace(
            "latebrake::CancelTouchOwnerAndOutput(\n"
            "        TouchDrive, static_cast<int32>(FingerIndex),",
            "TouchDrive.Release(\n"
            "        static_cast<int32>(FingerIndex),",
        )
        self.assertNotIn(
            "CancelTouchOwnerAndOutput",
            body(source, "TouchReleased"),
        )
        source = self.source.replace(
            "latebrake::MoveTouchWithResidualSafety(\n"
            "            TouchDrive, Layout, FingerId, Location.X, Location.Y,",
            "TouchDrive.Move(\n"
            "            Layout, FingerId, Location.X, Location.Y,",
        )
        self.assertNotIn(
            "MoveTouchWithResidualSafety", body(source, "ApplyTouch")
        )


if __name__ == "__main__":
    unittest.main()
