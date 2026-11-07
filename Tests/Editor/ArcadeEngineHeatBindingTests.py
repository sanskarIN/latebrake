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
        self.assertIn("bEffectiveBraking || OpposingBraking.OpposingThrottle ||\n"
                      "            ReverseGovernor.ReverseMotorBlocked ||\n"
                      "            !AxleBudget.RearDriveReady", self.cpp)
        tick = self.cpp.split("void ALatebrakeCarPawn::Tick(", 1)[1]
        self.assertLess(tick.index("CalculateOpposingThrottleBraking({"),
                        tick.index("EngineHeat.Observe({"))
        self.assertLess(tick.index("EngineHeat.Observe({"),
                        tick.index("if (AxleBudget.RearDriveReady &&\n"
                                   "        !bEffectiveBraking &&\n"
                                   "        !OpposingBraking.OpposingThrottle &&\n"
                                   "        !ReverseGovernor.ReverseMotorBlocked)"))
        self.assertIn("RearDriveTractionScale", self.cpp)
        self.assertIn("ContactTractionScale", self.cpp)
        self.assertIn("NitroDrive.Active, true", self.cpp)
        self.assertIn("EngineHeat.SettleNitroDelivery(bNitroBonusDispatched);",
                      self.cpp)
        self.assertIn("EngineHeat.SettleNitroDelivery(false);",
                      self.cpp)
        self.assertIn("bool SettleNitroDelivery(bool delivered) noexcept",
                      self.policy)
        self.assertIn("PendingNitroExtra = nitroExtra;", self.policy)
        self.assertIn("Heat = baseHeat;", self.policy)
        self.assertIn("if (delivered)", self.policy)
        self.assertIn("Heat = std::clamp(Heat + PendingNitroExtra", self.policy)
        self.assertNotIn("PendingHeatWithoutNitro", self.policy)
        self.assertNotIn("Heat = std::clamp(baseHeat + nitroExtra", self.policy)
        self.assertIn("400 denied nitro samples never enter heat before settlement",
                      read("Tests/Standalone/ArcadeEngineHeatTests.cpp"))
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
