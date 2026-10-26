"""v0.5.71 Results insight admission and actual Unreal source binding.

Pure source contracts are not licensed Unreal Engine compilation, cooked
assets, rendered Canvas pixel assertions, or installed device test evidence.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
STATS = ROOT / "Source/Latebrake/Public/Racing/RecentLapPaceStatistics.h"
SECTORS = ROOT / "Source/Latebrake/Public/Racing/BestSectorReview.h"
DRIVING = ROOT / "Source/Latebrake/Public/Racing/SampledDrivingReview.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
TESTS = ROOT / "Tests/Standalone"


class ResultsInsightsBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.stats = STATS.read_text(encoding="utf-8")
        cls.sectors = SECTORS.read_text(encoding="utf-8")
        cls.driving = DRIVING.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_real_results_menu_shows_grounded_sampled_data(self):
        for fragment in (
            '#include "Racing/RecentLapPaceStatistics.h"',
            '#include "Racing/BestSectorReview.h"',
            '#include "Racing/SampledDrivingReview.h"',
            "CalculateRecentLapPaceStatistics(Race)",
            "BuildBestSectorReview(Race)",
            "BuildSampledDrivingReview(",
            "Car->GetDrivingStats()",
            "PaceStatistics.SampleCount",
            "PaceStatistics.MedianSeconds",
            "PaceStatistics.PopulationDeviationSeconds",
            "PaceStatistics.ConsecutiveImprovements",
            "SectorReview.MixedBestSectorsSeconds",
            "SectorReview.SectorSeconds[Index]",
            "DriveReview.EstimatedDistanceKm",
            "DriveReview.BrakingPercent",
            "DriveReview.HighLateralSlipPercent",
            "MenuPager.Synchronize(Mode->GetMenuPage(), MenuDetails.Num()",
        ):
            self.assertIn(fragment, self.hud)
        self.assertLess(
            self.hud.index("CalculateRecentLapPaceStatistics(Race)"),
            self.hud.index("MenuPager.Synchronize("))
        self.assertLess(
            self.hud.index("BuildBestSectorReview(Race)"),
            self.hud.index("MenuPager.Synchronize("))
        self.assertIn("if (!bCompact)", self.hud)

    def test_all_policies_are_read_only_and_fail_closed(self):
        for src in (self.stats, self.sectors, self.driving):
            for forbidden in (
                "SetActorLocation", "PassCheckpoint(", "Advance(",
                "AddTimePenalty(", "SaveRecords(", "CareerAward(",
                "NewObject(", "FString", "UCLASS(", "PlayerController",
            ):
                self.assertNotIn(forbidden, src)
            self.assertIn("bool Valid = false;", src)
            self.assertIn("return result;", src)
        for frag in ("CanAwardFinishedRace(race)", "RecentTimedLaps::Capacity",
                     "std::sort(", "ConsecutiveImprovements"):
            self.assertIn(frag, self.stats)
        for frag in ("CanAwardFinishedRace(race)", "GetBestSectorSeconds(index)",
                     "GetTheoreticalBestLapSeconds()", "SectorSeconds{}"):
            self.assertIn(frag, self.sectors)
        for frag in ("std::isfinite(time)", "brakes > frames",
                     "HighLateralSlipPercent", "AcceptedFrames"):
            self.assertIn(frag, self.driving)

    def test_production_linked_cpp_targets_are_strict(self):
        for name, source, production in (
            ("RecentLapPaceStatistics", "RecentLapPaceStatisticsTests.cpp",
             "RaceProgress.cpp"),
            ("BestSectorReview", "BestSectorReviewTests.cpp",
             "RaceProgress.cpp"),
            ("SampledDrivingReview", "SampledDrivingReviewTests.cpp",
             "DrivingSessionStats.cpp"),
        ):
            with self.subTest(name=name):
                self.assertIn(f"add_test(NAME {name}", self.cmake)
                self.assertIn(source, self.cmake)
                self.assertIn(production, self.cmake)
                self.assertIn("/W4 /WX", self.cmake)
                self.assertIn("-Wpedantic -Werror", self.cmake)
                self.assertTrue((TESTS / source).is_file())


if __name__ == "__main__":
    unittest.main()
