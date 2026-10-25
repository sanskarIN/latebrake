"""v0.5.66 real Unreal HUD vertical overflow source contract.

These checks inspect the actual Unreal HUD and strict portable policy.
They DO NOT compile UHT/UE 5.8 or inspect pixel-accurate phone screenshots.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
PLAN = ROOT / "Source/Latebrake/Public/UI/HudVerticalPlan.h"
TEST = ROOT / "Tests/Standalone/HudVerticalPlanTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class HudVerticalPlanBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.plan = PLAN.read_text(encoding="utf-8")
        cls.cpp_test = TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_active_unreal_hud_uses_production_layout_bounds(self):
        self.assertIn('#include "UI/HudVerticalPlan.h"', self.hud)
        self.assertIn("latebrake::CalculateHudVerticalPlan(", self.hud)
        self.assertIn("bTouchHUD = true;", self.hud)
        self.assertIn("DrawnTelemetryLines >= VerticalPlan.MaxTelemetryLines",
                      self.hud)
        self.assertIn("++DrawnTelemetryLines;", self.hud)
        self.assertIn("Content.IsEmpty()", self.hud)
        self.assertEqual(self.hud.count("const auto Line = [&]"), 1)
        self.assertLess(self.hud.index("CalculateHudVerticalPlan("),
                        self.hud.index("const auto Line = [&]"))
        self.assertNotIn("const float FooterY = FMath::Max(Y + 3.f", self.hud)
        self.assertIn("VerticalPlan.FooterY", self.hud)

    def test_essential_lap_and_clock_precede_optional_telemetry(self):
        hud = self.hud
        for marker in ("LAP  %d / %d  |  GATE  %d / %d",
                       "TIME  %02d:%05.2f",
                       "NEXT GATE %.0fm | %s", "DRIVE DATA",
                       "RivalRelativeMotionHint.Update("):
            self.assertIn(marker, hud)
        self.assertEqual(hud.count('TEXT("TIME  %02d:%05.2f")'), 1)
        self.assertEqual(hud.count('TEXT("LAP  %d / %d  |  GATE  %d / %d")'), 1)
        self.assertLess(hud.index("LAP  %d / %d"),
                        hud.index("RivalRelativeMotionHint.Update("))
        self.assertLess(hud.index("TIME  %02d:%05.2f"),
                        hud.index("RivalProximityHint.Update("))
        self.assertLess(hud.index("TIME  %02d:%05.2f"),
                        hud.index("DrivingStats.Get") if
                        "DrivingStats.Get" in hud else hud.index("GetDrivingStats()"))

    def test_critical_status_drawn_without_ordinary_row_budget(self):
        hud = self.hud
        self.assertIn("VerticalPlan.SaveWarningY", hud)
        self.assertIn("VerticalPlan.RaceStatusY", hud)
        self.assertIn("WARNING: LOCAL PROGRESS COULD NOT BE SAVED", hud)
        self.assertIn("PAUSED  -  ESC TO RESUME", hud)
        self.assertIn("FINISHED! PRESS T TO RACE AGAIN", hud)
        self.assertIn("INVALID FINISH - PRESS T TO RETRY", hud)
        self.assertLess(hud.index("VerticalPlan.SaveWarningY"),
                        hud.index("VerticalPlan.RaceStatusY"))
        self.assertIn("latebrake::CanAwardFinishedRace(Race)", hud)
        self.assertNotIn('Line(TEXT("PAUSED  -  ESC TO RESUME")', hud)

    def test_touch_bounds_reuse_real_hit_region_and_are_non_authoritative(self):
        for marker in (
            '#include "UI/TouchControlLayout.h"',
            "CalculateTouchControlLayout(width, height)",
            "pads.Throttle.Y", "pads.Steering.Y",
            "MaxTelemetryLines", "layout.LineHeight",
            "outcomeY + step > pads.Throttle.Y"
        ):
            self.assertIn(marker, self.plan)
        for forbidden in (
            "PassCheckpoint(", "AddTimePenalty(", "TryRecoverTo(",
            "SetActorLocation(", "SetAutonomousControls(",
            "SaveRecords(",
        ):
            self.assertNotIn(forbidden, self.plan)

    def test_strict_production_layout_tests_registered(self):
        for marker in (
            "HudVerticalPlanTests.cpp", "NAME HudVerticalPlan",
            "cxx_std_20", "/W4 /WX", "-Wpedantic -Werror",
        ):
            self.assertIn(marker, self.cmake)
        for marker in (
            "240.0, 240.0", "640.0, 360.0", "1920.0, 1080.0",
            "360.0, 640.0", "pads.Throttle.Y", "MaxTelemetryLines",
            "quiet_NaN()", "RaceStatusY",
        ):
            self.assertIn(marker, self.cpp_test)


if __name__ == "__main__":
    unittest.main()
