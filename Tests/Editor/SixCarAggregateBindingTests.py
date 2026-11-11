"""Static source contracts for the six-car aggregate integration branch.

No real UE 5.8 Editor/UHT/Chaos execution or packaged gameplay is performed.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class SixCarAggregateBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.start = read("Source/Latebrake/Public/AI/LiveSixCarStartingGrid.h")
        cls.forecast = read("Source/Latebrake/Public/AI/RivalFleetCollisionForecast.h")
        cls.supervisor = read("Source/Latebrake/Public/AI/RivalFleetRiskSupervisor.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.nav = read("mkdocs.yml")

    def test_actual_pre_green_road_and_post_green_rival_bindings(self):
        for value in ("SouthStraightCollisionActor.IsValid()",
                      "Hit.GetActor() == SouthStraight",
                      "LineTraceSingleByChannel(",
                      "latebrake::EvaluateLiveSixCarStartingGrid("):
            self.assertIn(value, self.mode)
        for value in ("Mode->TryCollectPhysicalTraffic(",
                      "latebrake::ApplyRivalFleetTraffic(TrafficCommand, FleetSnapshot)",
                      "FleetRiskSupervisor.Apply(",
                      "RiskSupervision.Command",
                      "Mode->IsExperimentalSixCarGridEnabled()"):
            self.assertIn(value, self.rival)

    def test_the_two_phases_keep_independent_authority(self):
        gate = self.mode.split(
            "bool ALatebrakeRaceGameMode::IsExperimentalFleetReadyForStart(", 1
        )[1].split("bool ALatebrakeRaceGameMode::", 1)[0]
        self.assertIn("latebrake::EvaluateLiveSixCarStartingGrid(", gate)
        self.assertNotIn("FleetRiskSupervisor.Apply(", gate)
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        self.assertIn("Mode->IsRaceStartGreen()", tick)
        self.assertLess(tick.index("latebrake::ApplyRivalFleetTraffic("),
                        tick.index("FleetRiskSupervisor.Apply("))
        self.assertLess(tick.index("FleetRiskSupervisor.Apply("),
                        tick.index("FollowingBrakeHold.Apply("))
        self.assertLess(tick.index("FollowingBrakeHold.Apply("),
                        tick.index("SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)"))

    def test_invalid_physics_and_identity_block_motor(self):
        for value in ("LiveGridMinimumGroundProbes",
                      "EvaluateChassisGroundContact(",
                      "LiveGridFailure::InsufficientGroundSupport"):
            self.assertIn(value, self.start)
        for value in ("ValidateSixCarFleet(", "FleetForecastHorizonSeconds",
                      "sample.BrakeObservationQualified",
                      "conservativeDeceleration"):
            self.assertIn(value, self.forecast)
        for value in ("observed != Roster", "RemainingHold",
                      "planned.Throttle = std::min(planned.Throttle, ThrottleLimit)"):
            self.assertIn(value, self.supervisor)

    def test_complete_source_test_target_and_manual_qa(self):
        for value in ("LiveSixCarStartingGridTests.cpp",
                      "RivalFleetRiskSupervisorTests.cpp",
                      "SixCarAggregateSafetyTests.cpp",
                      "add_test(NAME SixCarAggregateSafety",
                      "/W4 /WX", "-Wall -Wextra -Wpedantic -Werror"):
            self.assertIn(value, self.cmake)
        for value in ("LIVE_SIX_CAR_GRID_QA.md",
                      "FLEET_PREDICTIVE_SAFETY_QA.md",
                      "SIX_CAR_AGGREGATE_QA.md"):
            self.assertIn(value, self.nav)
        for path in (".github/workflows/core-tests.yml",
                     ".github/workflows/docs-pages.yml"):
            workflow = read(path)
            self.assertIn("feature/six-car-live-grid-launch-alignment-20261110", workflow)
            self.assertIn("contents: read", workflow)


if __name__ == "__main__":
    unittest.main()
