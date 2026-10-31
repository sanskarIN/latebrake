"""Unreal source-bound rival pace verification; not an engine or device test."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RivalGripPaceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn_header = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.rival_header = read("Source/Latebrake/Public/AI/LatebrakeRivalCar.h")
        cls.rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.policy = read("Source/Latebrake/Public/AI/RivalGripPace.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.guide = read("docs/RIVAL_GRIP_PACE_QA.md")

    def test_only_existing_rival_uses_grip_governor(self):
        self.assertIn('#include "AI/RivalGripPace.h"', self.rival_header)
        self.assertIn("latebrake::RivalGripPace GripPace;", self.rival_header)
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        for snippet in (
            "Mode->IsRaceStartGreen()", "IsRacePhysicsReady()",
            "latebrake::CanDriveRival(", "Mode->HasLocalPlayerDriver()",
            "latebrake::PlanRivalTurn(", "latebrake::AnticipateRivalCorner(",
            "GetTireCondition().GripMultiplier()",
            "GetTireCondition().HasValidSample()",
            "Velocity, GetActorRightVector()",
            "GripPace.Apply(Command, {",
            "latebrake::YieldRivalToPlayer(Command, {",
            "FollowingBrakeHold.Apply(",
            "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)",
            "Super::Tick(DeltaSeconds);",
        ):
            self.assertIn(snippet, tick)
        self.assertLess(tick.index("AnticipateRivalCorner("),
                        tick.index("GripPace.Apply("))
        self.assertLess(tick.index("GripPace.Apply("),
                        tick.index("YieldRivalToPlayer("))
        self.assertLess(tick.index("YieldRivalToPlayer("),
                        tick.index("FollowingBrakeHold.Apply("))
        self.assertIn("GetTireCondition() const", self.pawn_header)

    def test_no_stale_telemetry_after_discontinuity(self):
        for method in ("ResetPilot", "ConfigurePilot"):
            block = self.rival.split(
                "void ALatebrakeRivalCar::" + method + "(", 1
            )[1].split("void ALatebrakeRivalCar::", 1)[0]
            self.assertIn("GripPace.Reset();", block)
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        self.assertGreaterEqual(tick.count("GripPace.Reset();"), 3)
        for stop in ("if (!Mode || !Mode->IsRaceStartGreen()",
                     "if (Observation.NeedsRecovery)",
                     "if (Observation.Completed)"):
            self.assertIn(stop, tick)

    def test_grip_policy_is_bounded_and_has_no_scoring_authority(self):
        for required in ("std::isfinite(", "MaximumFrameSeconds = 0.10",
                         "MinimumSampleGrip = 0.68",
                         "BrakeHysteresisSeconds = 0.12",
                         "std::clamp(", "std::min(command.Throttle, permitted)",
                         "command.Brake = true", "LastGripLimited",
                         "return command;", "return {};"):
            self.assertIn(required, self.policy)
        for forbidden in ("UCLASS(", "AddForce(", "AddTorque(", "SaveGame",
                          "RaceProgress", "AwardCredits(", "PassCheckpoint(",
                          "UE_LOG(", "SetAutonomousControls("):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("NAME RivalGripPace", self.cmake)
        self.assertIn("RivalGripPaceTests.cpp", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)
        self.assertIn("Unreal Engine 5.8", self.guide)
        self.assertIn("source-only", self.guide.lower())


if __name__ == "__main__":
    unittest.main()
