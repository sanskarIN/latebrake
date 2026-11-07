"""Source-only Unreal pawn binding checks; not UE 5.8 or device qualification."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Vehicles/FrameStableDamping.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class FrameStableDampingBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_runtime_uses_grip_and_frame_bounds_for_all_resistance(self):
        self.assertIn('#include "Vehicles/OpposingThrottleBraking.h"', self.pawn)
        start = self.pawn.index("void ALatebrakeCarPawn::Tick(float DeltaSeconds)")
        end = self.pawn.index("void ALatebrakeCarPawn::ApplySignalLights(", start)
        tick = self.pawn[start:end]
        self.assertIn("!IsRacePhysicsReady()", tick)
        self.assertLess(tick.index("!IsRacePhysicsReady()"),
                        tick.index("CalculateOpposingThrottleBraking("))
        for expression in (
            "CalculateOpposingThrottleBraking({",
            "if (!OpposingBraking.Valid)",
            "OpposingBraking.ForwardForce",
            "OpposingBraking.WholeBodyBudgetDeceleration",
        ):
            with self.subTest(expression=expression):
                self.assertIn(expression, tick)
        # Opposite-pedal braking now has a stricter grip/axle/frame ceiling.
        # Service braking and sideways slip each have independent finite
        # force/impulse ceilings, sharing a bounded contact-grip allowance.
        self.assertIn("latebrake::CalculateGripLimitedBraking({", tick)
        self.assertIn("PhysicalBraking.ForwardForce", tick)
        self.assertIn("latebrake::CalculateLateralGripForce({", tick)
        self.assertIn("LateralForce.RightForce", tick)
        self.assertNotIn("FrameStableDampingRate(EffectiveBrakeRate, DeltaSeconds)", tick)
        for forbidden in (
            "LongitudinalSpeed * Mass * 3.f",
            "FrameStableDampingRate(3.0, DeltaSeconds)",
            "Chassis->AddForce(-Forward * LongitudinalSpeed * Mass *",
            "LateralSpeed * Mass * GripRate",
            "LongitudinalSpeed * Mass * BrakeRate",
        ):
            self.assertNotIn(forbidden, tick)

    def test_policy_and_strict_portable_ci_are_connected(self):
        self.assertIn("std::isfinite(perSecondRate)", self.policy)
        self.assertIn("std::isfinite(deltaSeconds)", self.policy)
        self.assertIn("std::min(perSecondRate, 1.0 / deltaSeconds)", self.policy)
        self.assertIn("FrameStableDampingTests.cpp", self.cmake)
        self.assertIn("NAME FrameStableDamping", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)

    def test_unbounded_engine_damping_is_forbidden(self):
        self.assertIn("CalculateOpposingThrottleBraking({", self.pawn)
        self.assertIn("if (!OpposingBraking.Valid)", self.pawn)
        self.assertIn("!OpposingBraking.OpposingThrottle", self.pawn)
        self.assertNotIn("Chassis->AddForce(-Forward * LongitudinalSpeed * Mass *", self.pawn)
        self.assertIn("CalculateGripLimitedBraking({", self.pawn)
        self.assertIn("if (!PhysicalBraking.Valid)", self.pawn)
        self.assertIn("CalculateLateralGripForce({", self.pawn)
        self.assertIn("if (!LateralForce.Valid)", self.pawn)
        self.assertNotIn("FrameStableDampingRate(GripRate, DeltaSeconds)", self.pawn)


if __name__ == "__main__":
    unittest.main()
