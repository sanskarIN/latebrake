"""Portable source-to-Unreal evidence for the six-car risk supervisor.

Contract tests verify ordering, source provenance and safety isolation, not
actual Unreal Engine 5.8/UHT compilation or Chaos collision physics.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def text(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RivalFleetRiskSupervisorBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rival_h = text("Source/Latebrake/Public/AI/LatebrakeRivalCar.h")
        cls.rival = text("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.forecast = text("Source/Latebrake/Public/AI/RivalFleetCollisionForecast.h")
        cls.supervisor = text("Source/Latebrake/Public/AI/RivalFleetRiskSupervisor.h")
        cls.tests = text("Tests/Standalone/RivalFleetRiskSupervisorTests.cpp")
        cls.cmake = text("Tests/Standalone/CMakeLists.txt")
        cls.docs = text("docs/FLEET_PREDICTIVE_SAFETY_QA.md")
        cls.nav = text("mkdocs.yml")

    def test_isolates_normal_one_rival_sessions(self):
        for marker in (
            "RivalFleetRiskSupervisor FleetRiskSupervisor",
            "void ResetPilot()",
        ):
            self.assertIn(marker, self.rival_h)
        self.assertIn("const bool bUseFleetTraffic =", self.rival)
        self.assertIn("Mode->IsExperimentalSixCarGridEnabled()", self.rival)
        self.assertIn("!bUseFleetTraffic && FleetRiskSupervisor.HasPhysicalRoster()", self.rival)
        self.assertIn("FleetRiskSupervisor.Reset()", self.rival)

    def test_exact_live_sample_applied_after_existing_traffic(self):
        for marker in (
            "Mode->TryCollectPhysicalTraffic(",
            "const latebrake::RivalFleetTrafficSample FleetSnapshot{",
            "bPredictiveBrakeQualified",
            "BrakeCapabilityScale",
            "GetBrakeHeat().ServiceBrakeMultiplier()",
            "latebrake::ApplyRivalFleetTraffic(TrafficCommand, FleetSnapshot)",
            "const latebrake::FleetRiskSupervision RiskSupervision =",
            "FleetRiskSupervisor.Apply(",
            "RiskSupervision.Command, static_cast<double>(DeltaSeconds)",
            "bTrafficQualified && RiskSupervision.Qualified",
            "latebrake::ResolveRivalPassTraffic(",
            "latebrake::ModerateRivalYawRate(",
            "SetAutonomousControls(",
        ):
            self.assertIn(marker, self.rival)
        self.assertLess(
            self.rival.index("latebrake::ApplyRivalFleetTraffic(TrafficCommand"),
            self.rival.index("FleetRiskSupervisor.Apply("))
        self.assertLess(
            self.rival.index("FleetRiskSupervisor.Apply("),
            self.rival.index("FollowingBrakeHold.Apply("))
        self.assertLess(
            self.rival.index("FollowingBrakeHold.Apply("),
            self.rival.index("latebrake::ResolveRivalPassTraffic("))

    def test_pure_policy_cannot_touch_player_or_gameplay_authority(self):
        for source in (self.forecast, self.supervisor):
            for forbidden in (
                "UCLASS(", "UPROPERTY(", "GetWorld(", "SpawnActor(",
                "SetActorLocation(", "AddForce(", "AddTorque(",
                "SaveRecords(", "Race.PassCheckpoint(",
                "GrantCredits(", "AddTimePenalty(",
            ):
                self.assertNotIn(forbidden, source)
        for marker in (
            "sample.BlockerCount != MaxRecoveryObstacles",
            "FleetBodyInsideAuthoredAsphalt(",
            "StartGridFootprintTouchesBarrier(pose)",
            "CircuitCollisionBlockCount",
            "CollisionBlockStyle::Asphalt",
            "StartGridProjectionRadius(body, 1.0, 0.0)",
            "std::all_of(roster.begin(), roster.end()",
            "ForecastFleetContact(",
            "FleetForecastHorizonSeconds",
            "FleetForecastDecelerationCmS2",
            "StartGridProjectionRadius(",
            "FleetRiskKind::SweptContact",
            "FleetRiskKind::StoppingDistance",
            "sample.BrakeObservationQualified",
            "sample.ConservativeBrakeScale",
            "conservativeDeceleration",
            "ceiling < result.ThrottleCeiling",
        ):
            self.assertIn(marker, self.forecast)
        for marker in (
            "Roster.fill(0)",
            "observed != Roster",
            "FleetRiskRosterChangeHoldSeconds",
            "FleetRiskEmergencyHoldSeconds",
            "FleetRiskThrottleRecoveryPerSecond",
            "planned.Throttle = std::min(planned.Throttle, ThrottleLimit)",
        ):
            self.assertIn(marker, self.supervisor)

    def test_real_cross_platform_target_and_qa_are_linked(self):
        self.assertIn("RivalFleetRiskSupervisorTests.cpp", self.cmake)
        self.assertIn("NAME RivalFleetRiskSupervisor", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)
        self.assertIn("i < 30000", self.tests)
        self.assertIn("TestObservedBrakeCapacity()", self.tests)
        self.assertIn("TestStrongestFollowingLimitWinsAnyRosterOrder()",
                      self.tests)
        self.assertIn("off-road padded body denies motor", self.tests)
        self.assertIn("barrier-overlapping body is not qualified paved traffic",
                      self.tests)
        self.assertIn("for (int fps : {30, 60, 120})", self.tests)
        self.assertIn("FLEET_PREDICTIVE_SAFETY_QA.md", self.nav)
        self.assertIn("UE5.8", self.docs)
        self.assertIn("SOURCE-ONLY", self.docs)


if __name__ == "__main__":
    unittest.main()
