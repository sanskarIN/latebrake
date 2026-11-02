"""Actual UE pawn bindings for the prototype speed-aware yaw controller.

Does not establish Chaos gameplay feel, cooked Windows builds or real tire
physics. Only verifies that the Unreal pawn consumes the portable controller.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Vehicles/SpeedAwareYawTorque.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
NAV = ROOT / "mkdocs.yml"


class UnrealSpeedAwareYawTorqueBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_real_chaos_chassis_uses_shared_yaw_rate_target(self):
        self.assertIn('#include "Vehicles/SpeedAwareYawTorque.h"', self.pawn)
        self.assertIn(
            "static_cast<float>(latebrake::SteeringChassisAngularDamping)",
            self.pawn
        )
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        for token in (
            "Chassis->GetPhysicsAngularVelocityInRadians()",
            "latebrake::CalculateSpeedAwareYawTorque({",
            "static_cast<double>(LongitudinalSpeed)",
            "static_cast<double>(AngularVelocity.Z)",
            "static_cast<double>(AssistedSteering)",
            "bArcade, Surface.OffPavement",
            "if (!PlannedYaw.Valid)",
            "NeutralizeMotionWhenDrivingBlocked();",
            "static_cast<float>(PlannedYaw.TorqueZ)",
            "Chassis->AddTorqueInRadians(FVector(",
            "TurnTorque * ContactTractionScale",
        ):
            with self.subTest(token=token):
                self.assertIn(token, tick)
        self.assertLess(
            tick.index("CalculateSpeedAwareYawTorque({"),
            tick.index("latebrake::AllocateArcadeBoost(")
        )
        self.assertLess(
            tick.index("if (!PlannedYaw.Valid)"),
            tick.index("Chassis->AddTorqueInRadians(")
        )

    def test_handbrake_drift_and_existing_speed_limits_retained(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertIn("const float TurnTorque = BrakeProfile.HandbrakeActive", tick)
        self.assertIn("(bArcade ? 21000000.f : 14000000.f)", tick)
        self.assertIn("latebrake::LimitSteeringAtSpeed(", tick)
        self.assertIn("latebrake::LimitPlanarReverseSpeed(", tick)
        self.assertIn("Chassis->SetPhysicsLinearVelocity(", tick)
        self.assertIn("SteeringArcadeLateralAcceleration = 1176.0;", self.policy)
        self.assertIn("SteeringSimulationLateralAcceleration = 784.0;", self.policy)
        self.assertIn("SteeringTorqueLimitPerMass = 105000.0;", self.policy)

    def test_race_authority_is_still_separate(self):
        self.assertIn("CanApplyPlayerDrive({", self.pawn)
        for forbidden in (
            "Race.PassCheckpoint(", "Race.Advance(", "AddTimePenalty(",
            "SaveRecords(", "EarnCredits(",
            "SetActorRotation(", "SetPhysicsAngularVelocity",
        ):
            self.assertNotIn(forbidden, self.policy)

    def test_cross_platform_tests_and_qa_documented(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        self.assertIn("SpeedAwareYawTorqueTests.cpp", cmake)
        self.assertIn("NAME SpeedAwareYawTorque", cmake)
        self.assertIn("SPEED_AWARE_YAW_STEERING_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
