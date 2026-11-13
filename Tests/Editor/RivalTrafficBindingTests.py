"""Source-only contracts for actual Unreal rival-to-player traffic checks.

C++20 numeric tests verify the algorithm. These tests do not simulate
Chaos, qualify collision avoidance or prove an installed racing game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class RivalTrafficBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def load(path):
            return (ROOT / path).read_text(encoding="utf-8")
        cls.source = load("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.policy = load("Source/Latebrake/Public/AI/RivalTrafficAwareness.h")
        cls.cmake = load("Tests/Standalone/CMakeLists.txt")

    def test_observes_actual_player_not_synthetic_driver(self):
        tick = self.source.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        for token in (
            "Mode->HasLocalPlayerDriver()",
            "PlanRivalTurn(", "AnticipateRivalCorner(",
            "UGameplayStatics::GetPlayerPawn(this, 0)",
            "IsValid(Player) && Player->IsRacePhysicsReady()",
            "Player->GetActorLocation()",
            "Player->GetVelocity()",
            "YieldRivalToPlayer(SafePassCommand, {",
            "Position.X, Position.Y, Heading, FlatSpeed",
            "PlayerVelocity.X, PlayerVelocity.Y, bPlayerReady",
            "TrafficCommand.Brake",
            "BrakeForecast.SafeDecelerationCmPerSecondSquared",
            "Super::Tick(DeltaSeconds);",
        ):
            self.assertIn(token, tick)
        self.assertLess(tick.index("PlanRivalTurn("), tick.index("YieldRivalToPlayer("))
        self.assertLess(tick.index("YieldRivalToPlayer("),
                        tick.index("FollowingBrakeHold.Apply("))
        self.assertLess(tick.index("FollowingBrakeHold.Apply("),
                        tick.index("SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)"))

    def test_braking_authority_is_shared_with_real_corner_forecast(self):
        forecast = self.source.index(
            "const latebrake::RivalCornerBrakeResult BrakeForecast =")
        passed = self.source.index(
            "BrakeForecast.SafeDecelerationCmPerSecondSquared")
        traffic = self.source.index(
            "latebrake::YieldRivalToPlayer(SafePassCommand, {")
        self.assertLess(forecast, traffic)
        self.assertLess(traffic, passed)
        self.assertIn("!std::isfinite(sample.QualifiedBrakeDecelerationCmPerSecondSquared)", self.policy)
        self.assertIn("sample.QualifiedBrakeDecelerationCmPerSecondSquared < 1.0", self.policy)
        self.assertIn("closing * closing /", self.policy)
        self.assertIn("const bool occupiedFootprint =", self.policy)
        self.assertIn("const bool insufficientBrakingRunway =", self.policy)

    def test_measured_sideways_merge_anticipation_is_bounded(self):
        # Pure C++20 policy only: this does not emulate real Chaos traffic.
        # Sideways motion must be read from both existing WORLD velocity
        # vectors and projected at the physical rear-contact time, never
        # from a guessed steering axis, future waypoint, or player input.
        for marker in (
            "const double signedLateralDistance =",
            "const double relativeLateralSpeed =",
            "sample.PlayerWorldVelocityY - sample.RivalWorldVelocityY",
            "sample.PlayerWorldVelocityX - sample.RivalWorldVelocityX",
            "LaneEntryHorizonSeconds = 1.25",
            "MinimumLaneEntryLookaheadSeconds = 0.10",
            "const double timeToForwardContact =",
            "predictedLateral = signedLateralDistance +",
            "predictedLaneEntry = std::isfinite(predictedLateral)",
            "const bool laneConflict =",
            "!laneConflict ||",
        ):
            self.assertIn(marker, self.policy)
        self.assertLess(self.policy.index("const bool laneConflict ="),
                        self.policy.index("if (forwardDistance <= 0.0 ||"))
        self.assertIn("12000", (ROOT / "Tests/Standalone/"
                                 "RivalTrafficAwarenessTests.cpp").read_text(
                                     encoding="utf-8"))

    def test_never_mutates_race_or_player_control(self):
        for forbidden in (
            "AddForce", "AddTorque", "RaceProgress", "PassCheckpoint",
            "AddTimePenalty", "EarnCredits", "Garage", "SaveGame",
            "SetPhysicsLinearVelocity", "AActor", "UCLASS(",
        ):
            self.assertNotIn(forbidden, self.policy)
        for required in (
            "command.Arrived || command.Brake",
            "command.Steering",
            "command.Throttle = 0.0",
            "command.Brake = true",
            "std::min(command.Throttle, permittedThrottle)",
        ):
            self.assertIn(required, self.policy)

    def test_proximity_and_relative_speed_are_physical(self):
        for marker in (
            "RivalHeadingRadians", "std::cos(", "std::sin(",
            "PlayerWorldVelocityX", "PlayerWorldVelocityY",
            "RivalPlanarSpeedCmPerSecond",
            "forwardDistance", "lateralDistance", "closingSpeed",
            "LaneCorridorCm = 225.0", "MaxAwarenessDistanceCm = 1450.0",
            "EmergencyDistanceCm = 600.0", "closingSpeed >= 230.0",
            "MaximumPredictiveAwarenessCm = 6500.0",
            "ReactionReserveSeconds = 0.50",
            "insufficientBrakingRunway",
            "sample.QualifiedBrakeDecelerationCmPerSecondSquared",
            "!sample.PlayerReady",
        ):
            self.assertIn(marker, self.policy)
        self.assertIn("RivalTrafficAwarenessTests.cpp", self.cmake)
        self.assertIn("NAME RivalTrafficAwareness", self.cmake)


if __name__ == "__main__":
    unittest.main()
