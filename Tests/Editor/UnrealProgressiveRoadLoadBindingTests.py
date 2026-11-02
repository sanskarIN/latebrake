"""Source binding of progressive road load into the REAL Unreal chassis.

This validates engine-facing C++ call sites and physical safety boundaries,
not actual Chaos/UHT compilation, tire physics, gameplay feel or Win64 QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CAR = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Vehicles/ProgressiveRoadLoad.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
NAV = ROOT / "mkdocs.yml"


class UnrealProgressiveRoadLoadBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.car = CAR.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_shared_real_chassis_gets_low_damping_and_aero_load(self):
        self.assertIn('#include "Vehicles/ProgressiveRoadLoad.h"', self.car)
        self.assertIn(
            "static_cast<float>(latebrake::RoadLoadChassisLinearDamping)",
            self.car
        )
        self.assertNotIn("Chassis->SetLinearDamping(0.5f);", self.car)
        tick = self.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        for token in (
            "latebrake::CalculateProgressiveRoadLoad({",
            "static_cast<double>(Velocity.X)",
            "static_cast<double>(Velocity.Y)",
            "static_cast<double>(Mass)",
            "bArcade, Surface.OffPavement",
            "Chassis->AddForce(FVector(",
            "static_cast<float>(RoadLoad.ForceX)",
            "static_cast<float>(RoadLoad.ForceY), 0.f",
        ):
            self.assertIn(token, tick)

    def test_unqualified_frames_cannot_apply_roadload(self):
        tick = self.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        self.assertLess(
            tick.index("CanApplyPlayerDrive({"),
            tick.index("CalculateProgressiveRoadLoad({")
        )
        self.assertLess(
            tick.index("if (!Surface.Valid)"),
            tick.index("CalculateProgressiveRoadLoad({")
        )
        self.assertLess(
            tick.index("CalculateProgressiveRoadLoad({"),
            tick.index("if (!RoadLoad.Valid)")
        )
        self.assertLess(
            tick.index("if (!RoadLoad.Valid)"),
            tick.index("latebrake::AllocateArcadeBoost(")
        )
        self.assertLess(
            tick.index("if (!RoadLoad.Valid)"),
            tick.index("Chassis->AddForce(FVector(")
        )
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", tick)
        self.assertIn("RoadLoadMaxFrameSeconds = 0.10;", self.policy)
        self.assertIn("RoadLoadOffPavementResistance = 1.45;", self.policy)

    def test_load_never_overwrites_vehicle_velocity_or_rewards(self):
        # Only the dedicated road-load application is inspected, not the
        # pre-existing hard speed caps/recovery logic elsewhere in Tick.
        application = self.car.split(
            "// Real planar resistance: low-speed coasting", 1
        )[1].split(
            "// Anti-slide assistance is stronger in Arcade", 1
        )[0]
        self.assertIn("Chassis->AddForce(", application)
        for forbidden in (
            "SetPhysicsLinearVelocity(", "AddTorqueInRadians(",
            "SetSimulatePhysics(", "Race.PassCheckpoint(",
            "SaveRecords(", "EarnCredits(",
        ):
            self.assertNotIn(forbidden, application)
            self.assertNotIn(forbidden, self.policy)

    def test_strict_build_target_and_documentation_exist(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        self.assertIn("ProgressiveRoadLoadTests.cpp", cmake)
        self.assertIn("NAME ProgressiveRoadLoad", cmake)
        self.assertIn("PROGRESSIVE_ROAD_LOAD_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
