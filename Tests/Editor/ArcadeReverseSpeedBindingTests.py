"""Protect real-player and physical-rival reverse-gear speed cap bindings.

Source-only assertions. They cannot compile UnrealHeaderTool, cook the map,
prove Chaos force behavior, or replace installed Windows/Android playtests.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


class ReverseSpeedBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.car = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = read("Source/Latebrake/Public/Vehicles/ArcadeReverseSpeedLimit.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.tests = read("Tests/Standalone/ArcadeReverseSpeedLimitTests.cpp")

    def test_shared_actual_chassis_callback_runs_after_chaos_and_race_gates(self):
        tick = self.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        markers = (
            "Mode->IsRaceStartGreen()",
            "!IsRacePhysicsReady()",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity()",
            "if (!Surface.Valid)",
            "const float ActiveSpeedCap = MaxSpeed * CurrentNitroSpeedCapScale;",
            "FlatVelocity.GetClampedToMaxSize(ActiveSpeedCap)",
            "const FVector PostCapVelocity = Chassis->GetPhysicsLinearVelocity();",
            "latebrake::LimitPlanarReverseSpeed(",
            "if (!ReverseLimit.Valid)",
            "NeutralizeMotionWhenDrivingBlocked();",
            "if (ReverseLimit.Limited)",
        )
        self.assertEqual([tick.index(marker) for marker in markers],
                         sorted(tick.index(marker) for marker in markers))
        self.assertIn('#include "Vehicles/ArcadeReverseSpeedLimit.h"', self.car)

    def test_reverse_clamp_uses_real_world_yaw_and_preserves_vertical(self):
        tick = self.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        for marker in (
            "const FVector Forward = GetActorForwardVector();",
            "static_cast<double>(PostCapVelocity.X)",
            "static_cast<double>(PostCapVelocity.Y)",
            "static_cast<double>(Forward.X)",
            "static_cast<double>(Forward.Y), bArcade)",
            "static_cast<float>(ReverseLimit.VelocityX)",
            "static_cast<float>(ReverseLimit.VelocityY)",
            "PostCapVelocity.Z",
        ):
            self.assertIn(marker, tick)
        self.assertEqual(tick.count("LimitPlanarReverseSpeed("), 1)
        self.assertIn("return {true, false, velocityX, velocityY};", self.header)
        self.assertIn("velocityX + unitForwardX * excessReverse", self.header)
        self.assertIn("velocityY + unitForwardY * excessReverse", self.header)

    def test_no_free_forward_speed_or_award_paths(self):
        self.assertIn("MaximumReverseSpeedCmPerSecond(bool arcadeHandling)", self.header)
        self.assertIn("arcadeHandling ? 1350.0 : 1000.0", self.header)
        self.assertNotIn("SaveGame", self.header)
        self.assertNotIn("RaceRewards", self.header)
        self.assertNotIn("Nitro", self.header)
        self.assertNotIn("Throttle", self.header)
        self.assertIn("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle", self.car)
        self.assertIn("CurrentNitroSpeedCapScale", self.car)

    def test_registered_on_all_platforms_with_bounds_stress(self):
        self.assertIn("ArcadeReverseSpeedLimitTests.cpp", self.cmake)
        self.assertIn("NAME ArcadeReverseSpeedLimit", self.cmake)
        self.assertIn("latebrake_arcade_reverse_speed_tests PRIVATE /W4 /WX", self.cmake)
        self.assertIn("200000", self.tests)
        self.assertIn("std::abs(outputLateral - lateral)", self.tests)


if __name__ == "__main__":
    unittest.main()
