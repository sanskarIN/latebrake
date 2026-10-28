"""Source wiring regression for touch steering center-jitter filtering.

This inspects the real Unreal Pawn's touch route but does not compile UE 5.8
or prove physical Android touch latency, rendering or vehicle response.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class TouchSteeringCenterBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def read(path):
            return (ROOT / path).read_text(encoding="utf-8")

        cls.layout = read("Source/Latebrake/Public/UI/TouchControlLayout.h")
        cls.drive = read("Source/Latebrake/Public/UI/TouchDriveState.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")

    def test_new_press_and_drag_share_the_same_filtered_axis(self):
        self.assertIn("TouchSteeringCenterDeadzone = 0.08", self.layout)
        self.assertIn("const double raw = std::clamp(", self.layout)
        self.assertIn("return std::copysign(std::clamp(output,", self.layout)
        self.assertIn("SteeringAxis = TouchSteeringValue(layout, x);", self.drive)
        self.assertEqual(
            self.drive.count("SteeringAxis = TouchSteeringValue(layout, x);"), 2
        )
        self.assertIn("TouchDrive.Press(Layout, FingerId, Location.X, Location.Y);",
                      self.pawn)
        self.assertIn("latebrake::MoveTouchWithResidualSafety(", self.pawn)
        self.assertIn("TouchDrive.SteeringActive()", self.pawn)
        self.assertIn("TouchDrive.Steering()", self.pawn)

    def test_neutralization_and_device_limits_still_apply(self):
        for marker in (
            "SteeringAxis = 0.0;",
            "Cancel(finger);",
            "if (!layout.Valid)",
            "if (y < layout.Steering.Y || y > layout.Steering.Y + layout.Steering.Height)",
            "PedalState = Pedal::Neutral;",
        ):
            self.assertIn(marker, self.drive)
        residual = (ROOT / "Source/Latebrake/Public/UI/TouchResidualSafety.h").read_text(
            encoding="utf-8"
        )
        self.assertIn("wasSteeringOwner && drive.Steering() == 0.0", residual)
        self.assertIn("smoothedSteering = 0.f;", residual)
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", self.pawn)
        self.assertIn("SmoothedSteering = 0.f;", self.pawn)


if __name__ == "__main__":
    unittest.main()
