"""Practice trend summaries consume only accepted comparable race finishes."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class PracticeAttemptSeriesBindingTests(unittest.TestCase):
    def test_series_observations_share_finish_authority(self):
        history=read("Source/Latebrake/Public/Racing/PracticeSessionHistory.h")
        finish=history.split("PracticeComparison Finish(",1)[1]
        self.assertLess(finish.index("if(Changed)"),finish.index("MakePracticeAttemptSample(race)"))
        self.assertLess(finish.index("CanAwardFinishedRace(race)"),finish.index(".Observe(sample)"))
        self.assertLess(finish.index(".Observe(sample)"),finish.index("auto& record=Records"))
        self.assertIn("for(auto& series:Series)series.Reset();",history)
        self.assertIn("Series[static_cast<std::size_t>(index)].Read()",history)
    def test_results_disclose_comparable_observations_only(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for token in ("GetPracticeAttemptSeries(Comparison.Configuration)",
                      "Series.Finishes==Comparison.Record.FinishedAttempts",
                      "FEE-FREE FINISHES", "RECENT MATCHING ATTEMPTS", "MEAN ACTIVE"):
            self.assertIn(token,hud)
        self.assertIn("add_latebrake_portable_research_test(PracticeAttemptSeries)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
