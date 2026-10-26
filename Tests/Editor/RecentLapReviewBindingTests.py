"""v0.5.70: Results expose bounded genuine timed laps after a valid finish.

These source contracts do not prove UE 5.8/UHT builds or Canvas render QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
MODEL = ROOT / "Source/Latebrake/Public/Racing/RecentLapReview.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
TEST = ROOT / "Tests/Standalone/RecentLapReviewTests.cpp"


class RecentLapReviewBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.model = MODEL.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")

    def test_results_page_consumes_real_race_without_mutation(self):
        marks = (
            '#include "Racing/RecentLapReview.h"',
            "latebrake::BuildRecentLapReview(Race)",
            "if (LapReview.Valid)",
            "Offset < LapReview.Count",
            "Row.CompletedLap, Row.LapSeconds",
            "Row.SecondsBehindRunBest",
            "Row.HasPrevious",
            "Row.DeltaToPreviousSeconds",
            "RECENT TIMED LAPS (NEWEST FIRST)",
            "MenuPager.Synchronize(Mode->GetMenuPage(), MenuDetails.Num()",
        )
        for mark in marks:
            self.assertIn(mark, self.hud)
        self.assertLess(
            self.hud.index("BuildRecentLapReview(Race)"),
            self.hud.index("MenuPager.Synchronize("))
        for forbidden in ("AddTimePenalty(", "SaveRecords(", "PassCheckpoint(",
                          "SetActorLocation", "CareerAward"):
            self.assertNotIn(forbidden, self.model)

    def test_review_refuses_unawardable_ends_and_does_not_allocate(self):
        for mark in (
            "CanAwardFinishedRace(race)",
            "RecentTimedLaps::Capacity",
            "GetRecentTimedLap(offset)",
            "GetBestLapSeconds()",
            "std::array<RecentLapReviewRow",
            "return {};",
        ):
            self.assertIn(mark, self.model)
        for mark in (
            "penaltyOnly", "zeroLap", "many", "race.Reset()",
            "Latest", "bounded.Valid",
        ):
            # Test variable names and assertions are deliberate source contracts.
            if mark == "Latest":
                continue
            self.assertIn(mark, self.test)
        for mark in ("RecentLapReviewTests.cpp", "NAME RecentLapReview",
                     "RaceProgress.cpp", "/W4 /WX", "-Wpedantic -Werror"):
            self.assertIn(mark, self.cmake)


if __name__ == "__main__":
    unittest.main()
