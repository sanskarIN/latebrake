"""Source bindings only, not licensed Unreal Engine compilation or QA."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class ArcadeEngineHeatBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.cpp = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy = read("Source/Latebrake/Public/Vehicles/ArcadeEngineHeat.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.docs = read("docs/ARCADE_ENGINE_HEAT_QA.md")

    def test_shared_vehicle_lifecycle(self):
        self.assertIn('#include "Vehicles/ArcadeEngineHeat.h"', self.h)
        self.assertIn("latebrake::ArcadeEngineHeat EngineHeat;", self.h)
        self.assertIn("GetEngineHeat() const", self.h)
        self.assertIn("EngineHeat.Reset();", self.h)
        self.assertEqual(self.cpp.count("EngineHeat.Suspend();"), 3)

    def test_all_motor_boosts_share_the_same_power_limit(self):
        self.assertIn("EngineHeat.Observe({", self.cpp)
        self.assertIn("bEffectiveBraking ? 0.0", self.cpp)
        self.assertIn("NitroDrive.Active, true", self.cpp)
        self.assertIn("EngineThermal.MotorMultiplier", self.cpp)
        self.assertEqual(
            self.cpp.count("ImpactMotorScale * EngineMotorScale * GearPowerFactor"),
            3)
        self.assertLess(self.cpp.index("EngineHeat.Observe({"),
                        self.cpp.index("Chassis->AddForce(Forward * DriveForce"))

    def test_hud_is_only_advisory_and_compact_safe(self):
        section = self.hud.split("// Sustained propulsion heat", 1)[1].split(
            "// Actual high-speed hard chassis collisions", 1)[0]
        self.assertIn("!bCompact", section)
        self.assertIn("VerticalPlan.MaxTelemetryLines >= 28", section)
        self.assertIn("Car->GetEngineHeat().HasValidSample()", section)
        self.assertIn("ENGINE HEAT", section)
        for forbidden in ("AwardCredits(", "PassCheckpoint(",
                          "SaveRecords(", "SetPhysicsLinearVelocity("):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("ArcadeEngineHeatTests.cpp", self.cmake)
        self.assertIn("NAME ArcadeEngineHeat", self.cmake)
        self.assertIn("Unreal Engine 5.8", self.docs)
        self.assertIn("source-only", self.docs.lower())


if __name__ == "__main__":
    unittest.main()
