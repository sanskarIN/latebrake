"""Pre-launch summary reads exact pending setup and comparable physical evidence."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class PracticeLaunchPreviewBindingTests(unittest.TestCase):
    def test_pending_preview_reads_both_observation_stores(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        block=mode.split("ALatebrakeRaceGameMode::GetPendingPracticeLaunchPreview() const",1)[1].split(
            "bool ALatebrakeRaceGameMode::HandlePracticeTrialInput",1)[0]
        for token in ("GetPendingPracticeExperimentPlan()", "Plan.Selected",
                      "GetPendingPracticeTrial()", "PracticeHistory,PracticeGateHistory"):
            self.assertIn(token,block)
        for forbidden in ("RestartRace(", "SaveRecords(", "ApplyGarageToPlayer("):
            self.assertNotIn(forbidden,block)
    def test_gate_reference_is_not_claimed_as_best_finish(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for token in ("Mode->GetPendingPracticeLaunchPreview()",
                      "NO COMPARABLE FINISH FOR THIS EXACT SETUP",
                      "COMPLETE GATE REFERENCE", "GATE REFERENCE +%.2fs VS SESSION BEST",
                      "NO COMPLETE GATE REFERENCE YET"):
            self.assertIn(token,hud)
        policy=read("Source/Latebrake/Public/Racing/PracticeLaunchPreview.h")
        self.assertIn("reference+PracticeGateClockTolerance>=record.BestFinishSeconds",policy)
        self.assertIn("std::max(0.0,reference-record.BestFinishSeconds)",policy)
        self.assertIn("add_latebrake_portable_research_test(PracticeLaunchPreview)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
