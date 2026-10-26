"""v0.5.74 source contract for human analog deadzone wiring.

This validates actual Unreal Pawn C++ source and portable test registration,
not licensed UE compilation, gamepad device signals or installed gameplay.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
HEADER = ROOT / "Source/Latebrake/Public/Vehicles/DriveAxisDeadzone.h"
INPUT = ROOT / "Config/DefaultInput.ini"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
TEST = ROOT / "Tests/Standalone/DriveAxisDeadzoneTests.cpp"


class DriveAxisDeadzoneBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.input = INPUT.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")

    def test_real_player_callback_applies_distinct_human_filters(self):
        self.assertIn('#include "Vehicles/DriveAxisDeadzone.h"', self.pawn)
        def body(name):
            return self.pawn.split(
                f"void ALatebrakeCarPawn::{name}(", 1
            )[1].split("\nvoid ALatebrakeCarPawn::", 1)[0]
        self.assertIn("FilterHumanThrottleAxis(Value)", body("ThrottleInput"))
        self.assertIn("FilterHumanSteeringAxis(Value)", body("SteerInput"))
        self.assertNotIn("FilterHumanThrottleAxis(", body("SetAutonomousControls"))
        self.assertNotIn("FilterHumanSteeringAxis(", body("SetAutonomousControls"))
        self.assertIn("TouchDrive.ThrottleActive() ? 1.0", self.pawn)
        self.assertIn("TouchDrive.SteeringActive()", self.pawn)

    def test_real_mapping_includes_keyboard_and_signed_triggers(self):
        for line in (
            'AxisName="Throttle",Scale=1.000000,Key=W',
            'AxisName="Throttle",Scale=-1.000000,Key=S',
            'AxisName="Throttle",Scale=1.000000,Key=Gamepad_RightTriggerAxis',
            'AxisName="Throttle",Scale=-1.000000,Key=Gamepad_LeftTriggerAxis',
            'AxisName="Steer",Scale=-1.000000,Key=A',
            'AxisName="Steer",Scale=1.000000,Key=D',
            'AxisName="Steer",Scale=1.000000,Key=Gamepad_LeftX',
        ):
            self.assertIn(line, self.input)

    def test_policy_guards_invalid_inputs_and_no_physics_mutation(self):
        for marker in (
            "SanitizeDriveAxis(rawAxis)", "std::isfinite(rawAxis)",
            "std::isfinite(threshold)", "std::copysign(",
            "FilterHumanSteeringAxis", "FilterHumanThrottleAxis",
        ):
            self.assertIn(marker, self.header)
        for banned in (
            "AddForce(", "SetAutonomousControls(", "PassCheckpoint(",
            "AddTimePenalty(", "SaveGame(", "UCLASS(", "FVector",
        ):
            self.assertNotIn(banned, self.header)

    def test_strict_production_policies_registered(self):
        for marker in (
            "DriveAxisDeadzoneTests.cpp", "NAME DriveAxisDeadzone",
            "cxx_std_20", "/W4 /WX", "-Wpedantic -Werror",
        ):
            self.assertIn(marker, self.cmake)
        for marker in (
            "quiet_NaN()", "double prior = 0.0",
            "small signed trigger jitter", "full authority",
            "underlying autonomous/touch sanitizer remains unchanged",
        ):
            self.assertIn(marker, self.test)


if __name__ == "__main__":
    unittest.main()
