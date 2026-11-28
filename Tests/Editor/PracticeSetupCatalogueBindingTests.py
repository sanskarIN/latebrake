"""The setup catalogue reads session observations, never creates profile records."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class PracticeSetupCatalogueBindingTests(unittest.TestCase):
    def test_catalogue_uses_actual_same_group_finishes(self):
        p=read("Source/Latebrake/Public/Racing/PracticeSetupCatalogue.h")
        for token in ("history.Read(key)", "history.ReadSeries(key)",
                      "series.Finishes!=record.FinishedAttempts",
                      "selected.Preset,{car,engine,grip},selected.Handling",
                      "DiffersOnlyOnPracticeAxis", "ComparePracticeSetups"):
            self.assertIn(token,p)
        for forbidden in ("EarnCredits(", "Garage.Unlock(", "SaveRecords(", "race.Advance("):
            self.assertNotIn(forbidden,p)
    def test_results_use_configuration_of_actual_finish(self):
        mode=read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        self.assertIn("PracticeAnalysis.ReadCatalogue(PracticeHistory,Key)",mode)
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for token in ("GetPracticeSetupCatalogue(Comparison.Configuration)",
                      "OBSERVED SETUPS | SAME MODE & HANDLING | SESSION ONLY",
                      "RankPracticeSetups(Setups", "ReviewPracticeAxis(Setups",
                      "OBSERVED TIMES, NOT CONTROLLED PERFORMANCE CLAIMS"):
            self.assertIn(token,hud)
        self.assertIn("add_latebrake_portable_research_test(PracticeSetupCatalogue)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
