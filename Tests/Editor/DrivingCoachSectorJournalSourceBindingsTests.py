"""Bind the advisory journal only after GameMode accepted the player gate.

Checks source contracts only: not a native UE/UHT/Win64 test. The policy cannot
award, invent or reorder checkpoint crossings, and never owns race time.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]


def source(path):
    return (ROOT/path).read_text(encoding="utf-8")


class DrivingCoachSectorJournalSourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode_header=source("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.mode=source("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.hud=source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy=source("Source/Latebrake/Public/Racing/DrivingCoachSectorJournal.h")
        cls.cmake=source("Tests/Standalone/CMakeLists.txt")

    def test_gate_witness_after_exact_real_player_acceptance(self):
        gate=self.mode.split("void ALatebrakeRaceGameMode::RegisterCheckpoint(",1)[1].split(
            "void ALatebrakeRaceGameMode::",1)[0]
        self.assertIn("Race.PassCheckpoint(CheckpointIndex)",gate)
        self.assertIn("CoachSectorJournal.ObserveGate(",gate)
        self.assertLess(gate.index("Race.PassCheckpoint(CheckpointIndex)"),
                        gate.index("CoachSectorJournal.ObserveGate("))
        self.assertIn("PlayerGateOrdinal",gate)
        self.assertIn("GetCoachReview()",gate)
        self.assertIn("VerifyDrivingCoachIntegrity(",gate)

    def test_restart_and_recovery_stop_inherited_coverage(self):
        self.assertIn('"Racing/DrivingCoachSectorJournal.h"',self.mode_header)
        self.assertIn("DrivingCoachSectorJournal CoachSectorJournal;",self.mode_header)
        self.assertIn("GetPlayerCoachSectorReview()",self.mode_header)
        self.assertGreaterEqual(self.mode.count("CoachSectorJournal.Reset("),2)
        self.assertIn("CoachSectorJournal.Suspend();",self.mode)
        self.assertIn("if (Notice!=latebrake::PlayerRecoveryNotice::VerifiedAndCharged)",
                      self.mode)

    def test_results_only_display_and_no_authority(self):
        self.assertIn("Mode->GetPlayerCoachSectorReview()",self.hud)
        self.assertIn("SAMPLED GATE INTERVAL",self.hud)
        for forbidden in ("Race.PassCheckpoint(", "SaveRecords(",
                          "Garage.EarnCredits(","ApplyGarageToPlayer("):
            self.assertNotIn(forbidden,self.policy)
        for present in ("ValidDrivingCoachSectorSample(",
                        "PausedOrRecovered", "SourceClockMismatch",
                        "LastOrdinal",
                        "ChassisCoverageFraction", "ActorToken"):
            self.assertIn(present,self.policy)

    def test_portable_source_binding_is_registered(self):
        self.assertIn("DrivingCoachSectorJournalTests.cpp",self.cmake)
        self.assertIn("NAME DrivingCoachSectorJournal",self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror",self.cmake)


if __name__ == "__main__":
    unittest.main()
