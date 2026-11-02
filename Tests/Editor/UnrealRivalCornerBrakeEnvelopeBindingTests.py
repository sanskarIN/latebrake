"""Real rival Chaos source binding for speed/distance and observed grip forecast.

Not an Unreal 5.8 executable/gameplay/suspension test.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
RIVAL = ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp"
POLICY = ROOT / "Source/Latebrake/Public/AI/RivalCornerBrakeEnvelope.h"
BRAKES = ROOT / "Source/Latebrake/Public/Vehicles/GripLimitedBraking.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
NAV = ROOT / "mkdocs.yml"


class UnrealRivalCornerBrakeEnvelopeBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rival = RIVAL.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.brakes = BRAKES.read_text(encoding="utf-8")

    def test_real_rival_uses_last_qualified_physical_grip_and_brake_heat(self):
        for marker in (
            '#include "AI/RivalCornerBrakeEnvelope.h"',
            "latebrake::AnticipateRivalCorner(",
            "latebrake::ApplyRivalCornerBrakeEnvelope(Command, {",
            "Route.GetNextWaypoint()",
            "{Position.X, Position.Y}, FlatSpeed",
            "GetSurfaceTraction().HasValidSample()",
            "GetTireCondition().HasValidSample()",
            "GetCollisionDamage().HasEligibleFrame()",
            "GetBrakeHeat().HasValidSample()",
            "GetSurfaceTraction().IsOffPavement()",
            "GetTireCondition().GripMultiplier()",
            "GetCollisionDamage().LateralGripMultiplier()",
            "GetBrakeHeat().ServiceBrakeMultiplier()",
            "GetHandlingMode() == ELatebrakeHandlingMode::Arcade",
            "Command = BrakeForecast.Command;",
            "Command = latebrake::ApplyRivalDifficulty(",
            "Command = GripPace.Apply(",
            "latebrake::YieldRivalToPlayer(Command, {",
            "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)",
        ):
            self.assertIn(marker, self.rival)
        start = self.rival.index("const latebrake::CircuitPoint NextTarget =")
        segment = self.rival[start:]
        self.assertLess(segment.index("latebrake::AnticipateRivalCorner("),
                        segment.index("latebrake::ApplyRivalCornerBrakeEnvelope("))
        self.assertLess(segment.index("latebrake::ApplyRivalCornerBrakeEnvelope("),
                        segment.index("Command = latebrake::ApplyRivalDifficulty("))
        self.assertLess(segment.index("Command = BrakeForecast.Command;"),
                        segment.index("Command = GripPace.Apply("))
        self.assertLess(segment.index("Command = GripPace.Apply("),
                        segment.index("latebrake::YieldRivalToPlayer(Command, {"))

    def test_actual_braking_coefficients_shared_not_invented(self):
        self.assertIn('#include "Vehicles/GripLimitedBraking.h"', self.policy)
        self.assertIn("ArcadeServiceBrakeDeceleration", self.brakes)
        self.assertIn("SimulationServiceBrakeDeceleration", self.brakes)
        self.assertIn("ArcadeServiceBrakeDeceleration", self.policy)
        self.assertIn("SimulationServiceBrakeDeceleration", self.policy)
        self.assertIn("RivalBrakeForecastSafetyScale = 0.76", self.policy)
        self.assertIn("RivalEarlyBrakeMaximumApproachCm = 5500.0", self.policy)
        self.assertIn("RivalCornerRunThroughBufferCm = 550.0", self.policy)
        self.assertIn("std::sqrt(", self.policy)
        self.assertIn("sample.BrakeHeatMultiplier", self.policy)
        self.assertIn("crossTrack > RivalEarlyBrakeMaximumCrossTrackCm", self.policy)
        self.assertIn("inboundRemaining < RivalEarlyBrakeMinimumApproachCm", self.policy)
        self.assertIn("!sample.PriorPhysicsQualified", self.policy)

    def test_does_not_steer_teleport_give_awards_or_ignore_rival_route(self):
        for forbidden in (
            "PassCheckpoint(", "AwardCredits(", "SaveGame",
            "SetActorLocation(", "SetPhysicsLinearVelocity(",
            "SetActorRotation(", "AddForce(", "AddTorqueInRadians(",
            "Route.ObserveVerifiedGate(", "TryRecoverTo(",
        ):
            self.assertNotIn(forbidden, self.policy)
        for marker in (
            "Route.ObserveVerifiedGate(", "RivalGateTrace.Sample(",
            "Observation.NeedsRecovery", "Route.ReportRecoveryResult(",
            "GetRace()", "CanDriveRival(",
        ):
            self.assertIn(marker, self.rival)

    def test_registered_for_cross_platform_strict_and_docs(self):
        cmake = CMAKE.read_text(encoding="utf-8")
        self.assertIn("RivalCornerBrakeEnvelopeTests.cpp", cmake)
        self.assertIn("NAME RivalCornerBrakeEnvelope", cmake)
        self.assertIn("/W4 /WX", cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", cmake)
        self.assertIn(
            "RIVAL_BRAKE_ENVELOPE_QA.md", NAV.read_text(encoding="utf-8")
        )


if __name__ == "__main__":
    unittest.main()
