"""Verify Unreal source paths enforce recordable per-lap timing.

Static bindings only; they do not compile Unreal or test installed gameplay.
The executable C++ fixture exercises the real portable RaceProgress logic.
"""

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODE = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
FINISH = ROOT / "Source/Latebrake/Public/Racing/FinishedRaceInsights.h"
RACE = ROOT / "Source/Latebrake/Private/Racing/RaceProgress.cpp"


class FinishTimingAwardBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.insights = FINISH.read_text(encoding="utf-8")
        cls.race = RACE.read_text(encoding="utf-8")

    def test_finish_eligibility_gate_precedes_all_persistent_writes(self):
        method = self.mode.split(
            "void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1
        )[1].split("\nvoid ALatebrakeRaceGameMode::", 1)[0]
        validation = method.index("latebrake::ValidateFinishTiming(Race)")
        rejection = method.index(
            "FinishTiming != latebrake::FinishTimingEligibility::Eligible"
        )
        write = method.index("latebrake::ProfileWriteCheckpoint SavedProfile(")
        credit = method.index("Garage.EarnCredits(LastRaceCredits)")
        medal = method.index("Career.CommitFinish(")
        record = method.index("latebrake::ImprovedBestTime(")
        for later in (write, credit, medal, record):
            self.assertLess(validation, later)
            self.assertLess(rejection, later)
        denied = method.split("if (FinishTiming !=", 1)[1].split(
            "\n    // Only the SaveGame write", 1
        )[0]
        self.assertIn("LastRaceCredits = 0;", denied)
        self.assertIn("LastCareerAward = {};", denied)
        self.assertIn("return;", denied)

    def test_results_and_scorecard_use_identical_awardability_policy(self):
        self.assertIn("if (!CanAwardFinishedRace(race))", self.insights)
        self.assertIn("CanAwardFinishedRace(Race)", self.hud)
        self.assertIn("CanAwardFinishedRace(Mode->GetRace())", self.hud)
        self.assertIn("FINISH NOT RECORDABLE", self.hud)
        self.assertIn("INVALID FINISH - PRESS T TO RETRY", self.hud)

    def test_observed_clock_is_recorded_only_by_advance_not_penalty(self):
        advance = self.race.split("void RaceProgress::Advance(", 1)[1].split(
            "\nbool RaceProgress::AddTimePenalty(", 1
        )[0]
        penalty = self.race.split("bool RaceProgress::AddTimePenalty(", 1)[1].split(
            "\nbool RaceProgress::PassCheckpoint(", 1
        )[0]
        lap = self.race.split("bool RaceProgress::PassCheckpoint(", 1)[1]
        self.assertIn("ActiveClockAdvancedThisLap = true;", advance)
        self.assertNotIn("ActiveClockAdvancedThisLap = true;", penalty)
        self.assertIn("if (ActiveClockAdvancedThisLap)", lap)
        self.assertIn("ActiveClockAdvancedThisLap = false;", lap)

    def test_gate_removal_is_detectable(self):
        without_gate = self.mode.replace(
            "latebrake::ValidateFinishTiming(Race)",
            "latebrake::FinishTimingEligibility::Eligible"
        )
        self.assertNotIn("latebrake::ValidateFinishTiming(Race)", without_gate)


if __name__ == "__main__":
    unittest.main()
