"""Source-level binding verification for an observed rival final-gate clock.

Does NOT run Unreal/UHT, the real race, or certify physics-frame order.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODE_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
MODE_CPP = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
HUD_CPP = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
POLICY = ROOT / "Source/Latebrake/Public/Racing/RivalFinishClockReview.h"
NAV = ROOT / "mkdocs.yml"


class UnrealRivalFinishClockBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode_header = MODE_H.read_text(encoding="utf-8")
        cls.mode_cpp = MODE_CPP.read_text(encoding="utf-8")
        cls.hud_cpp = HUD_CPP.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_one_review_state_per_attempt_and_new_race(self):
        for token in (
            '#include "Racing/RivalFinishClockReview.h"',
            "latebrake::RivalFinishClockReview RivalFinishClocks;",
            "latebrake::RivalFinishClockComparison LastRivalFinishClockComparison;",
            "GetLastRivalFinishClockComparison() const",
        ):
            self.assertIn(token, self.mode_header)
        self.assertEqual(
            self.mode_cpp.count(
                "RivalFinishClocks.Configure("
                "Race.GetCheckpointCount(), Race.GetRequiredLaps());"
            ), 2
        )
        self.assertEqual(
            self.mode_cpp.count("LastRivalFinishClockComparison = {};"), 3
        )

    def test_only_actual_accepted_rival_final_gate_can_record(self):
        observer = self.mode_cpp.split(
            "void ALatebrakeRaceGameMode::ObserveRivalVerifiedGate()", 1
        )[1].split(
            "bool ALatebrakeRaceGameMode::TryGetLatestRivalMatchedGateSplit(", 1
        )[0]
        for token in (
            "TryGetRivalCreditedGateGap(Eligibility)",
            "RivalSplits.RecordRival(",
            "if (bAcceptedSplit && !bCareerRace &&",
            "SelectedEvent == latebrake::EventKind::Circuit",
            "RivalCar->HasCompletedRoute()",
            "RivalFinishClocks.RecordRivalAcceptedGate(",
        ):
            self.assertIn(token, observer)
        for prohibited in (
            "SaveRecords(", "EarnCredits(", "Race.PassCheckpoint(",
            "Race.Advance(", "Career.CommitFinish(",
        ):
            self.assertNotIn(prohibited, observer)

    def test_player_finish_timing_eligibility_precedes_review(self):
        final_handler = self.mode_cpp.split(
            "void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::", 1
        )[0]
        for expected in (
            "latebrake::ValidateFinishTiming(Race)",
            "LastRivalFinishClockComparison = RivalFinishClocks.CompareAtPlayerFinish(",
            "Race.GetElapsedSeconds() - Race.GetTotalPenaltySeconds()",
            "RivalFinishObservationState::RivalRouteAlreadyComplete",
            "!bCareerRace && SelectedEvent == latebrake::EventKind::Circuit",
        ):
            self.assertIn(expected, final_handler)
        self.assertLess(
            final_handler.index("ValidateFinishTiming(Race)"),
            final_handler.index(
                "RivalFinishClocks.CompareAtPlayerFinish("
            )
        )
        self.assertLess(
            final_handler.index("RivalFinishClocks.CompareAtPlayerFinish("),
            final_handler.index("ProfileWriteCheckpoint SavedProfile("),
        )
        self.assertIn("NearFinishToleranceSeconds = 0.25;", self.policy)

    def test_results_are_uncertain_and_never_award_a_win(self):
        results = self.hud_cpp.split(
            "case latebrake::MenuPage::Results:", 1
        )[1].split(
            "case latebrake::MenuPage::", 1
        )[0]
        for token in (
            "GetLastRivalFinishClockComparison()",
            "RivalFinishClockState::RivalEarlierObserved",
            "RivalFinishClockState::NearSimultaneousUncertain",
            "UNOFFICIAL: RIVAL FINAL GATE OBSERVED",
            "UNOFFICIAL: FINAL GATES TOO CLOSE TO TIME",
        ):
            self.assertIn(token, results)
        cmake = CMAKE.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        self.assertIn("RivalFinishClockReviewTests.cpp", cmake)
        self.assertIn("NAME RivalFinishClockReview", cmake)
        self.assertIn("RIVAL_FINISH_CLOCK_REVIEW_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
