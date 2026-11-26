"""Source contract for non-authoritative desktop racing instruments.

Portable C++ tests cover geometry and bad input; this test only checks
the Unreal-facing presentation wiring, not UE rendering or installed QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(name):
    return (ROOT / name).read_text(encoding="utf-8")


class HudInstrumentClusterBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy = source("Source/Latebrake/Public/UI/HudInstrumentCluster.h")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")

    def test_runtime_reads_actual_telemetry_and_preserves_text_lane(self):
        self.assertIn('#include "UI/HudInstrumentCluster.h"', self.hud)
        for marker in (
            "CalculateHudInstrumentCluster(",
            "Mode->GetRace().IsFinished()",
            "Car->GetSpeedKmH()",
            "Car->GetNitroBoost().ChargeFraction()",
            "Car->GetEngineHeat().HeatFraction()",
            "Car->GetCollisionDamage().DamageFraction()",
            "Instruments.Panel.X - HorizontalBounds.Left",
            "Value.Len(), SafeTextWidth,",
            'DrawMeter(*NitroMeterName, Instruments.NitroFraction,',
            'latebrake::ReadNitroHud(Car->GetNitroBoost(),',
            'NitroHudPhase::Cooldown',
            'NitroHudPhase::Stored',
            'DrawMeter(TEXT("ENGINE HEAT (ARCADE)")',
            'DrawMeter(TEXT("CHASSIS CONDITION")',
        ):
            self.assertIn(marker, self.hud)
        self.assertLess(
            self.hud.index("const double SafeTextWidth"),
            self.hud.index("Value.Len(), SafeTextWidth,"))
        self.assertLess(
            self.hud.index("if (Mode->IsMenuOpen())"),
            self.hud.index("if (Instruments.Visible)\n    {"))

    def test_real_font_measured_percentage_never_competes_with_label(self):
        # Source-level Unreal binding, not a native editor screenshot.
        panel = self.hud.split(
            "    // Read-only instrument panel.", 1)[1].split(
            "    // A deterministic footer", 1)[0]
        for marker in (
            "GetTextSize(Percentage, PercentageWidth, PercentageHeight,",
            "CalculateHudInstrumentMeterText(",
            "Instruments.Panel, Instruments.Scale,",
            "MeterText.LabelMaxWidth",
            "latebrake::ChooseHudTextPrefix(",
            "MeterName.Left(Prefix) + Suffix",
            "DrawText(Percentage, Ink,",
            "MeterText.PercentX",
            "DrawText(VisibleName, Ink,",
            "MeterText.LabelX",
        ):
            self.assertIn(marker, panel)
        self.assertLess(
            panel.index("GetTextSize(Percentage,"),
            panel.index("CalculateHudInstrumentMeterText("))
        self.assertLess(
            panel.index("CalculateHudInstrumentMeterText("),
            panel.index("DrawText(Percentage, Ink,"))
        self.assertIn("std::isfinite(measuredPercentWidth)", self.policy)
        self.assertIn("labelWidth = percentX - gap - left", self.policy)
        self.assertIn("percentRight = panel.X + panel.Width - inset",
                      self.policy)

    def test_instruments_are_display_only(self):
        panel = self.hud.split(
            "    // Read-only instrument panel.", 1)[1].split(
            "    // A deterministic footer", 1)[0]
        for forbidden in ("RegisterCheckpoint", "AddForce(",
                          "SaveRecords(", "AddTimePenalty(",
                          "TrySpend", "SetAutonomousControls"):
            self.assertNotIn(forbidden, panel)
        for marker in ("touchUI || menuOpen || raceFinished",
                       "layout.Compact", "std::isfinite(speedKmH)",
                       "std::isfinite(engineHeatFraction)",
                       "std::clamp(", "x + panelWidth > width",
                       "y + panelHeight > height"):
            self.assertIn(marker, self.policy)

    def test_policy_registered_with_warnings_as_errors(self):
        self.assertIn("HudInstrumentClusterTests.cpp", self.cmake)
        self.assertIn("NAME HudInstrumentCluster", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)


if __name__ == "__main__":
    unittest.main()
