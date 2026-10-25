"""v0.5.65 source-binding contracts for real HUD opponent approach trend.

This does not compile the Unreal Editor, UHT, Chaos vehicle or packaged game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD_HEADER = ROOT / "Source/Latebrake/Public/UI/LatebrakeRaceHUD.h"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
TRACKER = ROOT / "Source/Latebrake/Public/Track/RivalRelativeMotion.h"
TEST = ROOT / "Tests/Standalone/RivalRelativeMotionTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class RivalRelativeMotionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HUD_HEADER.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.tracker = TRACKER.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_one_real_radar_snapshot_drives_both_indicators(self):
        self.assertIn('#include "Track/RivalRelativeMotion.h"', self.header)
        self.assertIn("latebrake::RivalRelativeMotionTracker RivalRelativeMotionHint;", self.header)
        self.assertEqual(self.hud.count("BuildRaceMiniMapSnapshot("), 1)
        for marker in ("RivalProximityHint.Update(", "RivalRelativeMotionHint.Update(",
                       "bWayfindingEligible && bRivalRace",
                       "RivalTrend.SeparationChangeMetersPerSecond",
                       "RIVAL CLOSING | %.0fm/s DIRECT",
                       "RIVAL CLOSING | %.1fm/s DIRECT RATE",
                       "RIVAL SEPARATING | %.1fm/s DIRECT RATE"):
            self.assertIn(marker, self.hud)
        self.assertLess(self.hud.index("BuildRaceMiniMapSnapshot("),
                        self.hud.index("RivalRelativeMotionHint.Update("))
        self.assertLess(self.hud.index("RivalRelativeMotionHint.Update("),
                        self.hud.index("if (!bCompact && Canvas->SizeX >= 1120)"))

    def test_reset_lifecycle_matches_proximity_warnings(self):
        self.assertGreaterEqual(self.hud.count("RivalRelativeMotionHint.Reset();"), 4)
        self.assertEqual(self.hud.count("RivalRelativeMotionHint.Reset();"),
                         self.hud.count("RivalProximityHint.Reset();"))
        for marker in ("LastWayfindingPawn.Get() != Car",
                       "LastWayfindingSession != CurrentWayfindingSession",
                       "LastWayfindingTeleport != CurrentWayfindingTeleport",
                       "LastRivalVerifiedRecoveries != CurrentRivalVerifiedRecoveries"):
            self.assertIn(marker, self.hud)
        for marker in ("radar.DirectRivalSeparationValid", "deltaSeconds < 0.01",
                       "deltaSeconds > 0.25", "std::abs(radial) > 90.0",
                       "ConfirmSeconds = 0.4", "MinMotionMetersPerSecond = 2.0",
                       "std::min(ConfirmSeconds, CandidateSeconds + deltaSeconds)"):
            self.assertIn(marker, self.tracker)

    def test_no_scoring_mutations_and_cross_platform_production_geometry(self):
        for forbidden in ("PassCheckpoint(", "AddTimePenalty(", "SaveRecords(",
                          "SetAutonomousControls(", "SetActorLocation("):
            self.assertNotIn(forbidden, self.tracker)
        for marker in ("RivalRelativeMotionTests.cpp", "NAME RivalRelativeMotion",
                       "Private/Track/RaceMiniMap.cpp", "cxx_std_20",
                       "/W4 /WX", "-Wpedantic -Werror"):
            self.assertIn(marker, self.cmake)
        for marker in ("RivalRelativeMotionTracker", "RivalRelativeTrend::Closing",
                       "RivalRelativeTrend::Opening", "snapshot(30.0)",
                       "snapshot(1.0)", "std::numeric_limits<double>::quiet_NaN()"):
            self.assertIn(marker, self.test)


if __name__ == "__main__":
    unittest.main()
