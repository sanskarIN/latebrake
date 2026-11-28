"""Guided practice experiments stage a pending setup, not a race or saved record."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class PracticeExperimentPlanBindingTests(unittest.TestCase):
    def test_plan_uses_actual_pending_mode_handling_and_history(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        plan=mode.split("ALatebrakeRaceGameMode::GetPendingPracticeExperimentPlan() const",1)[1].split(
            "bool ALatebrakeRaceGameMode::HandlePracticeTrialInput",1)[0]
        for marker in ("GetPracticeTrialAccess().AllowsEdit()", "Car->GetHandlingMode()",
                       "QuickPracticeSelection", "GetPendingPracticeTrial().Key",
                       "PracticeAnalysis.ReadPlan(PracticeHistory,Key)"):
            self.assertIn(marker,plan)
        handler=mode.split("bool ALatebrakeRaceGameMode::HandlePracticeTrialInput",1)[1].split(
            "void ALatebrakeRaceGameMode::MenuNext",1)[0]
        for marker in ("PracticeTrialAction::NextTest","PracticeTrialAction::RetestBest",
                       "GetPendingPracticeExperimentChoices()", "PracticeTrial.Stage(GetPracticeTrialAccess()",
                       "BumpQuickChoiceRevision();"):
            self.assertIn(marker,handler)
        for token in ("RestartRace(", "StartQuickPractice(", "SaveRecords(", "ApplyGarageToPlayer(",
                      "Garage.Select(", "Garage.Unlock(", "Garage.Upgrade("):
            self.assertNotIn(token,handler)
    def test_desktop_captions_and_current_targets_are_bound(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeQuickRaceHUD.cpp")
        self.assertEqual(hud.count("Mode->GetPendingPracticeExperimentChoices()"),2)
        self.assertIn("NEXT TEST SETUP",hud)
        self.assertIn("RETEST MEASURED BEST",hud)
        panel=read("Source/Latebrake/Public/UI/PracticeTrialPanel.h")
        self.assertIn("PracticeExperimentChoices Experiments",panel)
        self.assertIn("shown.Snapshot==current",panel)
        self.assertIn("snapshot.Experiments.NextValid",panel)
        self.assertIn("snapshot.Experiments.BestValid",panel)
        main=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("SUGGESTIONS ARE TEST COVERAGE, NOT PREDICTED SPEED",main)
        self.assertIn("NEXT TEST STAGES SETUP; SELECT STARTS",main)
        self.assertIn("add_latebrake_portable_research_test(PracticeExperimentPlan)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
