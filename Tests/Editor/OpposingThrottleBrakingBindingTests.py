"""Actual pawn source contracts; not an Unreal compile or driving test."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class OpposingThrottleBrakingBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(
            encoding="utf-8")
        cls.tick = cls.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]

    def test_final_throttle_and_same_frame_real_load_precede_brake_and_motor(self):
        stages = (
            "if (!IsRacePhysicsReady())",
            "ApplySprungForces(SpringResponse, WheelWorldAnchors);",
            "GripReattachment.Advance(",
            "if (!FMath::IsFinite(Speed)",
            "const latebrake::BrakeHeatOutput Thermal =",
            "const float EffectiveThrottle =",
            "latebrake::CalculateGripLimitedBraking({",
            "latebrake::CalculateOpposingThrottleBraking({",
            "if (!OpposingBraking.Valid)",
            "latebrake::CalculateLateralGripForce({",
            "if (!LateralForce.Valid)",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
        )
        self.assertEqual([self.tick.index(s) for s in stages],
                         sorted(self.tick.index(s) for s in stages))
        sample = self.tick.split("CalculateOpposingThrottleBraking({", 1)[1].split("});", 1)[0]
        for term in (
            "static_cast<double>(DeltaSeconds)",
            "static_cast<double>(LongitudinalSpeed)",
            "static_cast<double>(Speed)", "static_cast<double>(Mass)",
            "static_cast<double>(EffectiveThrottle)",
            "Surface.LateralGripMultiplier * TireCondition.GripMultiplier()",
            "static_cast<double>(GarageGrip)",
            "CollisionDamage.LateralGripMultiplier()",
            "AxleBudget.RearDriveReady ? AxleBudget.RearDriveTractionScale : 0.0",
            "GroundContact.ContactTractionScale, bEffectiveBraking, bArcade",
        ):
            self.assertIn(term, sample)
        self.assertEqual(self.tick.count("CalculateOpposingThrottleBraking({"), 1)
        self.assertIn("if (!OpposingBraking.Valid)\n    {\n"
                      "        RejectOptionalDriveAfterSpring(SpringResponse);\n"
                      "        return;\n    }", self.tick)

    def test_braking_cannot_stack_with_motor_or_bypass_lateral_grip(self):
        self.assertIn("if (AxleBudget.RearDriveReady &&\n"
                      "        !bEffectiveBraking &&\n"
                      "        !OpposingBraking.OpposingThrottle &&\n"
                      "        !ReverseGovernor.ReverseMotorBlocked)", self.tick)
        self.assertIn("ContactBraking.WholeBodyBudgetDeceleration +\n"
                      "                OpposingBraking.WholeBodyBudgetDeceleration +\n"
                      "                ReverseGovernor.WholeBodyBudgetDeceleration,", self.tick)
        brake = self.tick.split(
            "else if (AxleBudget.RearDriveReady && !bEffectiveBraking)", 1
        )[1].split("// Real planar resistance", 1)[0]
        self.assertIn("Chassis->AddForce(Forward *\n"
                      "            static_cast<float>(OpposingBraking.ForwardForce));", brake)
        for forbidden in ("RearDriveTractionScale", "ContactTractionScale",
                          "GearPowerFactor", "SetPhysics", "AddTorque"):
            self.assertNotIn(forbidden, brake)
        self.assertNotIn("Chassis->AddForce(-Forward * LongitudinalSpeed * Mass *", self.tick)
        self.assertNotIn("FrameStableDampingRate(3.0, DeltaSeconds)", self.tick)
        self.assertIn("static_cast<float>(LateralForce.RightForce) *\n"
                      "        ContactTractionScale);", self.tick)

    def test_portable_composition_is_registered_with_strict_build(self):
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("OpposingThrottleBrakingTests.cpp", cmake)
        self.assertIn("NAME OpposingThrottleBraking", cmake)
        self.assertIn("latebrake_opposing_throttle_braking_tests PRIVATE /W4 /WX", cmake)
        policy = (ROOT / "Source/Latebrake/Public/Vehicles/OpposingThrottleBraking.h").read_text(
            encoding="utf-8")
        for forbidden in ("GetWorld(", "AddForce(", "SetPhysicsLinearVelocity(",
                          "SaveGame(", "PassCheckpoint(", "AwardCredits("):
            self.assertNotIn(forbidden, policy)


if __name__ == "__main__":
    unittest.main()
