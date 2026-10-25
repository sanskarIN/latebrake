"""Static contracts for the actual Unreal steering integrator.

This verifies checked-in C++ call sites and sequencing only. It does not
substitute for UE 5.8/UHT compilation, Chaos handling tests or a device build.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
RIVAL = ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp"
PHYSICS = ROOT / "Source/Latebrake/Public/Vehicles/RaceVehiclePhysicsReadiness.h"
STEERING = ROOT / "Source/Latebrake/Public/Vehicles/SpeedSensitiveSteering.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


def section(source: str, begin: str, end: str) -> str:
    if source.count(begin) != 1 or end not in source:
        raise AssertionError("Missing unique method boundary: " + begin)
    return source.split(begin, 1)[1].split(end, 1)[0]


class SpeedSensitiveSteeringBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.rival = RIVAL.read_text(encoding="utf-8")
        cls.physics = PHYSICS.read_text(encoding="utf-8")
        cls.steering = STEERING.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_actual_pawn_uses_portable_speed_policy(self):
        self.assertIn('#include "Vehicles/SpeedSensitiveSteering.h"', self.pawn)
        tick = section(
            self.pawn,
            "void ALatebrakeCarPawn::Tick(float DeltaSeconds)",
            "bool ALatebrakeCarPawn::IsRacePhysicsReady() const",
        )
        self.assertIn("latebrake::LimitSteeringAtSpeed(SmoothedSteering, Speed, bArcade)", tick)
        self.assertIn("const float AssistedSteering", tick)
        self.assertIn("AssistedSteering * SteeringAuthority * SteerDirection", tick)
        self.assertNotIn("SmoothedSteering * SteeringAuthority * SteerDirection", tick)
        self.assertLess(tick.index("LimitSteeringAtSpeed("),
                        tick.index("Chassis->AddTorqueInRadians("))
        self.assertLess(tick.index("!IsRacePhysicsReady()"),
                        tick.index("LimitSteeringAtSpeed("))

    def test_nonfinite_derived_speed_neutralized_before_all_motor_forces(self):
        tick = section(
            self.pawn,
            "void ALatebrakeCarPawn::Tick(float DeltaSeconds)",
            "bool ALatebrakeCarPawn::IsRacePhysicsReady() const",
        )
        for expr in ("!FMath::IsFinite(Speed)", "!FMath::IsFinite(LongitudinalSpeed)",
                     "!FMath::IsFinite(LateralSpeed)"):
            with self.subTest(expr=expr):
                self.assertIn(expr, tick)
        self.assertLess(tick.index("!FMath::IsFinite(Speed)"),
                        tick.index("Chassis->AddForce("))
        self.assertLess(tick.index("!FMath::IsFinite(Speed)"),
                        tick.index("Chassis->AddTorqueInRadians("))
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", tick)

    def test_authoritative_chassis_readiness_includes_full_actor_rotation(self):
        ready = section(
            self.pawn,
            "bool ALatebrakeCarPawn::IsRacePhysicsReady() const",
            "float ALatebrakeCarPawn::GetSpeedKmH() const",
        )
        self.assertIn("const FRotator Rotation = GetActorRotation();", ready)
        self.assertIn("Rotation.Pitch, Rotation.Yaw, Rotation.Roll", ready)
        for axis in ("Pitch", "Yaw", "Roll"):
            with self.subTest(axis=axis):
                self.assertIn("std::isfinite(vehicle.Rotation" + axis + "Degrees)",
                              self.physics)

    def test_base_pawn_policy_shared_by_player_and_rival(self):
        # The AI chooses control axes but delegates all physical force and
        # torque to the base pawn. Do not replace this with raw AI torque.
        self.assertIn("SetAutonomousControls(", self.rival)
        self.assertIn("Super::Tick(DeltaSeconds);", self.rival)
        self.assertNotIn("AddTorqueInRadians(", self.rival)
        self.assertNotIn("AddForce(", self.rival)

    def test_portable_contract_compiled_in_strict_CI(self):
        self.assertIn("SpeedSensitiveSteeringTests.cpp", self.cmake)
        self.assertIn("NAME SpeedSensitiveSteering", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)
        self.assertIn("SteeringSpeedFactor(", self.steering)
        self.assertIn("std::isfinite(speedCmPerSecond)", self.steering)

    def test_regressions_become_observable(self):
        detached = self.pawn.replace(
            "latebrake::LimitSteeringAtSpeed(SmoothedSteering, Speed, bArcade)",
            "SmoothedSteering /* unsafe full high-speed torque */",
        )
        self.assertNotIn("latebrake::LimitSteeringAtSpeed(", detached)
        detached_rotation = self.pawn.replace(
            "Rotation.Pitch, Rotation.Yaw, Rotation.Roll",
            "0.0, 0.0, 0.0",
        )
        self.assertNotIn("Rotation.Pitch, Rotation.Yaw, Rotation.Roll",
                         detached_rotation)


if __name__ == "__main__":
    unittest.main()
