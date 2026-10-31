"""Bindings for measured human spin traction in the real player pawn.

Static source tests complement a portable C++ model; they cannot compile
UnrealHeaderTool, measure actual Chaos forces or qualify any game package.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def get(path):
    return (ROOT / path).read_text(encoding="utf-8")


class HumanSpinTractionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = get("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = get("Source/Latebrake/Public/Vehicles/HumanSpinTractionControl.h")
        cls.cmake = get("Tests/Standalone/CMakeLists.txt")
        cls.test = get("Tests/Standalone/HumanSpinTractionControlTests.cpp")
        cls.guide = get("docs/HUMAN_SPIN_TRACTION_QA.md")

    def test_chassis_readiness_before_measured_spin_policy(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        tokens = (
            "!IsRacePhysicsReady()",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity();",
            "if (!FMath::IsFinite(Speed)",
            "SurfaceTraction.Observe({",
            "StabilizeCornerThrottle(",
            "const bool bHumanDriver = IsLocallyControlled()",
            "GetPhysicsAngularVelocityInRadians()",
            "ModerateHumanSpinThrottle(",
            "NitroBoost.Step(",
            "AllocateArcadeBoost(",
            "GearAwareMotorPowerFactor(",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
        )
        indices = [tick.index(token) for token in tokens]
        self.assertEqual(indices, sorted(indices))
        self.assertEqual(tick.count("ModerateHumanSpinThrottle("), 1)
        self.assertIn('#include "Vehicles/HumanSpinTractionControl.h"', self.pawn)
        self.assertIn("CornerThrottle, static_cast<double>(LongitudinalSpeed)", tick)
        self.assertIn("static_cast<double>(AngularVelocity.Z)", tick)
        self.assertIn("bArcade, bHumanDriver, BrakeProfile.HandbrakeActive", tick)

    def test_single_final_motor_command_shared_by_all_boosts(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertEqual(
            tick.count("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *"),
            3,
        )
        self.assertIn("NitroBoost.Step(", tick)
        self.assertIn("SlipstreamBoost.Observe({", tick)
        self.assertIn("CombinedBoost.NitroExtra", tick)
        self.assertIn("DraftExtraForce = CombinedBoost.DraftExtra;", tick)
        self.assertIn("if (!bEffectiveBraking &&", tick)
        # No rival-specific RPM or route mutations; base shared motor path.
        self.assertNotIn("ModerateHumanSpinThrottle(", get(
            "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp"
        ))

    def test_policy_remains_pure_no_rewards_or_physics_mutation(self):
        for marker in (
            "std::isfinite(requestedThrottle)",
            "std::isfinite(signedForwardCmPerSecond)",
            "std::isfinite(lateralCmPerSecond)",
            "std::isfinite(yawRadiansPerSecond)",
            "!locallyControlledHuman", "handbrakeHeld", "command <= 0.0",
            "std::hypot(", "const double reduction = 0.70",
        ):
            self.assertIn(marker, self.header)
        for forbidden in (
            "#include \"CoreMinimal.h\"", "UCLASS(", "AddTorque",
            "AddForce(", "SaveGame", "PassCheckpoint", "SetActorLocation",
            "ResetPilot", "RaceProgress", "NitroBoost.Press", "AwardCredits",
        ):
            self.assertNotIn(forbidden, self.header)

    def test_portable_stress_and_installed_qa_are_separate(self):
        self.assertIn("HumanSpinTractionControlTests.cpp", self.cmake)
        self.assertIn("NAME HumanSpinTractionControl", self.cmake)
        self.assertIn("latebrake_human_spin_traction_tests PRIVATE /W4 /WX",
                      self.cmake)
        self.assertIn("250000", self.test)
        for assertion in ("Simulation Inspired", "handbrake", "reverse",
                          "sideways", "nonfinite", "unrepresentable"):
            self.assertIn(assertion, self.test)
        self.assertIn("Unreal Engine 5.8", self.guide)
        self.assertIn("not a playable", self.guide.lower())


if __name__ == "__main__":
    unittest.main()
