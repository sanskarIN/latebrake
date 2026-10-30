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

    def test_runtime_uses_the_same_bounded_policy_for_all_resistance(self):
        self.assertIn('#include "Vehicles/FrameStableDamping.h"', self.pawn)
        start = self.pawn.index("void ALatebrakeCarPawn::Tick(float DeltaSeconds)")
        end = self.pawn.index("void ALatebrakeCarPawn::ApplySignalLights(", start)
        tick = self.pawn[start:end]
        self.assertIn("!IsRacePhysicsReady()", tick)
        self.assertLess(tick.index("!IsRacePhysicsReady()"),
                        tick.index("FrameStableDampingRate("))
        for expression in (
            "FrameStableDampingRate(3.0, DeltaSeconds)",
            "FrameStableDampingRate(GripRate, DeltaSeconds)",
            "FrameStableDampingRate(EffectiveBrakeRate, DeltaSeconds)",
        ):
            with self.subTest(expression=expression):
                self.assertIn(expression, tick)
        for forbidden in (
            "LongitudinalSpeed * Mass * 3.f",
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

    def test_unbound_damping_is_detectable(self):
        bad = self.pawn.replace(
            "FrameStableDampingRate(EffectiveBrakeRate, DeltaSeconds)", "BrakeRate")
        self.assertNotIn("FrameStableDampingRate(EffectiveBrakeRate, DeltaSeconds)", bad)


if __name__ == "__main__":
    unittest.main()
