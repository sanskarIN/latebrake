"""Garage performance card contracts, not native rendering verification."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class GarageComparisonPanelBindingTests(unittest.TestCase):
    def setUp(self):
        self.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.renderer = read("Source/Latebrake/Private/UI/LatebrakeGarageComparisonHUD.cpp")
        self.policy = read("Source/Latebrake/Public/UI/GarageComparisonPanel.h")

    def test_uses_actual_garage_snapshot_and_preview(self):
        start = self.hud.index("const auto ComparisonLayout=")
        end = self.hud.index("Y=static_cast<float>(ComparisonLayout.NextDetailY)", start)
        block = self.hud[start:end]
        self.assertIn("GarageCatalogue.Read(Mode->GetGarageSnapshot())", block)
        self.assertIn("Mode->GetGaragePreviewVehicleId()", block)
        self.assertIn("CompareGaragePerformance(catalogue,viewedId)", self.policy)
        self.assertIn("equipped.Ownership!=GarageCatalogOwnership::Equipped", self.policy)

    def test_purchase_feedback_and_pagination_preserved(self):
        start = self.hud.index("const auto ComparisonLayout=")
        pinned = self.hud.rfind("DrawPinned(Mode->GetGaragePurchaseDisplayFeedback(), true);", 0, start)
        self.assertGreaterEqual(pinned, 0)
        self.assertLess(start, self.hud.index("MenuPager.Synchronize(", start))
        self.assertIn("DrawnMenuRows+=ComparisonLayout.ReservedRows", self.hud)
        self.assertIn("MenuPlan.MaxDetailsRows -\n                DrawnMenuRows", self.hud)
        self.assertIn("vertical.OverflowNoticeY-1.25*step", self.policy)
        self.assertIn("GarageComparisonMinimumDetailRows", self.policy)

    def test_protected_left_lane_and_touch_fallback(self):
        self.assertIn("Layout,MenuPlan,ReadablePanel,SafeTextWidth,Y,DrawnMenuRows,", self.hud)
        self.assertIn("true,bTouchHUD", self.hud)
        self.assertIn("!garage || touch || hud.Compact", self.policy)
        self.assertIn("HudRadarContains(owner,out.Panel)", self.policy)
        self.assertIn("HudRadarOverlaps(parts[i],parts[j])", self.policy)

    def test_measurement_before_any_draw(self):
        self.assertIn("GetTextSize(Text,W,H,Font,FontScale)", self.renderer)
        self.assertIn("VIEWED #%d vs EQUIPPED #%d", self.renderer)
        self.assertIn("CONFIGURED EST.", self.renderer)
        self.assertIn("GarageComparisonOwnershipLabel(Readout.ViewedOwnership)", self.renderer)
        self.assertIn("TEXT(\"%+.1f%%\")", self.renderer)
        self.assertLess(self.renderer.index("CanDrawGarageComparisonPanel(Layout,Sizes)"),
                        self.renderer.index("Rect(Layout.Panel,Background)"))
        self.assertLess(self.renderer.index("if(!Bars[I].Valid)return false;"),
                        self.renderer.index("Rect(Layout.Panel,Background)"))
        self.assertIn("LOCKED PREVIEW", self.policy)

    def test_no_authority_or_interaction(self):
        for forbidden in (
            "TryPurchase(", "EarnCredits(", "SaveRecords(", "SetAutonomousControls(",
            "SelectVehicle(", "SetGaragePreview", "HitTest", "RegisterCheckpoint(",
        ):
            self.assertNotIn(forbidden, self.renderer)
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("add_latebrake_portable_research_test(GarageComparisonPanel)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
