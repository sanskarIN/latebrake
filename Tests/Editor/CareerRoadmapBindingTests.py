"""Career roadmap is a read-only menu projection of authoritative profile state."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class CareerRoadmapBindingTests(unittest.TestCase):
    def test_native_career_pages_use_actual_snapshot(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        block = hud.split("const auto DrawCareerRoadmapDetails=[&]()", 1)[1].split(
            "switch (Mode->GetMenuPage())", 1)[0]
        for marker in ("Mode->GetCareer().Snapshot()", "Mode->GetGarageSnapshot()",
                       "Mode->GetMenuCareerEventId()", "Target.NextMedalTarget",
                       "Target.RequiredUnlockCost", "Plan.MinimumAdditionalFunding",
                       "CONDITIONAL PREREQUISITE ROUTE (NOT AWARDED)"):
            self.assertIn(marker, block)
        for forbidden in ("CommitFinish(", "StartCareerEvent(", "SaveRecords(", "Unlock("):
            self.assertNotIn(forbidden, block)
        for page in ("Career", "Championships", "Challenges"):
            section = hud.split(f"case latebrake::MenuPage::{page}:", 1)[1].split("break;", 1)[0]
            self.assertIn("DrawCareerRoadmapDetails();", section)

    def test_planner_uses_local_sanitized_copies(self):
        policy = read("Source/Latebrake/Public/Career/CareerRoadmap.h")
        for marker in ("progress.Load(saved)", "garage.Load(garageSaved)",
                       "PlanGaragePurchasePath(garage.Snapshot()", "RoadmapChampionshipComplete",
                       "garage.EarnCredits(reward)", "step.AdditionalFunding=plan.CreditsShort"):
            self.assertIn(marker, policy)
        self.assertNotIn("CommitFinish(", policy)
        self.assertIn("add_latebrake_portable_research_test(CareerRoadmap)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
