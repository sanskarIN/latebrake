"""Verify actual native menu drawing routes all button labels through measured fit."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class HudButtonCaptionFitBindingTests(unittest.TestCase):
    def test_live_menu_uses_actual_font_and_reserves_paint_swatch(self):
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")
        caption = hud.split("const auto DrawButtonCaption =",1)[1].split("const auto OrbitButtons",1)[0]
        for marker in ("GEngine->GetSmallFont()", "FitHudButtonCaption(Bounds, PreferredScale,",
                       "GetTextSize(bShort ? ShortCaption : Caption, TW, TH,",
                       "HudCaptionExtent{TW, TH}", "if (!Fit.Valid) return false;",
                       "Fit.X", "Fit.Y", "Fit.Scale"):
            self.assertIn(marker,caption)
        self.assertIn("DrawButtonCaption(Caption, FString::FromInt(Index+1),",hud)
        self.assertIn("DrawButtonCaption(Caption, FString(), Button.Bounds,",hud)
        self.assertIn("DrawButtonCaption(Caption, FString(), StyleButton.Bounds,",hud)
        self.assertIn("bPaint ? Swatch + 6.f * UIScale : 0.0",hud)
        self.assertIn("DrawButtonCaption(FString::FromInt(static_cast<int32>(i)+1),",hud)
        self.assertNotIn("DrawText(Caption, TextColor, BX +",hud)

    def test_geometry_policy_is_pure_and_registered(self):
        policy=(ROOT / "Source/Latebrake/Public/UI/HudButtonCaptionFit.h").read_text(encoding="utf-8")
        cmake=(ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("add_latebrake_portable_research_test(HudButtonCaptionFit)",cmake)
        for marker in ("right - horizontalInset - accessoryWidth", "extent.Width > availableWidth",
                       "extent.Height > availableHeight", "x + extent.Width <= textRight",
                       "y + extent.Height <= textBottom", "preferredScale * factor"):
            self.assertIn(marker,policy)
        for forbidden in ("SaveRecords(", "Garage.Select(", "SetGamePaused(", "DrawText(", "AddForce("):
            self.assertNotIn(forbidden,policy)

if __name__ == "__main__":
    unittest.main()
