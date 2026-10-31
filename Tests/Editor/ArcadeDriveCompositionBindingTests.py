"""Protect the actual single-player/rival arcade force-composition contract.

These are source binding checks, NOT Unreal compilation or installed driving QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class ArcadeDriveCompositionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.car = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = source("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")
        cls.guide = source("docs/ARCADE_DRIVE_COMPOSITION_QA.md")

    def test_real_chassis_gate_precedes_effects(self):
        tick = self.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        markers = (
            "!IsRacePhysicsReady()",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity()",
            "if (!FMath::IsFinite(Speed)",
            "SurfaceTraction.Observe({",
            "if (!Surface.Valid)",
            "CollisionDamage.Advance(",
            "TireCondition.Observe({",
            "BrakeHeat.Observe({",
            "StabilizeCornerThrottle(",
            "NitroBoost.Step(",
            "EngineHeat.Observe({",
            "SlipstreamBoost.Observe({",
            "AllocateArcadeBoost(",
            "CollisionDamage.MotorMultiplier()",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
        )
        self.assertTrue(all(m in tick for m in markers))
        self.assertEqual([tick.index(m) for m in markers],
                         sorted(tick.index(m) for m in markers))

    def test_all_three_motor_paths_share_surface_and_damage(self):
        tick = self.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertEqual(tick.count(
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *"), 3)
        self.assertEqual(tick.count(
            "static_cast<float>(Surface.MotorMultiplier)"), 3)
        self.assertEqual(tick.count("* ImpactMotorScale"), 3)
        self.assertEqual(tick.count("* EngineMotorScale"), 3)
        self.assertIn("CombinedBoost.NitroExtra", tick)
        self.assertIn("DraftExtraForce = CombinedBoost.DraftExtra;", tick)
        self.assertIn("CollisionDamage.LateralGripMultiplier()", tick)
        self.assertIn("TireCondition.GripMultiplier() * Surface.LateralGripMultiplier", tick)
        self.assertIn("FrameStableDampingRate(GripRate, DeltaSeconds)", tick)
        self.assertIn("FrameStableDampingRate(EffectiveBrakeRate, DeltaSeconds)", tick)
        self.assertIn("Thermal.ServiceBrakeMultiplier", tick)
        self.assertIn("if (!bEffectiveBraking &&", tick)

    def test_lifecycle_cannot_grant_free_repair_or_corrupt_running(self):
        self.assertIn("TireCondition.Reset(); BrakeHeat.Reset(); "
                      "EngineHeat.Reset(); CollisionDamage.Reset(); "
                      "SurfaceTraction.Reset();", self.header)
        for member in ("TireCondition", "BrakeHeat", "EngineHeat",
                       "CollisionDamage", "SurfaceTraction"):
            self.assertIn(f"{member}.Suspend();", self.car)
        self.assertIn("ClearDriverInputs();", self.car)
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", self.car)

    def test_portable_composition_regression_is_registered(self):
        self.assertIn("ArcadeDriveCompositionTests.cpp", self.cmake)
        self.assertIn("NAME ArcadeDriveComposition", self.cmake)
        self.assertIn("latebrake_arcade_drive_composition_tests PRIVATE /W4 /WX",
                      self.cmake)
        self.assertIn("200000", source(
            "Tests/Standalone/ArcadeDriveCompositionTests.cpp"))
        self.assertIn("not an installed Unreal 5.8 test", self.guide)


if __name__ == "__main__":
    unittest.main()
