"""Prove the Windows 2D trial uses one source-backed, overflow-safe
mouse policy and cannot be presented as a validated Unreal build.

These are static source contracts, not native Windows UI or UE runtime QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PREVIEW = ROOT / "Experiments/Win32Preview/LateBrakePreview.cpp"
MATH = ROOT / "Experiments/Win32Preview/PreviewViewportMath.h"
CPP_TEST = ROOT / "Tests/Standalone/PreviewViewportMathTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
WORKFLOW = ROOT / ".github/workflows/experimental-win32-preview.yml"
GUIDE = ROOT / "Experiments/Win32Preview/TRIAL_QUICKSTART.md"


class Win32TrialArtifactBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.preview = PREVIEW.read_text(encoding="utf-8")
        cls.math = MATH.read_text(encoding="utf-8")
        cls.cpp_test = CPP_TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.workflow = WORKFLOW.read_text(encoding="utf-8")
        cls.guide = GUIDE.read_text(encoding="utf-8")

    def test_real_pointer_path_calls_shared_64_bit_policy(self):
        self.assertIn('#include "PreviewViewportMath.h"', self.preview)
        self.assertIn("using latebrake_preview::CanvasViewport;", self.preview)
        self.assertIn("latebrake_preview::FitPreviewViewport(", self.preview)
        self.assertIn("latebrake_preview::TranslatePreviewMouse(", self.preview)
        self.assertIn("CanvasViewport vp=fitViewport(cw,ch);", self.preview)
        self.assertIn("translateMouse(vp,px,py,mouseX,mouseY)", self.preview)
        self.assertIn("mouseClick(mouseX,mouseY)", self.preview)
        self.assertNotIn("clientX>=vp.x+vp.w", self.preview)
        self.assertNotIn("(clientX-vp.x)*WIDTH", self.preview)

    def test_invalid_input_cannot_overflow_or_activate_canvas(self):
        for marker in (
            "static_cast<long long>(clientX) - viewport.x",
            "static_cast<long long>(clientY) - viewport.y",
            "relativeX >= viewport.w",
            "relativeY >= viewport.h",
            "relativeX * PreviewCanvasWidth / viewport.w",
            "relativeY * PreviewCanvasHeight / viewport.h",
            "viewport.x < 0 || viewport.y < 0",
            "return false;",
        ):
            self.assertIn(marker, self.math)
        self.assertLess(self.math.index("relativeX >= viewport.w"),
                        self.math.index("outX = static_cast<int>(canvasX)"))
        self.assertLess(self.math.index("relativeY >= viewport.h"),
                        self.math.index("outY = static_cast<int>(canvasY)"))
        for forbidden in ("CreateWindow", "AddForce(", "PassCheckpoint(",
                          "LoadLibrary", "SetActorLocation("):
            self.assertNotIn(forbidden, self.math)

    def test_real_headless_win32_selftest_checks_large_client(self):
        code = self.preview.split("static bool selftest()", 1)[1]
        for marker in (
            "fitViewport(1920,1080)",
            "translateMouse(vp,80,500",
            "fitViewport(2147483647,2147483647)",
            "mappedX!=WIDTH-1||mappedY!=HEIGHT-1",
            "lastX!=431||lastY!=219",
        ):
            self.assertIn(marker, code)

    def test_100k_portable_bounds_and_real_cmake_registration(self):
        self.assertIn("for (int i = 0; i < 100000; ++i)",
                      self.cpp_test)
        self.assertIn("const CanvasViewport atMaximum{INT_MAX", self.cpp_test)
        self.assertIn("TestBadDimensions();", self.cpp_test)
        self.assertIn("TestMapping(size[0], size[1]);", self.cpp_test)
        self.assertIn("PreviewViewportMathTests.cpp", self.cmake)
        self.assertIn("NAME PreviewViewportMath", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)

    def test_unsigned_unreal_distinction_and_binary_receipt(self):
        for marker in (
            "on:",
            "pull_request:",
            "Experiments/Win32Preview/**",
            "Build-Windows.ps1",
            "LateBrakePreviewV5.exe",
            "LateBrakeSetupV5.exe",
            "VerifyPayload.py",
            "TRIAL_BUILD_SHA256.txt",
            "TRIAL_QUICKSTART.md",
            "Get-FileHash -LiteralPath $game -Algorithm SHA256",
            "Get-FileHash -LiteralPath $setup -Algorithm SHA256",
            "retention-days: 14",
            "NOT Unreal",
        ):
            self.assertIn(marker, self.workflow)
        self.assertIn("NOT a trial", self.guide)
        self.assertIn("Windows 10/11", self.guide)
        self.assertIn("not a release", self.guide.lower())
        self.assertIn("Unreal Engine 5.8", self.guide)


if __name__ == "__main__":
    unittest.main()
