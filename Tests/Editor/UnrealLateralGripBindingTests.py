"""Bind actual Unreal player/rival Chaos sideways forces to a finite grip budget.

Source integration only: no Unreal Engine headers or cooked Win64 gameplay.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Vehicles/LateralGripBudget.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
NAV = ROOT / "mkdocs.yml"


class UnrealLateralGripBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.tick = cls.pawn.split(
            "void ALatebrakeCarPawn::Tick(", 1
        )[1].split("void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]

    def test_actual_shared_chaos_chassis_uses_measured_side_slip(self):
        for token in (
            '#include "Vehicles/LateralGripBudget.h"',
            "const FVector Right(-Forward.Y, Forward.X, 0.f);",
            "const float LateralSpeed = FVector::DotProduct(FlatVelocity, Right)",
            "latebrake::CalculateGripLimitedBraking({",
            "latebrake::CalculateLateralGripForce({",
            "static_cast<double>(LateralSpeed)",
            "static_cast<double>(Mass)",
            "BrakeProfile.LateralGripRate",
            "static_cast<double>(GarageGrip) *",
            "TireCondition.GripMultiplier() *",
            "Surface.LateralGripMultiplier *",
            "CollisionDamage.LateralGripMultiplier()",
            "ContactBraking.WholeBodyBudgetDeceleration",
            "bArcade, BrakeProfile.HandbrakeActive",
            "if (!LateralForce.Valid)",
            "RejectOptionalDriveAfterSpring(SpringResponse);",
            "Chassis->AddForce(Right *",
            "static_cast<float>(LateralForce.RightForce)",
        ):
            self.assertIn(token, self.pawn)
        self.assertLess(
            self.tick.index("latebrake::CalculateGripLimitedBraking({"),
            self.tick.index("latebrake::CalculateLateralGripForce({")
        )
        self.assertLess(
            self.tick.index("if (!LateralForce.Valid)"),
            self.tick.index("Chassis->AddForce(Right *")
        )
        self.assertLess(
            self.tick.index("!IsRacePhysicsReady()"),
            self.tick.index("latebrake::CalculateLateralGripForce({")
        )

    def test_handbrake_driver_authority_retained(self):
        self.assertIn(
            "bHandbrakeHeld && IsLocallyControlled() &&",
            self.tick
        )
        self.assertIn(
            "Cast<APlayerController>(GetController()) != nullptr",
            self.tick
        )
        self.assertIn("HandbrakeLateralGripFraction = 0.42;", self.policy)
        self.assertIn("HandbrakeActive", self.policy)
        self.assertIn("GripLimitedBrakingOutput PhysicalBraking", self.tick)
        self.assertIn("PhysicalBraking.ForwardForce", self.tick)
        self.assertIn("CalculateOpposingThrottleBraking({", self.tick)
        self.assertIn("OpposingBraking.WholeBodyBudgetDeceleration", self.tick)
        self.assertNotIn("FrameStableDampingRate(3.0, DeltaSeconds)", self.tick)
        self.assertNotIn("FrameStableDampingRate(GripRate, DeltaSeconds)", self.tick)
        self.assertNotIn("Chassis->AddForce(-Right * LateralSpeed", self.tick)

    def test_policy_not_a_scoring_or_teleport_authority(self):
        for prohibited in (
            "Race.PassCheckpoint(", "SaveRecords(", "EarnCredits(",
            "SetPhysicsLinearVelocity(", "SetActorRotation(",
            "AddForce(", "AddTorqueInRadians(", "GetWorld(",
        ):
            self.assertNotIn(prohibited, self.policy)
        for marker in (
            "std::isfinite(sample.DeltaSeconds)",
            "std::isfinite(sample.LateralCmPerSecond)",
            "std::isfinite(sample.ChassisMassKg)",
            "std::isfinite(sample.GripScale)",
            "std::isfinite(sample.RequestedBrakeDecelerationCmPerSecondSquared)",
            "totalBudget * totalBudget - usedLongitudinal * usedLongitudinal",
            "0.8 * lateral / sample.DeltaSeconds",
        ):
            self.assertIn(marker, self.policy)

    def test_strict_portable_regression_and_site_qa_exist(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        self.assertIn("LateralGripBudgetTests.cpp", cmake)
        self.assertIn("NAME LateralGripBudget", cmake)
        self.assertIn("cxx_std_20", cmake)
        self.assertIn("LATERAL_GRIP_BUDGET_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
