"""Real Unreal pawn force wiring, inspected as source rather than executed."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class TractionLimitedPropulsionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(
            encoding="utf-8")
        cls.tick = cls.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]

    def test_real_lateral_and_allocated_motor_force_precede_final_cap(self):
        order = (
            "ApplySprungForces(SpringResponse, WheelWorldAnchors);",
            "GripReattachment.Advance(",
            "if (!LateralForce.Valid)",
            "AllocateArcadeBoost(",
            "const float AxleGearPowerFactor =",
            "if (!SpeedGovernor.Valid)",
            "const double AllocatedMotorForce =",
            "latebrake::CalculateTractionLimitedPropulsion({",
            "if (!MotorTraction.Valid)",
            "const float GearPowerFactor = AxleGearPowerFactor *",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
        )
        self.assertEqual([self.tick.index(x) for x in order],
                         sorted(self.tick.index(x) for x in order))
        self.assertEqual(self.tick.count("CalculateTractionLimitedPropulsion({"), 1)
        self.assertIn("if (!MotorTraction.Valid)\n    {\n"
                      "        RejectOptionalDriveAfterSpring(SpringResponse);\n"
                      "        return;\n    }", self.tick)
        allocated = self.tick.split("const double AllocatedMotorForce =", 1)[1].split(
            "const latebrake::TractionLimitedPropulsionOutput", 1)[0]
        for token in (
            "static_cast<double>(DriveForce)", "static_cast<double>(EffectiveThrottle)",
            "Surface.MotorMultiplier", "static_cast<double>(ImpactMotorScale)",
            "static_cast<double>(EngineMotorScale)",
            "static_cast<double>(AxleGearPowerFactor)",
            "(1.0 + CombinedBoost.NitroExtra + DraftExtraForce)",
        ):
            self.assertIn(token, allocated)
        sample = self.tick.split("CalculateTractionLimitedPropulsion({", 1)[1].split("});", 1)[0]
        for token in (
            "static_cast<double>(DeltaSeconds)", "static_cast<double>(Mass)",
            "AllocatedMotorForce",
            "LateralForce.RightForce * static_cast<double>(ContactTractionScale)",
            "static_cast<double>(GarageGrip) * TireCondition.GripMultiplier()",
            "Surface.LateralGripMultiplier", "CollisionDamage.LateralGripMultiplier()",
            "GroundContact.ContactTractionScale",
            "AxleBudget.RearDriveTractionScale, bArcade",
        ):
            self.assertIn(token, sample)

    def test_all_three_additive_force_paths_share_one_reduction(self):
        tick = self.tick
        self.assertIn("const float GearPowerFactor = AxleGearPowerFactor *\n"
                      "        static_cast<float>(MotorTraction.ForceScale);", tick)
        motor = tick.split("if (AxleBudget.RearDriveReady &&\n"
                           "        !bEffectiveBraking &&\n"
                           "        !OpposingBraking.OpposingThrottle)", 1)[1].split(
            "else if (AxleBudget.RearDriveReady && !bEffectiveBraking)", 1)[0]
        self.assertEqual(motor.count("Chassis->AddForce("), 3)
        self.assertEqual(motor.count("* EngineMotorScale * GearPowerFactor);"), 3)
        self.assertNotIn("AxleGearPowerFactor", motor)
        self.assertIn("if (NitroDrive.Active)", motor)
        self.assertIn("if (DraftExtraForce > 0.0)", motor)
        self.assertIn("if (bEffectiveBraking && GroundContact.HasDriveContact)", tick)
        brake = tick.split("else if (AxleBudget.RearDriveReady && !bEffectiveBraking)", 1)[1].split(
            "// Real planar resistance", 1)[0]
        self.assertIn("OpposingBraking.ForwardForce", brake)
        self.assertNotIn("MotorTraction", brake)
        self.assertNotIn("GearPowerFactor", brake)

    def test_portable_shared_constants_and_strict_ci_are_connected(self):
        policy = (ROOT / "Source/Latebrake/Public/Vehicles/TractionLimitedPropulsion.h").read_text(
            encoding="utf-8")
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
        for term in ('#include "Vehicles/LateralGripBudget.h"',
                     "ArcadeCombinedGripBudget", "SimulationCombinedGripBudget",
                     "nominalBudget * s.ContactTractionScale",
                     "nominalBudget * s.RearTractionScale",
                     "wholeBodyBudget * wholeBodyBudget - usedLateral * usedLateral"):
            self.assertIn(term, policy)
        for forbidden in ("GetWorld(", "AddForce(", "SetPhysicsLinearVelocity(",
                          "AddTorque", "SaveGame(", "PassCheckpoint(", "AwardCredits("):
            self.assertNotIn(forbidden, policy)
        self.assertIn("TractionLimitedPropulsionTests.cpp", cmake)
        self.assertIn("NAME TractionLimitedPropulsion", cmake)
        self.assertIn("latebrake_traction_limited_propulsion_tests PRIVATE /W4 /WX", cmake)


if __name__ == "__main__":
    unittest.main()
