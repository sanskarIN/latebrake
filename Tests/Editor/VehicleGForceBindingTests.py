"""Source-only Unreal bindings for advisory Chaos world-velocity G telemetry.

Portable C++20 tests exercise numerical physics policy independently.
These contracts cannot replace UE/UHT compilation, rendering or hardware QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class VehicleGForceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def src(path):
            return (ROOT / path).read_text(encoding="utf-8")
        cls.policy = src("Source/Latebrake/Public/Racing/VehicleGForceMeter.h")
        cls.pawn_h = src("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = src("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.mode = src("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.hud = src("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = src("Tests/Standalone/CMakeLists.txt")

    def test_physics_state_is_valid_before_reading_measured_velocity(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "bool ALatebrakeCarPawn::IsRacePhysicsReady()", 1
        )[0]
        for required in (
            "if (!Mode || !Mode->IsRaceStartGreen()",
            "if (!IsRacePhysicsReady())",
            "GetPhysicsLinearVelocity()",
            "FMath::IsFinite(Velocity.X)",
            "GForceMeter.Observe({",
            "static_cast<double>(Velocity.X)",
            "static_cast<double>(Velocity.Y)",
            "GetActorRotation().Yaw",
            "bEffectiveBraking",
        ):
            self.assertIn(required, tick)
        self.assertLess(tick.index("if (!IsRacePhysicsReady())"),
                        tick.index("GForceMeter.Observe({"))
        self.assertLess(tick.index("GetPhysicsLinearVelocity()"),
                        tick.index("GForceMeter.Observe({"))
        self.assertLess(tick.index("FMath::IsFinite(Velocity.X)"),
                        tick.index("GForceMeter.Observe({"))

    def test_pause_teleport_and_restart_reset_discontinuity(self):
        blocked = self.pawn.split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1
        )[1].split("void ALatebrakeCarPawn::ThrottleInput(", 1)[0]
        teleport = self.pawn.split(
            "bool ALatebrakeCarPawn::TryRecoverTo(", 1
        )[1].split("void ALatebrakeCarPawn::RecoverTo(", 1)[0]
        self.assertIn("GForceMeter.Suspend();", blocked)
        self.assertIn("GForceMeter.Suspend();", teleport)
        self.assertIn("GForceMeter.Reset();", self.pawn_h)
        restart = self.mode.split(
            "void ALatebrakeRaceGameMode::RestartRace()", 1
        )[1].split("void ALatebrakeRaceGameMode::RecoverPlayer(", 1)[0]
        self.assertIn("Car->ResetDrivingStats();", restart)
        self.assertIn("const latebrake::VehicleGForceMeter& GetVehicleGForceMeter()", self.pawn_h)

    def test_results_and_expanded_hud_are_unofficial(self):
        for required in (
            "GetVehicleGForceMeter()",
            "GetPeakBrakingG()",
            "GetPeakCorneringG()",
            "GetLiveLongitudinalG()",
            "GetLiveLateralG()",
            "CHASSIS G-FORCE ESTIMATES",
            "CHASSIS G (EST)",
            "CanAwardFinishedRace(Race)",
        ):
            self.assertIn(required, self.hud)
        self.assertIn("if (!bCompact)", self.hud)
        for forbidden in (
            "AddForce", "SetTime", "AddTimePenalty", "SaveGame",
            "CareerAward", "EarnCredits", "PassCheckpoint", "UCLASS(",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("VehicleGForceMeterTests.cpp", self.cmake)
        self.assertIn("NAME VehicleGForceMeter", self.cmake)

    def test_world_velocity_math_and_fail_closed_derivative(self):
        for required in (
            "WorldVelocityX - PreviousX",
            "WorldVelocityY - PreviousY",
            "std::remainder(sample.HeadingDegrees, 360.0)",
            "std::cos(yaw)",
            "std::sin(yaw)",
            "MaxResultantG * GravityCmPerSecondSquared",
            "sample.BrakeApplied",
            "forwardSpeed >= 280.0",
            "MaxSampleSeconds = 0.25",
            "MinSampleSeconds = 0.001",
            "HavePrevious = false;",
        ):
            self.assertIn(required, self.policy)


if __name__ == "__main__":
    unittest.main()
