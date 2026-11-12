"""Nonshipping six-car terminal-fault and independent event countdown binding.

Source-only: no UHT, real Chaos car, owned cooked level, installed Win64 or
real engine start-light timing is verified by this test.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class ExperimentalFleetStartFaultBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.mode_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.policy = read("Source/Latebrake/Public/AI/ExperimentalFleetStartPolicy.h")
        cls.tests = read("Tests/Standalone/ExperimentalFleetStartPolicyTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.nav = read("mkdocs.yml")

    def test_immutable_dev_flag_and_per_attempt_terminal_fault_are_distinct(self):
        self.assertIn("bool bExperimentalSixCarGrid = false;", self.mode_h)
        self.assertIn("bool bExperimentalFleetFaulted = false;", self.mode_h)
        self.assertIn("FParse::Param(", self.mode)
        self.assertEqual(self.mode.count("bExperimentalSixCarGrid = false;"), 0)
        self.assertIn("DecideExperimentalFleetStart({", self.mode)
        self.assertIn("SelectedEvent == latebrake::EventKind::Circuit", self.mode)
        self.assertIn("bCareerRace", self.mode)
        self.assertIn("FleetStart.MayAttemptPhysicalSpawn", self.mode)
        self.assertIn("const bool bFleetStartReady = FleetStart.PermitCountdown ||",
                      self.mode)
        self.assertIn("!bExperimentalFleetFaulted &&", self.mode)
        self.assertIn("bFleetStartReady;", self.mode)

    def test_only_accepted_restart_clears_latched_fault(self):
        self.assertEqual(self.mode.count("bExperimentalFleetFaulted = false;"), 1)
        restart = self.mode.split(
            "void ALatebrakeRaceGameMode::RestartRace()", 1
        )[1].split("void ALatebrakeRaceGameMode::RecoverPlayer(", 1)[0]
        self.assertIn("ReplayWrite.Commit();", restart)
        self.assertIn("bExperimentalFleetFaulted = false;", restart)
        self.assertIn("DestroyExperimentalRivals();", restart)
        self.assertLess(restart.index("ReplayWrite.Commit();"),
                        restart.index("bExperimentalFleetFaulted = false;"))
        self.assertLess(restart.index("bExperimentalFleetFaulted = false;"),
                        restart.index("RestartPlacement.OnRestartRequested();"))

    def test_physical_spawn_and_geometry_failures_are_terminal_this_attempt(self):
        spawn = self.mode.split(
            "void ALatebrakeRaceGameMode::RefreshExperimentalRivals()", 1
        )[1].split(
            "bool ALatebrakeRaceGameMode::IsRegisteredRivalActor(", 1
        )[0]
        self.assertIn("if (bExperimentalFleetFaulted)", spawn)
        self.assertIn("if (bExperimentalSixCarGrid && !bCareerRace &&", spawn)
        self.assertIn("!IsPlannedStartGridSafe()", spawn)
        self.assertIn("ArePlannedRivalRecoveryLanesSafe()", spawn)
        self.assertIn("EXPERIMENTAL_GRID_FAULT geometry invalid", spawn)
        self.assertIn("EXPERIMENTAL_GRID_FAULT physical spawn/verification failed",
                      spawn)
        self.assertGreaterEqual(spawn.count("bExperimentalFleetFaulted = true;"), 3)
        self.assertNotIn("bExperimentalSixCarGrid = false;", spawn)
        self.assertIn("DestroyExperimentalRivals();", spawn)

    def test_pure_policy_cases_and_strict_portable_target(self):
        for marker in (
            "DeveloperRequested", "CircuitEvent", "CareerEvent",
            "TerminalSpawnFault", "RosterPresent", "AllPhysicalCarsReady",
            "OriginalCircuitReady", "LocalDriverReady", "PrimaryRivalReady",
            "Inactive", "RaceFinished", "LightsAlreadyGreen",
            "RequiredForEvent", "MayAttemptPhysicalSpawn",
            "PermitCountdown", "BlockedByTerminalFault",
            "s.DeveloperRequested && s.CircuitEvent && !s.CareerEvent",
            "!required || (!fault && s.AllPhysicalCarsReady)",
        ):
            self.assertIn(marker, self.policy)
        for forbidden in (
            "UCLASS(", "SpawnActor(", "GetWorld(", "SaveRecords(",
            "PassCheckpoint(", "AddTimePenalty(", "SetActorLocation(",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("mask < 4096", self.tests)
        self.assertIn("ExperimentalFleetStartPolicyTests.cpp", self.cmake)
        self.assertIn("NAME ExperimentalFleetStartPolicy", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("EXPERIMENTAL_FLEET_START_FAULT_QA.md", self.nav)


if __name__ == "__main__":
    unittest.main()
