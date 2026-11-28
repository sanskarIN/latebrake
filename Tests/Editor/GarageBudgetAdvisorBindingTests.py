"""Source binding for cached, non-mutating garage budget advice."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageBudgetAdvisorBindingTests(unittest.TestCase):
    def test_only_garage_page_reads_cached_advice(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        page = hud.split("case latebrake::MenuPage::Garage:", 1)[1].split(
            "case latebrake::MenuPage::Results:", 1)[0]
        self.assertIn("BudgetAdvisor.Read(Mode->GetGarageSnapshot(),PreviewId)", page)
        self.assertEqual(hud.count("BudgetAdvisor.Read("), 1)
        self.assertIn("CONFIGURED MULTIPLIERS ONLY", page)
        self.assertIn("PLANNING ONLY: Q/U/I CONFIRM INDIVIDUAL PURCHASES", page)
        self.assertIn("Focus.NextImprovement", page)
        self.assertIn("BestViewedAffordable", page)
        self.assertIn("latebrake::GarageBudgetAdvisor BudgetAdvisor;",
                      read("Source/Latebrake/Public/UI/LatebrakeRaceHUD.h"))

    def test_advice_uses_authoritative_planner(self):
        header = read("Source/Latebrake/Public/Garage/GarageBudgetAdvisor.h")
        for marker in ("PlanGaragePurchasePath(out.Baseline", "GaragePlanScore(plan.Projected",
                       "SameGarageBudgetSnapshot", "Cached.ViewedVehicle!=viewedVehicle"):
            self.assertIn(marker, header)
        for forbidden in ("SaveGame", "simulation.Unlock(", "simulation.Upgrade(",
                          "UnlockVehicle(", "SelectGarageVehicle("):
            self.assertNotIn(forbidden, header)
        self.assertIn("add_latebrake_portable_research_test(GarageBudgetAdvisor)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
