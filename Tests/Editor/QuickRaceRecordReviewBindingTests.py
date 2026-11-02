"""Source-level binding for official Unreal finished Quick Race record recap."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
class QuickRaceRecordReviewBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode=(ROOT/"Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text()
        cls.hud=(ROOT/"Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text()
        cls.header=(ROOT/"Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h").read_text()
        cls.policy=(ROOT/"Source/Latebrake/Public/Racing/QuickRaceRecordReview.h").read_text()
    def test_snapshot_before_finish_record_write(self):
        self.assertIn("LoadRecords();\n    AttemptPreviousBestSeconds = GetEventBestSeconds();",self.mode)
        restart=self.mode.split("void ALatebrakeRaceGameMode::RestartRace()",1)[1].split(
            "void ALatebrakeRaceGameMode::RecoverPlayer(",1)[0]
        self.assertLess(restart.index("Race.Reset();"),
                        restart.index("AttemptPreviousBestSeconds = GetEventBestSeconds();"))
        finish=self.mode.split("void ALatebrakeRaceGameMode::RegisterCheckpoint(",1)[1].split(
            "void ALatebrakeRaceGameMode::SyncDesktopMenuPointerMode(",1)[0]
        self.assertNotIn("AttemptPreviousBestSeconds =",finish)
        self.assertIn("SaveRecords();",finish)
        self.assertIn("GetAttemptPreviousBestSeconds() const",self.header)
    def test_actual_results_hud_reads_persisted_record(self):
        part=self.hud.split("case latebrake::MenuPage::Results:",1)[1].split(
            "case latebrake::MenuPage::Pause:",1)[0]
        for token in ("ReviewQuickRaceRecord(","CanAwardFinishedRace(Race)",
                      "HasProgressSaveError()","GetAttemptPreviousBestSeconds()",
                      "GetEventBestSeconds()","Race.GetElapsedSeconds()",
                      "SAVE FAILED: best time and rewards NOT persisted",
                      "NEW SAVED PB","FIRST SAVED PB"):
            self.assertIn(token,part)
        self.assertIn('#include "Racing/QuickRaceRecordReview.h"',self.hud)
    def test_pure_policy_and_cmake(self):
        for forbidden in ("SaveRecords(", "ImprovedBestTime(", "EarnCredits(",
                          "PassCheckpoint(", "CommitFinish("):
            self.assertNotIn(forbidden,self.policy)
        cmake=(ROOT/"Tests/Standalone/CMakeLists.txt").read_text()
        self.assertIn("QuickRaceRecordReviewTests.cpp",cmake)
        self.assertIn("NAME QuickRaceRecordReview",cmake)
if __name__=="__main__":
    unittest.main()
