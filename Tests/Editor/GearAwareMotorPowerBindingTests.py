"""Source binding checks for real player/rival gear-aware motor taper.

These tests do NOT compile Unreal Engine or certify installed game physics.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class GearAwareMotorPowerBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.car = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")
        cls.policy = source("Source/Latebrake/Public/Vehicles/GearAwareMotorPower.h")

    def test_reuses_real_player_rival_chassis_only_after_gates(self):
        tick = self.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        required = (
            "Mode->IsRaceStartGreen()",
            "!IsRacePhysicsReady()",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity()",
            "if (!Surface.Valid)",
            "NitroBoost.Step(",
            "SlipstreamBoost.Observe({",
            "AllocateArcadeBoost(",
            "CollisionDamage.MotorMultiplier()",
            "latebrake::GearAwareMotorPowerFactor(",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
            "const float ActiveSpeedCap = MaxSpeed * CurrentNitroSpeedCapScale;",
            "latebrake::LimitPlanarReverseSpeed(",
        )
        offsets = [tick.index(x) for x in required]
        self.assertEqual(offsets, sorted(offsets))
        self.assertIn('#include "Vehicles/GearAwareMotorPower.h"', self.car)

    def test_all_three_boost_and_base_motor_terms_share_taper(self):
        tick = self.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertEqual(tick.count("* ImpactMotorScale * GearPowerFactor);"), 3)
        self.assertEqual(tick.count(
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *"), 3)
        self.assertIn("CombinedBoost.NitroExtra", tick)
        self.assertIn("DraftExtraForce = CombinedBoost.DraftExtra;", tick)
        self.assertIn("static_cast<double>(EffectiveThrottle)", tick)
        self.assertIn("static_cast<double>(LongitudinalSpeed)", tick)
        self.assertIn("static_cast<double>(MaxSpeed * CurrentNitroSpeedCapScale)", tick)
        self.assertIn("bArcade));", tick)

    def test_braking_still_uses_original_driver_demand_and_damping(self):
        tick = self.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertIn("if (bEffectiveBraking || FMath::Abs(EffectiveThrottle) < 0.02f)", tick)
        self.assertIn("Chassis->AddForce(-Forward * LongitudinalSpeed * Mass *", tick)
        self.assertIn("FrameStableDampingRate(EffectiveBrakeRate, DeltaSeconds)", tick)
        self.assertIn("if (!bEffectiveBraking &&", tick)
        self.assertIn("if (NitroDrive.Active)", tick)

    def test_production_curve_is_finite_and_registered_as_strict_cxx(self):
        self.assertIn("MaximumReverseSpeedCmPerSecond(arcadeHandling)", self.policy)
        self.assertIn("return 1.0 - smoothstep;", self.policy)
        self.assertIn("if (speed >= speedCap)", self.policy)
        self.assertIn("if (!std::isfinite(requestedThrottle)", self.policy)
        self.assertIn("GearAwareMotorPowerTests.cpp", self.cmake)
        self.assertIn("NAME GearAwareMotorPower", self.cmake)
        self.assertIn("latebrake_gear_aware_motor_power_tests PRIVATE /W4 /WX", self.cmake)
        self.assertIn("200000", source("Tests/Standalone/GearAwareMotorPowerTests.cpp"))


if __name__ == "__main__":
    unittest.main()
