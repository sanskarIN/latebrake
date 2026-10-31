"""Source-only wiring and authority contracts for predictive rival following.

These assertions cannot prove licensed UE compile, Chaos braking distances,
frame rates, on-track traffic, installed EXE, or Android AI performance.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class RivalFollowingBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rival = load("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.rival_h = load("Source/Latebrake/Public/AI/LatebrakeRivalCar.h")
        cls.policy = load("Source/Latebrake/Public/AI/RivalTrafficAwareness.h")
        cls.hold = load("Source/Latebrake/Public/AI/RivalFollowingBrakeHold.h")
        cls.cmake = load("Tests/Standalone/CMakeLists.txt")

    def test_real_player_motion_drives_prediction_before_pawn_tick(self):
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        for marker in (
            "GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>()",
            "Mode->HasLocalPlayerDriver()",
            "Mode->IsRaceStartGreen()",
            "Player->IsRacePhysicsReady()",
            "Player->GetActorLocation()",
            "Player->GetVelocity()",
            "Velocity.X, Velocity.Y",
            "PlanRivalTurn(",
            "AnticipateRivalCorner(",
            "YieldRivalToPlayer(Command, {",
            "FollowingBrakeHold.Apply(",
            "bPlayerReady, TrafficCommand.Brake && !Command.Brake",
            "SafeSpinCommand.Throttle",
            "SafeSpinCommand.Steering",
            "SafeSpinCommand.Brake",
            "Super::Tick(DeltaSeconds);",
        ):
            self.assertIn(marker, tick)
        self.assertLess(tick.index("YieldRivalToPlayer("),
                        tick.index("FollowingBrakeHold.Apply("))
        self.assertLess(tick.index("FollowingBrakeHold.Apply("),
                        tick.index("SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)"))

    def test_time_to_contact_requires_same_lane_forward_closing(self):
        for marker in (
            "PlayerWorldVelocityX",
            "PlayerWorldVelocityY",
            "RivalPlanarSpeedCmPerSecond",
            "RivalWorldVelocityX",
            "RivalWorldVelocityY",
            "rivalForwardSpeed",
            "forwardDistance",
            "lateralDistance",
            "closingSpeed",
            "ConservativeContactClearanceCm = 380.0",
            "CriticalTimeToContactSeconds = 0.75",
            "const double timeToContact = availableGap / closingSpeed",
            "closingSpeed >= 500.0",
            "nearEmergency || predictedEmergency",
            "command.Throttle = 0.0",
            "command.Brake = true",
            "command.Steering",
        ):
            self.assertIn(marker, self.policy)
        self.assertLess(self.policy.index("if (forwardDistance <= 0.0"),
                        self.policy.index("timeToContact ="))
        self.assertLess(self.policy.index("lateralDistance >= LaneCorridorCm"),
                        self.policy.index("timeToContact ="))

    def test_race_interruptions_and_recovery_reset_latch(self):
        for method in ("ResetPilot", "ConfigurePilot", "Tick"):
            section = self.rival.split("void ALatebrakeRivalCar::" + method + "(", 1)[1]
            if method == "ResetPilot":
                section = section.split("void ALatebrakeRivalCar::ConfigurePilot", 1)[0]
            elif method == "ConfigurePilot":
                section = section.split("void ALatebrakeRivalCar::Tick", 1)[0]
            self.assertIn("FollowingBrakeHold.Reset();", section)
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        self.assertGreaterEqual(tick.count("FollowingBrakeHold.Reset();"), 3)
        self.assertIn("RivalFollowingBrakeHold FollowingBrakeHold;", self.rival_h)
        self.assertIn('AI/RivalFollowingBrakeHold.h', self.rival_h)
        for marker in (
            "RemainingSeconds = 0.0",
            "MaxFrameSeconds = 0.25",
            "HoldSeconds = 0.18",
            "planned.Throttle = 0.0",
            "planned.Brake = true",
            "if (planned.Arrived)",
        ):
            self.assertIn(marker, self.hold)

    def test_noncompetitive_and_source_test_coverage(self):
        for forbidden in (
            "PassCheckpoint(", "AddTimePenalty(", "EarnCredits(",
            "SaveGame", "SetPhysicsLinearVelocity", "AddForce(", "AddTorque(",
            "GetGarage()", "UCLASS(", "AActor",
        ):
            self.assertNotIn(forbidden, self.hold + self.policy)
        self.assertIn("RivalFollowingBrakeHoldTests.cpp", self.cmake)
        self.assertIn("NAME RivalFollowingBrakeHold", self.cmake)


if __name__ == "__main__":
    unittest.main()
