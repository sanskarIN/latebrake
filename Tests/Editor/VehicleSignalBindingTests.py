"""Static Unreal integration gates for decorative real-motion rear lamps.

Numeric policy is built in real portable C++20 tests. These bindings
cannot prove Unreal Engine compilation, cook, actual rendered lights,
night racing or installed Android/Win64 device performance.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class VehicleSignalBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def load(path):
            return (ROOT / path).read_text(encoding="utf-8")
        cls.pawn_h = load("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = load("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.policy = load("Source/Latebrake/Public/Vehicles/VehicleSignalLights.h")
        cls.cmake = load("Tests/Standalone/CMakeLists.txt")

    def test_physics_and_focus_gates_precede_lamp_activation(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        for marker in (
            "Mode->IsRaceStartGreen()",
            "CanApplyPlayerDrive(",
            "if (!IsRacePhysicsReady())",
            "GetPhysicsLinearVelocity()",
            "FMath::IsFinite(LongitudinalSpeed)",
            "const bool bEffectiveBraking",
            "ApplySignalLights(true, bEffectiveBraking,",
            "static_cast<double>(LongitudinalSpeed)",
        ):
            self.assertIn(marker, tick)
        self.assertLess(tick.index("if (!IsRacePhysicsReady())"),
                        tick.index("ApplySignalLights(true, bEffectiveBraking"))
        self.assertLess(tick.index("FMath::IsFinite(LongitudinalSpeed)"),
                        tick.index("ApplySignalLights(true, bEffectiveBraking"))

    def test_lamps_are_child_components_and_not_chassis_collision(self):
        constructor = self.pawn.split("ALatebrakeCarPawn::ALatebrakeCarPawn()", 1)[1].split(
            "void ALatebrakeCarPawn::BeginPlay()", 1
        )[0]
        for marker in (
            'TEXT("RearLeftSignal")', 'TEXT("RearRightSignal")',
            'TEXT("ReverseSignal")',
            "Lamps[Index]->SetupAttachment(Chassis);",
            "Lamps[Index]->SetRelativeLocation(",
            "Lamps[Index]->SetAttenuationRadius(170.f);",
            "Lamps[Index]->SetIntensity(0.f);",
            "Lamps[Index]->SetCastShadows(false);",
            "Lamps[Index]->SetCastVolumetricShadow(false);",
            "Lamps[Index]->SetAffectGlobalIllumination(false);",
            "Lamps[Index]->SetAffectReflection(false);",
            "Lamps[Index]->SetLightColor(",
        ):
            self.assertIn(marker, constructor)
        for name in ("RearLeftSignal", "RearRightSignal", "ReverseSignal"):
            self.assertIn("TObjectPtr<UPointLightComponent> " + name, self.pawn_h)

    def test_no_saved_rewards_or_chassis_force_modification(self):
        fn = self.pawn.split("void ALatebrakeCarPawn::ApplySignalLights(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplyWheelVisualPose()", 1)[0]
        for marker in (
            "SelectVehicleSignalLights({", "RearLeftSignal->SetIntensity(",
            "RearRightSignal->SetIntensity(", "ReverseSignal->SetIntensity(",
        ):
            self.assertIn(marker, fn)
        for forbidden in (
            "AddForce", "AddTorque", "SaveGame", "RaceProgress",
            "PassCheckpoint", "SetPhysicsLinearVelocity", "EarnCredits",
            "SetActorLocation", "SetControlRotation",
        ):
            self.assertNotIn(forbidden, fn)
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("VehicleSignalLightsTests.cpp", self.cmake)
        self.assertIn("NAME VehicleSignalLights", self.cmake)

    def test_focus_recovery_and_physics_interruption_clear_lamps(self):
        for heading in (
            "NeutralizeMotionWhenDrivingBlocked()",
            "HandleAppInputInterruption()",
            "UnPossessed()",
            "EndPlay(const EEndPlayReason::Type EndPlayReason)",
            "TryRecoverTo(",
        ):
            body = self.pawn.split("void ALatebrakeCarPawn::" + heading, 1)
            if not body or len(body) == 1:
                body = self.pawn.split("bool ALatebrakeCarPawn::" + heading, 1)
            self.assertEqual(len(body), 2, heading)
            self.assertIn("ApplySignalLights(false, false, 0.0);",
                          body[1].split("\n}", 1)[0])

    def test_reverse_requires_measured_negative_forward_velocity(self):
        for marker in (
            "SignedForwardSpeedCmPerSecond",
            "std::isfinite(",
            "ReverseThresholdCmPerSecond = -130.0",
            "sample.EffectiveBrakeHeld",
            "!sample.RaceDrivingAllowed",
            "sample.SignedForwardSpeedCmPerSecond <=",
        ):
            self.assertIn(marker, self.policy)


if __name__ == "__main__":
    unittest.main()
