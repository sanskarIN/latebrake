"""Source-only Unreal HUD binding checks: accepted-gate display, not scoring.

No Unreal Engine rendering, game execution or hardware testing is claimed.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class RaceGateProgressGaugeBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")
        cls.policy = (ROOT / "Source/Latebrake/Public/UI/RaceGateProgressGauge.h").read_text(encoding="utf-8")
        cls.cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")

    def test_real_race_progress_is_source_of_truth(self):
        for expected in (
            '#include "UI/RaceGateProgressGauge.h"',
            "latebrake::CalculateRaceGateProgressGauge(",
            "Race.GetCompletedLaps(), Race.GetNextCheckpoint(),",
            "Race.GetRequiredLaps(), Race.GetCheckpointCount(),",
            "Race.IsFinished()",
            "GateGauge.Valid",
            "GateGauge.CreditedGates, GateGauge.TotalGates",
            "DrawMeter(*GateLabel, GateGauge.Fraction, 64.f, Gold)",
        ):
            with self.subTest(marker=expected):
                self.assertIn(expected, self.hud)

    def test_progress_is_display_only(self):
        panel = self.hud.split("    // Read-only instrument panel.", 1)[1].split(
            "    // A deterministic footer", 1)[0]
        for forbidden in (
            "PassCheckpoint(", "RegisterCheckpoint(", "AddTimePenalty(",
            "EarnCredits(", "SaveRecords(", "AddForce(", "SetGarageTuning("
        ):
            self.assertNotIn(forbidden, panel)
        self.assertIn("std::numeric_limits<int>::max() / checkpointCount", self.policy)
        self.assertIn("raceFinished != (completedLaps == requiredLaps)", self.policy)
        self.assertIn("checkpointCount > 256", self.policy)

    def test_portable_suite_registered(self):
        self.assertIn("RaceGateProgressGaugeTests.cpp", self.cmake)
        self.assertIn("NAME RaceGateProgressGauge", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)


if __name__ == "__main__":
    unittest.main()
