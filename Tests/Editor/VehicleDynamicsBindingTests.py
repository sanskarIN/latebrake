"""Check actual Unreal callsites of the portable mass/angular-velocity policy.

This is a source contract, not UE 5.8/UHT or live Chaos compilation.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2] / "Source/Latebrake"
PAWN = ROOT / "Private/Vehicles/LatebrakeCarPawn.cpp"
RIVAL = ROOT / "Private/AI/LatebrakeRivalCar.cpp"
MODE = ROOT / "Private/Game/LatebrakeRaceGameMode.cpp"
GATE = ROOT / "Private/Racing/LatebrakeCheckpoint.cpp"
POLICY_H = ROOT / "Public/Vehicles/VehicleDynamicsReadiness.h"
POLICY_CPP = ROOT / "Private/Vehicles/VehicleDynamicsReadiness.cpp"


def section(text: str, opening: str, closing: str) -> str:
    if text.count(opening) != 1:
        raise AssertionError("missing or ambiguous source method " + opening)
    return text.split(opening, 1)[1].split(closing, 1)[0]


class VehicleDynamicsBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.rival = RIVAL.read_text(encoding="utf-8")
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.gate = GATE.read_text(encoding="utf-8")
        cls.header = POLICY_H.read_text(encoding="utf-8")
        cls.policy = POLICY_CPP.read_text(encoding="utf-8")

    def test_production_policy_is_real_compiled_cpp(self):
        self.assertIn("VehicleDynamicsSample", self.header)
        self.assertIn("AreVehicleDynamicsReady", self.header)
        self.assertIn("EvaluateVehicleDynamics(", self.policy)
        self.assertIn("std::isfinite(sample.MassKilograms)", self.policy)
        self.assertIn("sample.MassKilograms <= 0.0", self.policy)
        for axis in ("X", "Y", "Z"):
            self.assertIn(f"std::isfinite(sample.AngularVelocity{axis}Degrees)",
                          self.policy)

    def test_authoritative_car_physics_readiness_gates_angular_and_mass(self):
        ready = section(
            self.pawn,
            "bool ALatebrakeCarPawn::IsRacePhysicsReady() const",
            "float ALatebrakeCarPawn::GetSpeedKmH() const",
        )
        self.assertIn("GetPhysicsAngularVelocityInDegrees()", ready)
        self.assertIn("GetMass()", ready)
        self.assertIn("latebrake::IsRaceVehiclePhysicsReady({", ready)
        self.assertIn("&& latebrake::AreVehicleDynamicsReady({", ready)
        self.assertIn("AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z",
                      ready)

    def test_driver_neutralizes_before_chassis_motor_and_torque(self):
        tick = section(
            self.pawn, "void ALatebrakeCarPawn::Tick(float DeltaSeconds)",
            "bool ALatebrakeCarPawn::IsRacePhysicsReady() const",
        )
        guard = "if (!IsRacePhysicsReady())"
        self.assertIn(guard, tick)
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", tick)
        self.assertLess(tick.index(guard), tick.index("SmoothedThrottle ="))
        self.assertLess(tick.index(guard), tick.index("Chassis->AddForce("))
        self.assertLess(tick.index(guard), tick.index("Chassis->AddTorqueInRadians("))
        neutral = section(
            self.pawn, "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()",
            "void ALatebrakeCarPawn::ThrottleInput(",
        )
        self.assertIn("SetPhysicsLinearVelocity(FVector::ZeroVector)", neutral)
        self.assertIn("SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector)",
                      neutral)

    def test_rival_checks_its_own_physics_before_route_advance(self):
        tick = section(self.rival,
                       "void ALatebrakeRivalCar::Tick(float DeltaSeconds)",
                       "\n} // nonexistent after method")
        self.assertIn("!IsRacePhysicsReady()", tick)
        self.assertLess(tick.index("!IsRacePhysicsReady()"),
                        tick.index("RivalGateTrace.Sample("))
        self.assertLess(tick.index("RivalGateTrace.Sample("),
                        tick.index("Route.ObserveVerifiedGate("))

    def test_clock_and_checkpoints_reuse_shared_car_readiness(self):
        self.assertIn("Car->IsRacePhysicsReady()", self.mode)
        self.assertIn("Car->IsRacePhysicsReady()", self.gate)
        gate_body = section(
            self.gate, "void ALatebrakeCheckpoint::Tick(float DeltaSeconds)",
            "\n} // nonexistent after method",
        )
        self.assertLess(gate_body.index("Car->IsRacePhysicsReady()"),
                        gate_body.index("MotionTrace.SampleDetailed("))

    def test_hud_speed_rejects_nonfinite_chaos_reading(self):
        speed = section(self.pawn,
                        "float ALatebrakeCarPawn::GetSpeedKmH() const",
                        "void ALatebrakeCarPawn::RecoverTo(")
        self.assertIn("IsValid(Chassis)", speed)
        self.assertIn("FMath::IsFinite(V.X)", speed)
        self.assertIn("FMath::IsFinite(V.Y)", speed)
        self.assertIn("FMath::IsFinite(SpeedKmH)", speed)
        self.assertIn("return 0.f;", speed)

    def test_regression_removing_chassis_dynamics_call_is_observable(self):
        broken = self.pawn.replace(
            "&& latebrake::AreVehicleDynamicsReady({",
            "&& true /* detached dynamics policy */ && ( {",
        )
        self.assertNotIn("&& latebrake::AreVehicleDynamicsReady({", broken)


if __name__ == "__main__":
    unittest.main()
