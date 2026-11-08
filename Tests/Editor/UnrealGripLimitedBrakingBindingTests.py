"""Source assertions for genuine shared Unreal chassis braking-force use.

These are source bindings, NOT Unreal Engine 5.8 compilation, wheel ABS,
real car physics, a packaged exe or physical hands-on driving validation.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
HEADER = ROOT / "Source/Latebrake/Public/Vehicles/GripLimitedBraking.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
NAV = ROOT / "mkdocs.yml"


class UnrealGripLimitedBrakingBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = PAWN.read_text(encoding="utf-8")
        cls.policy = HEADER.read_text(encoding="utf-8")

    def test_actual_real_unreal_chassis_applies_grip_limited_braking(self):
        tick = self.cpp.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        self.assertIn('#include "Vehicles/GripLimitedBraking.h"', self.cpp)
        for marker in (
            "BrakeHeat.Observe({",
            "TireCondition.Observe({",
            "SurfaceTraction.Observe({",
            "CollisionDamage.LateralGripMultiplier()",
            "latebrake::CalculateGripLimitedBraking({",
            "bServiceBraking, BrakeProfile.HandbrakeActive, bArcade",
            "TireCondition.GripMultiplier() * GarageGrip",
            "Thermal.Valid ? Thermal.ServiceBrakeMultiplier : 1.0",
            "if (!PhysicalBraking.Valid)",
            "RejectOptionalDriveAfterSpring(SpringResponse);",
            "Chassis->AddForce(Forward *",
            "ContactBraking.ForwardForces[Corner]",
        ):
            self.assertIn(marker, tick)
        self.assertLess(
            tick.index("latebrake::CalculateGripLimitedBraking({"),
            tick.index("Chassis->AddForce(Forward *")
        )
        self.assertLess(
            tick.index("if (!PhysicalBraking.Valid)"),
            tick.index("Chassis->AddForce(Forward *")
        )

    def test_coasting_is_road_load_not_pressed_brake(self):
        tick = self.cpp.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        braking = tick.split(
            "// Physically sampled brake force", 1
        )[1].split("// Neutral throttle COASTS", 1)[0]
        self.assertIn("ContactBraking.ForwardForces[Corner]", braking)
        self.assertIn("Chassis->AddForceAtLocation(", braking)
        self.assertIn("WheelRoadContacts[Corner]", braking)
        self.assertNotIn("FrameStableDampingRate(", braking)
        self.assertNotIn("SetPhysicsLinearVelocity(", braking)
        self.assertIn("if (bEffectiveBraking && GroundContact.HasDriveContact)", tick)
        self.assertIn("ContactTractionScale", tick)
        self.assertNotIn(
            "if (bEffectiveBraking || FMath::Abs(EffectiveThrottle)",
            tick
        )
        self.assertIn("RoadLoad.ForceX", tick)
        self.assertIn("RoadLoad.ForceY", tick)
        self.assertIn("latebrake::CalculateLateralGripForce({", tick)
        self.assertIn("ContactBraking.WholeBodyBudgetDeceleration", tick)
        self.assertIn("Chassis->AddForce(Right *", tick)
        self.assertNotIn("Chassis->AddForce(-Right * LateralSpeed", tick)
        self.assertIn("HandbrakeDriveProfile BrakeProfile", tick)

    def test_physics_values_finite_and_non_award_authoritative(self):
        for term in (
            "std::isfinite(s.DeltaSeconds)",
            "s.DeltaSeconds > MaxBrakeFrameSeconds",
            "std::isfinite(s.MassKg)",
            "std::isfinite(s.TireGripMultiplier)",
            "std::isfinite(s.DamageGripMultiplier)",
            "std::isfinite(s.ServiceBrakeHeatMultiplier)",
            "frameLimit =",
            "s.ForwardCmPerSecond",
        ):
            self.assertIn(term, self.policy)
        for forbidden in (
            "Race.PassCheckpoint(", "SaveRecords(", "EarnCredits(",
            "SetActorRotation(", "AddForce(", "AddTorqueInRadians(",
            "SetPhysicsLinearVelocity(",
        ):
            self.assertNotIn(forbidden, self.policy)

    def test_strict_cross_platform_build_and_site_qa_exist(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        self.assertIn("GripLimitedBrakingTests.cpp", cmake)
        self.assertIn("NAME GripLimitedBraking", cmake)
        self.assertIn("GRIP_LIMITED_BRAKING_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
