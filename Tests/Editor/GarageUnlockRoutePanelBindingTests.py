"""Sequential unlock route integration: source contract, not Unreal screenshot QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageUnlockRoutePanelBindingTests(unittest.TestCase):
    def test_only_observes_existing_economy_and_planner(self):
        policy = read("Source/Latebrake/Public/UI/GarageUnlockRoutePanel.h")
        for marker in (
            "garage.Load(persisted)", "PlanGaragePurchasePath(current,",
            "action.Target!=viewed", "garage.NextLockedVehicle()",
            "action.RequiresPurchaseConfirmation", "out.Shortfall!=path.CreditsShort",
            "std::min(data.Credits,data.TotalCost)",
        ):
            self.assertIn(marker, policy)
        for forbidden in ("garage.Unlock(", "garage.Select(", "SaveRecords(", "EarnCredits("):
            self.assertNotIn(forbidden, policy)

    def test_existing_purchase_feedback_and_compact_fallback(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        start = hud.index("const auto& UnlockRoute=UnlockRouteCache.Read(")
        self.assertGreaterEqual(hud.rfind("DrawPinned(Mode->GetGaragePurchaseDisplayFeedback(), true);",
                                         0, start), 0)
        self.assertIn("UnlockRoute.Count,true,bTouchHUD", hud)
        self.assertIn("DrawnMenuRows+=UnlockLayout.ReservedRows", hud)
        self.assertIn("Y=static_cast<float>(UnlockLayout.NextDetailY)", hud)
        self.assertIn("const auto& ViewedPath=UnlockRouteCache.Read(Mode->GetGarageSnapshot(),PreviewId)", hud)
        self.assertIn("Step.Cost,Step.CumulativeCost,Step.CumulativeShortfall", hud)
        self.assertIn("Q BUYS ONE NEXT CAR AT A TIME; TOTAL IS A PREVIEW.", hud)
        self.assertIn("latebrake::GarageUnlockRouteCache UnlockRouteCache;",
                      read("Source/Latebrake/Public/UI/LatebrakeRaceHUD.h"))

    def test_native_font_measurement_and_bounded_read_only_panel(self):
        renderer = read("Source/Latebrake/Private/UI/LatebrakeGarageUnlockRouteHUD.cpp")
        self.assertIn("GetTextSize(Text,W,H,Font,Scale)", renderer)
        self.assertIn("BuildGarageUnlockRouteBar(Layout.Track,Data)", renderer)
        self.assertIn("Rect(Bar.Funded,Mint)", renderer)
        self.assertLess(renderer.index("CanDrawGarageUnlockRoute(Layout,Sizes)"),
                        renderer.index("Rect(Layout.Panel,Panel)"))
        for forbidden in ("SaveRecords(", "GarageUnlockNext(", "HitTest", "GarageSelectVehicle("):
            self.assertNotIn(forbidden, renderer)
        self.assertIn("add_latebrake_portable_research_test(GarageUnlockRoutePanel)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
