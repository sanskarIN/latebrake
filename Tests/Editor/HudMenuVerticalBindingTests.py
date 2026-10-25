"""v0.5.68 menu vertical-overflow source safety tests.

No Unreal/UHT compilation, visual glyph rendering or Android device QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
PLAN = ROOT / "Source/Latebrake/Public/UI/HudMenuVerticalPlan.h"
TEST = ROOT / "Tests/Standalone/HudMenuVerticalPlanTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class HudMenuVerticalBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.plan = PLAN.read_text(encoding="utf-8")
        cls.cpp_test = TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_real_menu_uses_policy_before_drawing_all_pages(self):
        for marker in (
            '#include "UI/HudMenuVerticalPlan.h"',
            "latebrake::CalculateHudMenuVerticalPlan(",
            "DrawnMenuRows >= MenuPlan.MaxDetailsRows",
            "bMenuOverflow = true;",
            "++DrawnMenuRows;",
            "MenuPlan.OverflowNoticeY",
            "MORE MENU DETAILS HIDDEN",
            "case latebrake::MenuPage::Results:",
            "case latebrake::MenuPage::Garage:",
            "case latebrake::MenuPage::Career:",
        ):
            self.assertIn(marker, self.hud)
        self.assertLess(self.hud.index("CalculateHudMenuVerticalPlan("),
                        self.hud.index("case latebrake::MenuPage::Results:"))

    def test_save_failures_are_independent_of_menu_row_budget(self):
        for marker in (
            "MenuPlan.Valid && Mode->HasProgressSaveError()",
            "const FString SaveNotice = FitHudText(",
            "bTouchHUD ? TEXT(\"SAVE FAILED\")",
            "WARNING: PROGRESS NOT SAVED",
            "MenuPlan.SaveWarningY",
        ):
            self.assertIn(marker, self.hud)
        self.assertNotIn('MenuLine(TEXT("WARNING: PROGRESS NOT SAVED")',
                         self.hud)
        self.assertLess(self.hud.index("MORE MENU DETAILS HIDDEN"),
                        self.hud.index("MenuPlan.SaveWarningY"))
        self.assertIn("const FString Fitted = FitHudText(", self.hud)

    def test_production_geometry_preserves_steering_zone(self):
        for marker in (
            "CalculateTouchControlLayout(width, height).Steering.Y",
            "step = 28.0 * layout.Scale",
            "saveY + step > bottom",
            "std::clamp(maxRows, 0, 90)",
            "latestDetailY < layout.Top",
        ):
            self.assertIn(marker, self.plan)
        for forbidden in (
            "PassCheckpoint(", "AddTimePenalty(", "SaveRecords(",
            "SetActorLocation(", "SetAutonomousControls(", "HitTestTouchControl(",
        ):
            self.assertNotIn(forbidden, self.plan)

    def test_strict_three_os_layout_regression(self):
        for marker in (
            "HudMenuVerticalPlanTests.cpp", "NAME HudMenuVerticalPlan",
            "cxx_std_20", "/W4 /WX", "-Wpedantic -Werror",
        ):
            self.assertIn(marker, self.cmake)
        for marker in (
            "240.0, 240.0", "640.0, 360.0",
            "360.0, 640.0", "pads.Steering.Y",
        ):
            # In test, variable is named 'touch', not 'pads'.
            if marker == "pads.Steering.Y":
                marker = "touch.Steering.Y"
            self.assertIn(marker, self.cpp_test)


if __name__ == "__main__":
    unittest.main()
