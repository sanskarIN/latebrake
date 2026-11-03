"""Unreal source contracts for load-specific rear propulsion and front steering.

All assertions inspect *actual pawn source*, not an engine stub. They do not
establish licensed Unreal 5.8 UHT compilation, Chaos handling, or installed
Win64 gameplay on real hardware.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class UnrealAxleTractionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(
            encoding="utf-8"
        )
        cls.tick = cls.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        cls.policy = (ROOT / "Source/Latebrake/Public/Vehicles/AxleTractionBudget.h").read_text(
            encoding="utf-8"
        )
        cls.test = (ROOT / "Tests/Standalone/AxleTractionBudgetTests.cpp").read_text(
            encoding="utf-8"
        )
        cls.cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(
            encoding="utf-8"
        )

    def test_same_real_springs_determine_separate_front_rear_traction(self):
        tick = self.tick
        for token in (
            '#include "Vehicles/AxleTractionBudget.h"',
            "latebrake::CalculateSprungChassis(SpringSample)",
            "latebrake::ConstrainGroundContactToSuspension(",
            "latebrake::CalculateAxleTractionBudget(GroundContact, SpringResponse)",
            "if (!AxleBudget.Valid)",
            "AxleBudget.RearDriveTractionScale",
            "AxleBudget.FrontSteeringTractionScale",
            "const float RearDriveTractionScale =",
            "const float FrontSteeringTractionScale =",
            "ApplySprungForces(SpringResponse, WheelWorldAnchors);",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.pawn)
        self.assertLess(
            tick.index("CalculateSprungChassis(SpringSample)"),
            tick.index("CalculateAxleTractionBudget(GroundContact, SpringResponse)")
        )
        self.assertLess(
            tick.index("CalculateAxleTractionBudget(GroundContact, SpringResponse)"),
            tick.index("const float RearDriveTractionScale =")
        )

    def test_all_propulsion_and_boost_paths_require_rear_load(self):
        tick = self.tick
        for token in (
            "bHumanDriver && AxleBudget.RearDriveReady",
            "bEffectiveBraking || !AxleBudget.RearDriveReady",
            "if (bDraftTargetReady && AxleBudget.RearDriveReady)",
            "bArcade) * TransmissionFrame.PropulsionFactor *\n            RearDriveTractionScale);",
            "if (AxleBudget.RearDriveReady &&",
            "else if (AxleBudget.RearDriveReady && !bEffectiveBraking)",
            "RearDriveTractionScale);",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
            "CombinedBoost.NitroExtra",
            "static_cast<float>(DraftExtraForce)",
        ):
            with self.subTest(token=token):
                self.assertIn(token, tick)
        self.assertLess(
            tick.index("if (!AxleBudget.Valid)"),
            tick.index("NitroBoost.Step(")
        )
        self.assertLess(
            tick.index("if (!AxleBudget.Valid)"),
            tick.index("Chassis->AddForce(Forward * DriveForce")
        )
        # No positive motor or extra boost may use the old front-only
        # two-wheel ground-contact predicate.
        self.assertNotIn("if (bDraftTargetReady && GroundContact.HasDriveContact)", tick)

    def test_actual_yaw_uses_front_load_but_brakes_still_use_four_corners(self):
        tick = self.tick
        self.assertIn(
            "0.f, 0.f, TurnTorque * FrontSteeringTractionScale));", tick
        )
        self.assertIn(
            "static_cast<float>(LateralForce.RightForce) *\n        ContactTractionScale);",
            tick
        )
        self.assertIn(
            "static_cast<float>(PhysicalBraking.ForwardForce) *\n            ContactTractionScale);",
            tick
        )
        self.assertIn(
            "if (bEffectiveBraking && GroundContact.HasDriveContact)",
            tick
        )
        self.assertLess(
            tick.index("CalculateAxleTractionBudget(GroundContact, SpringResponse)"),
            tick.index("Chassis->AddTorqueInRadians(")
        )

    def test_policy_has_no_engine_or_game_progress_dependencies(self):
        for token in (
            "OneDrivenWheelGripFraction = 0.40",
            "OneSteeringWheelGripFraction = 0.45",
            "RearDriveTractionScale",
            "FrontSteeringTractionScale",
            "FrontAxleLoadRatio",
            "RearAxleLoadRatio",
            "ground.HasDriveContact !=",
            "ground.SupportingProbes != springs.ContactCount",
            "sum > springs.WeightForce",
            "i < 2", "rearContactFactor",
            "frontContactFactor",
            "RearDriveReady = ground.HasDriveContact",
            "FrontSteeringReady = ground.HasDriveContact",
            "std::min(", "std::clamp(",
        ):
            self.assertIn(token, self.policy)
        for forbidden in (
            "UCLASS(", "GetWorld(", "AddForce(", "AddTorque(",
            "SaveGame(", "PassCheckpoint(", "AwardCredits(", "Http",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("AxleTractionBudgetTests.cpp", self.cmake)
        self.assertIn("NAME AxleTractionBudget", self.cmake)
        self.assertIn("latebrake_axle_traction_budget_tests PRIVATE /W4 /WX", self.cmake)
        self.assertIn("100000", self.test)
        self.assertIn("front-only", self.test)
        self.assertIn("rear-only", self.test)


if __name__ == "__main__":
    unittest.main()
