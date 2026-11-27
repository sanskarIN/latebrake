"""Source binding for live priority HUD; not installed-engine visual testing."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class HudTelemetryPriorityBindingTests(unittest.TestCase):
    def test_collect_before_render(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        live = hud.split("const auto Line = [&]", 1)[1]
        collect, render = live.split("const auto SelectedTelemetry=", 1)
        self.assertIn("TelemetryQueue.Add(Index,Kind)", collect)
        self.assertNotIn("DrawnTelemetryLines >=", collect)
        for marker in ("HudTelemetryKind::Blocking", "HudTelemetryKind::Clock",
                       "HudTelemetryKind::Progress", "HudTelemetryKind::Speed",
                       "HudTelemetryKind::Urgent", "HudTelemetryKind::Navigation",
                       "HudTelemetryKind::Start"):
            self.assertIn(marker, collect)
        for marker in ("Mode->GetHudTelemetryMode()", "DrawSelectedHudTelemetry(",
                       "FitHudText(Content, Font, TextScale)", "++DrawnTelemetryLines",
                       "DrawnTelemetryLines >= VerticalPlan.MaxTelemetryLines"):
            self.assertIn(marker, render)

    def test_status_baselines_and_settings(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for marker in ("VerticalPlan.SaveWarningY", "VerticalPlan.RaceStatusY",
                       "VerticalPlan.FooterY", "HUD DETAIL: %s (SESSION ONLY)",
                       "F2: cycle Focused / Balanced / Detailed"):
            self.assertIn(marker, hud)

    def test_only_local_settings_change_density(self):
        mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        block = mode.split("void ALatebrakeRaceGameMode::CycleHudTelemetryMode()", 1)[1].split(
            "void ALatebrakeRaceGameMode::MenuNext()", 1)[0]
        for marker in ("CanCycleHudTelemetryMode(Menu.IsOpen()", "MenuPage::Settings",
                       "HasLocalPlayerDriver()", "AppFocusGuard.IsSuspended()",
                       "NextHudTelemetryMode(HudTelemetryMode)"):
            self.assertIn(marker, block)
        for forbidden in ("SaveRecords(", "RestartRace(", "Race.Advance("):
            self.assertNotIn(forbidden, block)
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.assertIn('BindAction(TEXT("CycleHudTelemetry")', pawn)
        self.assertIn("CycleHudTelemetryMode).bExecuteWhenPaused=true", pawn)
        self.assertIn('ActionName="CycleHudTelemetry",Key=F2',read("Config/DefaultInput.ini"))
        self.assertIn("add_latebrake_portable_research_test(HudTelemetryPriority)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
