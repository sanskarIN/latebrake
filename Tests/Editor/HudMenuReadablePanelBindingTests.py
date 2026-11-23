"""Source checks for real Unreal HUD Canvas high contrast menu backing.

These tests do not claim UE5.8 native compile, cooking or visual QA.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]

class HudMenuReadablePanelBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud=(ROOT/"Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")
        cls.policy=(ROOT/"Source/Latebrake/Public/UI/HudMenuReadablePanel.h").read_text(encoding="utf-8")
        cls.tests=(ROOT/"Tests/Standalone/HudMenuReadablePanelTests.cpp").read_text(encoding="utf-8")
        cls.cmake=(ROOT/"Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")

    def test_actual_menu_draws_before_wrapped_content(self):
        self.assertIn('#include "UI/HudMenuReadablePanel.h"',self.hud)
        section=self.hud.split("if (Mode->IsMenuOpen())",1)[1].split(
            "// Closing the menu must not retain",1)[0]
        self.assertIn("CalculateHudMenuReadablePanel(",section)
        self.assertIn("if (ReadablePanel.Valid)",section)
        self.assertIn("ReadablePanel.HeaderHeight",section)
        self.assertIn("DrawRect(Gold, BX, BY, 3.f * UIScale, BH);",section)
        self.assertLess(section.index("if (ReadablePanel.Valid)"),
                        section.index("const auto MenuLine ="))
        self.assertLess(section.index("if (ReadablePanel.Valid)"),
                        section.index("DrawPinned(FString::Printf("))

    def test_panel_cannot_own_clicks_or_cover_touch(self):
        for marker in ("Layout, MenuPlan, SafeTextWidth, true",
                       "vertical.SaveWarningY+22.0*s",
                       "layout.Left+safeTextWidth",
                       "std::isfinite(w)", "!menuOpen || !vertical.Valid"):
            self.assertIn(marker,self.hud+self.policy)
        for forbidden in ("HitTestDesktopMenuMouse(", "SaveRecords(",
                          "SetMenuPage(", "Garage.Unlock(", "AddForce("):
            self.assertNotIn(forbidden,self.policy)

    def test_stress_normal_and_compact_windows(self):
        self.assertIn("HudMenuReadablePanelTests.cpp",self.cmake)
        self.assertIn("NAME HudMenuReadablePanel",self.cmake)
        self.assertIn("100000",self.tests)
        self.assertIn("CheckView(size.first,size.second,true);",self.tests)
        self.assertIn("pad.Steering.Y",self.tests)

if __name__=="__main__":
    unittest.main()
