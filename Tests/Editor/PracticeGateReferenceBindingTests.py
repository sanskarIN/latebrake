"""Verify native practice gate receipts and nonpersistent comparison binding."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT/path).read_text(encoding="utf-8")


class PracticeGateReferenceBindingTests(unittest.TestCase):
    def test_actual_gate_receipts_and_qualified_finish(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        finish=mode.split("void ALatebrakeRaceGameMode::RegisterCheckpoint(",1)[1].split(
            "\nvoid ALatebrakeRaceGameMode::",1)[0]
        self.assertEqual(finish.count("PracticeGateHistory.ObserveGate("),1)
        self.assertLess(finish.index("Race.PassCheckpoint(CheckpointIndex)"),
                        finish.index("PracticeGateHistory.ObserveGate("))
        self.assertIn("Race,CheckpointIndex);",finish)
        self.assertLess(finish.index("ValidateFinishTiming(Race)"),
                        finish.index("PracticeGateHistory.Finish(PracticeResult,Race)"))
        self.assertLess(finish.index("PracticeHistory.Finish("),
                        finish.index("PracticeGateHistory.Finish("))
        self.assertLess(finish.index("PracticeGateHistory.Finish("),
                        finish.index("ProfileWriteCheckpoint SavedProfile"))
        restart=mode.split("void ALatebrakeRaceGameMode::RestartRace()",1)[1].split(
            "void ALatebrakeRaceGameMode::RecoverPlayer(",1)[0]
        self.assertLess(restart.index("PracticeGateHistory.CancelAttempt();"),
                        restart.index("PracticeGateHistory.Begin("))
        self.assertLess(restart.index("RestartPlacement.OnRestartPlacementResult"),
                        restart.index("PracticeGateHistory.Begin("))
        self.assertIn("PracticeGateHistory.ObserveConfiguration(PracticeKey,PracticeActor)",mode)
        self.assertGreaterEqual(mode.count("PracticeGateHistory.Invalidate();"),3)

    def test_advisory_only_and_visible(self):
        policy=read("Source/Latebrake/Public/Racing/PracticeGateReference.h")
        for forbidden in ("SaveRecords(", "EarnCredits(", "PassCheckpoint(", "AddTimePenalty("):
            self.assertNotIn(forbidden,policy)
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for marker in ("SESSION REF L%d G%d", "NEW COMPLETE SESSION GATE REFERENCE",
                       "VS PRIOR GATE REFERENCE", "SECTORS FASTER", "FINAL DELTA"):
            self.assertIn(marker,hud)
        cmake=read("Tests/Standalone/CMakeLists.txt")
        self.assertIn("target_sources(latebrake_research_PracticeGateReference PRIVATE",cmake)


if __name__=="__main__":
    unittest.main()
