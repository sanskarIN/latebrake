"""GameMode and HUD share revision-bound practice analysis, not per-frame scans."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class PracticeSetupAnalysisCacheBindingTests(unittest.TestCase):
    def test_revision_changes_only_on_finish_or_reset(self):
        history=read("Source/Latebrake/Public/Racing/PracticeSessionHistory.h")
        self.assertIn("std::uint64_t AnalysisRevision() const noexcept",history)
        reset=history.split("void ResetSession()",1)[1].split("bool Begin(",1)[0]
        self.assertIn("BumpRevision();",reset)
        finish=history.split("PracticeComparison Finish(",1)[1].split("PracticeComparison LastComparison()",1)[0]
        self.assertLess(finish.index(".Observe(sample)"),finish.index("BumpRevision();"))
        self.assertLess(finish.index("Last.Record=record;"),finish.index("BumpRevision();"))
        cancel=history.split("void CancelAttempt()",1)[1].split("void ResetSession()",1)[0]
        self.assertNotIn("BumpRevision(",cancel)
    def test_cache_shared_by_results_and_pending_planner(self):
        header=read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        self.assertIn("mutable latebrake::PracticeSetupAnalysisCache PracticeAnalysis;",header)
        self.assertIn("PracticeAnalysis.ReadCatalogue(PracticeHistory,Key)",header)
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        self.assertIn("PracticeAnalysis.ReadPlan(PracticeHistory,Key)",mode)
        cache=read("Source/Latebrake/Public/Racing/PracticeSetupAnalysisCache.h")
        for token in ("Source==&history","history.AnalysisRevision()",
                      "Catalogue.At(selected.Tuning.Selected","if(!PlanReady)"):
            self.assertIn(token,cache)
        self.assertIn("add_latebrake_portable_research_test(PracticeSetupAnalysisCache)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
