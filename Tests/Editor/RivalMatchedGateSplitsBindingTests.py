"""Source bindings for physically accepted local rival gate split times.

Does not claim UnrealHeaderTool, editor, packaged Win64 or Android QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RivalMatchedGateSplitsBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = load("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.header = load("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.rival = load("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.hud = load("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy = load("Source/Latebrake/Public/Racing/RivalMatchedGateSplits.h")
        cls.cmake = load("Tests/Standalone/CMakeLists.txt")

    def test_existing_accepted_gates_feed_observations(self):
        self.assertIn('#include "Racing/RivalMatchedGateSplits.h"', self.header)
        self.assertIn("latebrake::RivalMatchedGateSplits RivalSplits;", self.header)
        self.assertIn("TryGetLatestRivalMatchedGateSplit(", self.header)
        self.assertIn("void ObserveRivalVerifiedGate();", self.header)
        player_gate = self.mode.split(
            "void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1)[1].split(
            "#if !UE_BUILD_SHIPPING", 1)[0]
        self.assertLess(player_gate.index("Race.PassCheckpoint(CheckpointIndex)"),
                        player_gate.index("RivalSplits.RecordPlayer("))
        self.assertIn("Race.GetElapsedSeconds() - Race.GetTotalPenaltySeconds()",
                      player_gate)
        rival_gate = self.rival.split("if (Observation.Advanced)", 1)[1].split(
            "if (Observation.NeedsRecovery)", 1)[0]
        self.assertIn("Mode->ObserveRivalVerifiedGate();", rival_gate)
        self.assertLess(self.rival.index("Route.ObserveVerifiedGate("),
                        self.rival.index("Mode->ObserveRivalVerifiedGate();"))
        self.assertGreaterEqual(self.mode.count("RivalSplits.Configure("), 2)
        self.assertIn("TryGetRivalCreditedGateGap(Eligibility)", self.mode)

    def test_read_only_wide_screen_hud(self):
        section = self.hud.split("// Optional SAME-GATE verified split timing", 1)[1].split(
            "if (RivalProximityHint.Update(", 1)[0]
        for marker in ("!bCompact", "VerticalPlan.MaxTelemetryLines >= 28",
                       "TryGetLatestRivalMatchedGateSplit(", "SPLIT L%d G%d:",
                       "NOT PLACE"):
            self.assertIn(marker, section)
        for forbidden in ("RegisterCheckpoint(", "PassCheckpoint(",
                          "EarnCredits(", "SaveRecords(", "AddForce(",
                          "SetActorLocation(", "DrawRect("):
            self.assertNotIn(forbidden, section)
            self.assertNotIn(forbidden, self.policy)

    def test_portable_warnings_are_errors(self):
        self.assertIn("RivalMatchedGateSplitsTests.cpp", self.cmake)
        self.assertIn("NAME RivalMatchedGateSplits", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)


if __name__ == "__main__":
    unittest.main()
