"""Source-level binding regression; does not compile or execute Unreal Engine."""

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
TIMING = ROOT / "Source/Latebrake/Public/Racing/LiveSectorTarget.h"


class LiveSectorHudBindingTests(unittest.TestCase):
    def test_real_unreal_hud_uses_same_portable_predicate_as_tests(self):
        hud = HUD.read_text(encoding="utf-8")
        self.assertIn('#include "Racing/LiveSectorTarget.h"', hud)
        self.assertIn("latebrake::MeasureLiveSectorTarget(Race)", hud)
        self.assertIn("if (SectorTarget.Valid)", hud)
        self.assertIn("SectorTarget.OverBest", hud)
        self.assertIn("SectorTarget.SecondsUntilBest", hud)
        self.assertIn("SectorTarget.SecondsOverBest", hud)
        self.assertIn("OVER PREVIOUS BEST", hud)
        self.assertIn("UNTIL PREVIOUS BEST", hud)

    def test_sector_reference_never_predicts_unmeasured_time(self):
        portable = TIMING.read_text(encoding="utf-8")
        self.assertIn("race.GetBestSectorSeconds(sector)", portable)
        self.assertIn("race.GetCurrentSectorSeconds()", portable)
        self.assertIn("race.GetTimedLapCount() < 1", portable)
        self.assertIn("race.IsFinished()", portable)
        self.assertIn("result.OverBest = elapsed > best", portable)


if __name__ == "__main__":
    unittest.main()
