"""v0.5.67 horizontal HUD source-level integration checks.

Font width measurement is performed by Unreal AHUD::GetTextSize in the
real UI, with a portable decision routine. This is NOT a UE compilation,
DPI render, phone screenshot, safe area or installed-game acceptance test.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
BOUNDS = ROOT / "Source/Latebrake/Public/UI/HudTextBounds.h"
TEST = ROOT / "Tests/Standalone/HudTextBoundsTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class HudTextBoundsBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.bounds = BOUNDS.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_actual_engine_font_measurement_not_character_guess(self):
        for marker in (
            '#include "UI/HudTextBounds.h"',
            "latebrake::CalculateHudTextBounds(",
            "const auto FitHudText =",
            "GetTextSize(Text, MeasuredWidth, MeasuredHeight, Font, TextScale);",
            "latebrake::ChooseHudTextPrefix(",
            "Measure(Value), Measure(Suffix)",
            "Measure(Value.Left(Units) + Suffix)",
            "HorizontalBounds.MaxWidthPixels",
            "GetMediumFont()", "GetSmallFont()",
        ):
            self.assertIn(marker, self.hud)
        self.assertLess(self.hud.index("CalculateHudTextBounds("),
                        self.hud.index("if (Mode->IsMenuOpen())"))
        self.assertNotIn("Text.Len() * 8", self.hud)
        self.assertNotIn("Value.Left(32)", self.hud)
        self.assertIn("Previous >= 0xD800u", self.hud)
        self.assertIn("Following >= 0xDC00u", self.hud)

    def test_real_menu_race_and_status_are_all_measured(self):
        for marker in (
            "const FString Fitted = FitHudText(\n                Value, GEngine->GetSmallFont(), UIScale);",
            "const FString Fitted = FitHudText(Content, Font, TextScale);",
            "const FString SaveMessage = FitHudText(",
            "const FString PauseMessage = FitHudText(",
            "const FString FinishMessage = FitHudText(",
            "const FString FooterMessage = FitHudText(",
            "SAVE FAILED - PROGRESS NOT STORED",
            "PAUSED - TAP PAUSE",
            "FINISHED - RESTART",
            "INVALID FINISH",
            'TEXT("L%d/%d G%d/%d")',
        ):
            self.assertIn(marker, self.hud)
        self.assertEqual(self.hud.count("const auto FitHudText ="), 1)
        self.assertLess(self.hud.index("const auto FitHudText ="),
                        self.hud.index("const auto MenuLine ="))
        self.assertIn("DrawnTelemetryLines >= VerticalPlan.MaxTelemetryLines", self.hud)

    def test_genuine_touch_layout_used_with_non_authoritative_bounds(self):
        for marker in (
            '#include "UI/TouchControlLayout.h"',
            "CalculateTouchControlLayout(width, height)",
            "pads.CareerNext.X", "pads.MenuBrowse.X",
            "pads.Pause.X", "std::isfinite(width)",
            "std::min({pads.CareerNext.X",
            "ChooseHudTextPrefix(", "while (low < high)",
        ):
            self.assertIn(marker, self.bounds)
        for forbidden in (
            "PassCheckpoint(", "AddTimePenalty(", "TryRecoverTo(",
            "SetActorLocation(", "SetAutonomousControls(",
            "SaveRecords(", "HitTestTouchControl(",
        ):
            self.assertNotIn(forbidden, self.bounds)

    def test_real_policy_compiled_and_dpi_regressions_registered(self):
        for marker in (
            "HudTextBoundsTests.cpp", "NAME HudTextBounds",
            "cxx_std_20", "/W4 /WX", "-Wpedantic -Werror",
        ):
            self.assertIn(marker, self.cmake)
        for marker in (
            "240.0, 240.0", "360.0, 640.0", "640.0, 360.0",
            "1920.0, 1080.0", "1'000'000",
            "pads.CareerNext.X", "quiet_NaN()",
        ):
            self.assertIn(marker, self.test)


if __name__ == "__main__":
    unittest.main()
