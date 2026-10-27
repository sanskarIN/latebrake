"""Source contracts for prototype procedural wheel rendering.

These tests inspect actual Unreal pawn source; standalone C++20 exercises the
production numerical policy. Neither substitutes a cooked vehicle game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class WheelVisualBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def read(path):
            return (ROOT / path).read_text(encoding="utf-8")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.policy = read("Source/Latebrake/Public/Vehicles/WheelVisualMotion.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_spokes_are_real_noncolliding_components_and_have_proper_wheel_axes(self):
        constructor = self.pawn.split("ALatebrakeCarPawn::ALatebrakeCarPawn()", 1)[1].split(
            "void ALatebrakeCarPawn::BeginPlay()", 1)[0]
        for token in (
            "FrontLeftWheel", "FrontRightWheel", "RearLeftWheel", "RearRightWheel",
            "FrontLeftSpoke", "FrontRightSpoke", "RearLeftSpoke", "RearRightSpoke",
            'TEXT("FrontLeftSpoke")', 'TEXT("RearRightSpoke")',
            "Spokes[Index]->SetupAttachment(Wheels[Index]);",
            "Spokes[Index]->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
            "Spokes[Index]->SetSimulatePhysics(false);",
            "Wheels[Index]->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));",
        ):
            with self.subTest(token=token):
                self.assertIn(token, constructor)

    def test_real_physics_gates_precede_actual_wheel_motion(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplyWheelVisualPose()", 1)[0]
        for token in (
            "Mode->IsRaceStartGreen()",
            "CanApplyPlayerDrive(",
            "if (!IsRacePhysicsReady())",
            "GetPhysicsLinearVelocity()",
            "FMath::IsFinite(Velocity.X)",
            "FMath::IsFinite(Speed)",
            "LimitSteeringAtSpeed(",
            "UpdateWheelVisuals(static_cast<double>(DeltaSeconds)",
            "static_cast<double>(LongitudinalSpeed)",
            "static_cast<double>(AssistedSteering)",
        ):
            self.assertIn(token, tick)
        self.assertLess(tick.index("if (!IsRacePhysicsReady())"),
                        tick.index("UpdateWheelVisuals("))
        self.assertLess(tick.index("GetPhysicsLinearVelocity()"),
                        tick.index("UpdateWheelVisuals("))
        self.assertLess(tick.index("FMath::IsFinite(Speed)"),
                        tick.index("UpdateWheelVisuals("))

    def test_articulation_is_decorative_and_no_chaos_force_paths(self):
        visual = self.pawn.split("void ALatebrakeCarPawn::ApplyWheelVisualPose()", 1)[1].split(
            "bool ALatebrakeCarPawn::IsRacePhysicsReady()", 1)[0]
        for token in (
            "WheelMotion.Current()", "Base * Rolling", "Steering",
            "Index < 2", "SetRelativeRotation(", "WheelMotion.Advance({",
        ):
            self.assertIn(token, visual)
        for invalid in (
            "AddForce", "AddTorque", "SaveGame", "RaceProgress",
            "PassCheckpoint", "AddTimePenalty", "SetPhysicsLinearVelocity",
        ):
            self.assertNotIn(invalid, visual)
            self.assertNotIn(invalid, self.policy)
        self.assertIn("WheelVisualMotionTests.cpp", self.cmake)
        self.assertIn("NAME WheelVisualMotion", self.cmake)

    def test_pause_teleport_and_restart_have_explicit_motion_boundaries(self):
        blocked = self.pawn.split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1
        )[1].split("void ALatebrakeCarPawn::ThrottleInput(", 1)[0]
        recovery = self.pawn.split(
            "bool ALatebrakeCarPawn::TryRecoverTo(", 1
        )[1].split("void ALatebrakeCarPawn::RecoverTo(", 1)[0]
        self.assertIn("WheelMotion.Suspend();", blocked)
        self.assertIn("WheelMotion.Reset();", recovery)
        self.assertLess(recovery.index("WheelMotion.Reset();"),
                        recovery.index("SetActorLocationAndRotation("))
        self.assertIn("WheelMotion.Reset(); ApplyWheelVisualPose();",
                      self.pawn_h)
        self.assertIn("Car->ResetDrivingStats();", self.mode)

    def test_numerical_policy_bounds_real_reverse_and_time(self):
        for token in (
            "ForwardSpeedCmPerSecond", "std::remainder(",
            "MaxDeltaSeconds = 0.25", "MaxSteerDegrees = 30.0",
            "MaxForwardSpeedCmPerSecond = 15000.0",
            "std::isfinite(sample.SteeringInput)",
            "sample.ForwardSpeedCmPerSecond * sample.DeltaSeconds",
            "WheelRadiusCm",
        ):
            self.assertIn(token, self.policy)


if __name__ == "__main__":
    unittest.main()
