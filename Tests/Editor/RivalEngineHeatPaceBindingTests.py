"""Real rival heat pacing bindings: source contracts only, not Unreal QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RivalEngineHeatPaceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = read("Source/Latebrake/Public/AI/LatebrakeRivalCar.h")
        cls.rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.pace = read("Source/Latebrake/Public/AI/RivalEngineHeatPace.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.docs = read("docs/RIVAL_ENGINE_HEAT_PACE_QA.md")

    def test_reads_real_inherited_heat_only_after_all_preconditions(self):
        self.assertIn('#include "AI/RivalEngineHeatPace.h"', self.rival)
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        markers = (
            "Mode->IsRaceStartGreen()",
            "IsRacePhysicsReady()",
            "latebrake::CanDriveRival(",
            "Route.ObserveVerifiedGate(",
            "GripPace.Apply(Command, {",
            "SurfaceDamagePace.Apply(Command, {",
            "GetEngineHeat().HeatFraction()",
            "GetEngineHeat().HasValidSample()",
            "ModerateRivalEngineHeatPace(Command, {",
            "latebrake::YieldRivalToPlayer(Command, {",
            "FollowingBrakeHold.Apply(",
            "SpinThrottleGuard.Apply(",
            "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)"
        )
        offsets = [tick.index(marker) for marker in markers]
        self.assertEqual(offsets, sorted(offsets))

    def test_pacing_cannot_create_race_authority_or_force(self):
        for banned in (
            "UCLASS(", "GetWorld(", "AwardCredits(", "PassCheckpoint(",
            "SetActorLocation(", "SetAutonomousControls(",
            "SaveGame", "RaceProgress", "AddForce(", "AddTorque("
        ):
            self.assertNotIn(banned, self.pace)
        self.assertIn("command.Throttle = std::min(command.Throttle, allowed)",
                      self.pace)
        self.assertIn("if (!sample.HeatSampleReady", self.pace)
        self.assertIn("command.Arrived || command.Brake", self.pace)
        self.assertIn("return {};", self.pace)

    def test_portable_suite_and_release_honesty(self):
        self.assertIn("RivalEngineHeatPaceTests.cpp", self.cmake)
        self.assertIn("NAME RivalEngineHeatPace", self.cmake)
        self.assertIn("200000", read(
            "Tests/Standalone/RivalEngineHeatPaceTests.cpp"))
        self.assertIn("Unreal Engine 5.8", self.docs)
        self.assertIn("source-only", self.docs.lower())


if __name__ == "__main__":
    unittest.main()
