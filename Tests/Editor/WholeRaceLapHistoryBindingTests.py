"""Whole-attempt lap aggregate is fed only from the authoritative race clock."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[2]


class WholeRaceLapHistoryBindingTests(unittest.TestCase):
    def test_authoritative_gate_and_reset(self):
        source = (ROOT / "Source/Latebrake/Private/Racing/RaceProgress.cpp").read_text()
        self.assertEqual(source.count("WholeLapHistory.Observe("), 1)
        reset = source.split("void RaceProgress::Reset()", 1)[1].split("void RaceProgress::Advance", 1)[0]
        self.assertIn("WholeLapHistory.Reset();", reset)
        gate = source.split("bool RaceProgress::PassCheckpoint(", 1)[1]
        self.assertLess(gate.index("++CompletedLaps;"), gate.index("WholeLapHistory.Observe("))
        self.assertLess(gate.index("LastLapPenaltySeconds ="), gate.index("WholeLapHistory.Observe("))
        self.assertIn("lapHadActiveClockEvidence, LastLapPenaltySeconds", gate)
        header = (ROOT / "Source/Latebrake/Public/Racing/RaceProgress.h").read_text()
        self.assertIn("WholeLapHistory.Review(CompletedLaps,LapStartSeconds,LapPenaltyStartSeconds)", header)

    def test_full_summary_distinct_from_recent_window(self):
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text()
        block = hud.split("// Whole-run statistics", 1)[1].split("// Fee-free physical laps", 1)[0]
        self.assertIn("CanAwardFinishedRace(Race)", block)
        self.assertIn("Race.GetWholeRaceLapSummary()", block)
        self.assertIn("Whole.All.Count>latebrake::RecentTimedLaps::Capacity", block)
        self.assertIn("FULL RUN:", block)
        self.assertNotIn("SaveRecords(", block)
        self.assertIn("BuildCleanLapPaceReview(Race)", hud)

    def test_linked_actual_race_clock(self):
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text()
        self.assertIn("add_latebrake_portable_research_test(WholeRaceLapHistory)", cmake)
        self.assertIn("target_sources(latebrake_research_WholeRaceLapHistory PRIVATE", cmake)


if __name__ == "__main__":
    unittest.main()
