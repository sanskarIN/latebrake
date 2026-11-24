"""Bind portable keyboard rearm tests to actual Win32 input dispatch.

These static checks are supplementary: only native Win64 execution validates
GetAsyncKeyState, WM_MOUSEACTIVATE and foreground-window behavior.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Experiments/Win32Preview/LateBrakePreview.cpp"
GUARD = ROOT / "Experiments/Win32Preview/PreviewKeyboardFocusGuard.h"
TEST = ROOT / "Tests/Standalone/PreviewKeyboardFocusGuardTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class Win32KeyboardFocusBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.code = SOURCE.read_text(encoding="utf-8")
        cls.guard = GUARD.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_foreground_only_key_query_is_guarded(self):
        self.assertIn('#include "PreviewKeyboardFocusGuard.h"', self.code)
        self.assertIn("keyboardFocus.Sample(", self.code)
        self.assertIn("hasWindowFocus,trackedKeyboardHeld()", self.code)
        self.assertIn("keyboardInputArmed && (GetAsyncKeyState(k)&0x8000)!=0",
                      self.code)
        self.assertLess(self.code.index("keyboardInputArmed=keyboardFocus.Sample("),
                        self.code.index("bool enter=pressed("))
        self.assertIn("keyboardFocus.OnFocusLost();", self.code)
        self.assertIn("keyboardInputArmed=false;", self.code)
        self.assertIn("if(!hasWindowFocus)return;", self.code)

    def test_only_relevant_keys_must_be_neutral(self):
        self.assertIn("static bool trackedKeyboardHeld()", self.code)
        segment = self.code.split("static bool trackedKeyboardHeld()", 1)[1].split(
            "static void step(", 1)[0]
        for key in ("'W'", "'A'", "'S'", "'D'", "'P'", "'K'",
                    "'E'", "'U'", "'H'", "'J'", "0x25", "0x26",
                    "0x27", "0x28", "0x0D", "0x1B", "0x10", "0x20",
                    "0x70"):
            self.assertIn(key, segment)
        self.assertIn("neutralSamples_", self.guard)
        self.assertIn("if (++neutralSamples_ < 2)", self.guard)
        self.assertIn("TestDeterministicStress();", self.test)
        self.assertIn("250000", self.test)
        self.assertIn("NAME PreviewKeyboardFocusGuard", self.cmake)

    def test_inactive_click_cannot_select_menu(self):
        code = self.code.split("static LRESULT __stdcall WindowProc(", 1)[1]
        self.assertIn("if(msg==0x0021", code)
        self.assertIn("return 2;", code)
        self.assertIn("if(msg==0x0201&&!hasWindowFocus)return 0;", code)
        self.assertLess(code.index("if(msg==0x0201&&!hasWindowFocus)"),
                        code.index("mouseClick(mouseX,mouseY)"))


if __name__ == "__main__":
    unittest.main()
