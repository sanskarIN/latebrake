"""Race overview integration contracts, not installed-engine render tests."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class HudRaceStatusPanelBindingTests(unittest.TestCase):
    def setUp(self):
        self.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.renderer = read("Source/Latebrake/Private/UI/LatebrakeRaceStatusHUD.cpp")
        self.policy = read("Source/Latebrake/Public/UI/HudRaceStatusPanel.h")

    def test_authoritative_clock_progress_and_speed_only(self):
        begin = self.hud.index("const auto RaceStatus=latebrake::ReadHudRaceStatus(")
        end = self.hud.index("const bool bRaceStatusDrawn=", begin)
        source = self.hud[begin:end]
        for value in (
            "Race.GetCompletedLaps()", "Race.GetRequiredLaps()",
            "Race.GetNextCheckpoint()", "Race.GetCheckpointCount()",
            "Race.GetElapsedSeconds()", "Car->GetSpeedKmH()",
            "Mode->UsesMilesPerHour()", "Race.IsFinished()",
            "bRaceStatusRunning", "Car->IsRacePhysicsReady()",
        ):
            self.assertIn(value, source)
        self.assertIn("CalculateRaceGateProgressGauge(source.CompletedLaps", self.policy)

    def test_pause_focus_ownership_and_track_readiness(self):
        begin = self.hud.index("const bool bRaceStatusRunning=")
        end = self.hud.index("const auto RaceStatusLayout=", begin)
        context = self.hud[begin:end]
        for marker in (
            "Mode->IsCircuitReadyForGameplay()", "Mode->IsRestartPlacementReady()",
            "Mode->HasLocalPlayerDriver()", "Mode->IsRaceStartGreen()",
            "!Mode->IsFocusLossGuardActive()", "!Mode->IsManualRacePauseActive()",
            "!UGameplayStatics::IsGamePaused(this)", "Mode->IsExperimentalFleetDriveReady()",
        ):
            self.assertIn(marker, context)
        self.assertIn("bTouchHUD,Mode->IsMenuOpen(),Race.IsFinished(),bRaceStatusRunning",
                      self.hud)
        self.assertIn("touch || menuOpen || raceFinished || !running || hud.Compact",
                      self.policy)

    def test_protected_layout_and_remaining_telemetry_budget(self):
        self.assertIn("Layout,VerticalPlan,HorizontalBounds,SafeTextWidth,", self.hud)
        self.assertIn("RaceStatusLayout.RemainingRows:VerticalPlan.MaxTelemetryLines",
                      self.hud)
        self.assertIn("Y=static_cast<float>(RaceStatusLayout.TelemetryTop)", self.hud)
        self.assertIn("Mode->GetHudTelemetryMode(),TelemetryRowLimit", self.hud)
        self.assertIn("DrawnTelemetryLines >= TelemetryRowLimit", self.hud)
        self.assertIn("vertical.SaveWarningY-1.25*hud.LineHeight", self.policy)
        for baseline in ("VerticalPlan.SaveWarningY", "VerticalPlan.RaceStatusY",
                         "VerticalPlan.FooterY"):
            self.assertIn(baseline, self.hud)

    def test_fallback_keeps_every_essential_number(self):
        start = self.hud.index("const FString NitroLabel =")
        end = self.hud.index("// Optional, clearly ESTIMATED automatic gearbox", start)
        block = self.hud[start:end]
        self.assertEqual(block.count("if (!bRaceStatusDrawn)"), 2)
        for marker in (
            "HudTelemetryKind::Speed", "HudTelemetryKind::Clock",
            "HudTelemetryKind::Progress", 'TEXT("L%d/%d G%d/%d")',
            'TEXT("HANDBRAKE HELD")', 'TEXT("NITRO %s %.0f%%")',
        ):
            self.assertIn(marker, block)
        self.assertIn("const bool bRaceStatusDrawn=DrawRaceStatusPanel", self.hud)

    def test_measure_all_text_before_any_native_draw(self):
        for marker in (
            "GEngine->GetSmallFont()", "GEngine->GetMediumFont()",
            "GetTextSize(Text,W,H,Font,Scale)", "CanDrawHudRaceStatus(Layout,Sizes)",
            "BuildHudRaceStatusProgress(Layout,Status.Progress)", "if(!Bar.Valid)return false;",
        ):
            self.assertIn(marker, self.renderer)
        self.assertLess(self.renderer.index("CanDrawHudRaceStatus(Layout,Sizes)"),
                        self.renderer.index("Rect(Layout.Panel,Panel)"))
        self.assertLess(self.renderer.index("if(!Bar.Valid)return false;"),
                        self.renderer.index("Rect(Layout.Panel,Panel)"))
        self.assertNotIn("ChooseHudTextPrefix", self.renderer)
        self.assertNotIn("...", self.renderer)
        self.assertIn("SPEED / MPH", self.renderer)
        self.assertIn("SPEED / KM/H", self.renderer)

    def test_presentation_never_owns_gameplay_or_pointer_input(self):
        for forbidden in (
            "SetAutonomousControls(", "AddTimePenalty(", "RegisterCheckpoint(",
            "SetActorLocation(", "SaveRecords(", "RestartRace(", "HitTest",
        ):
            self.assertNotIn(forbidden, self.renderer)
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("add_latebrake_portable_research_test(HudRaceStatusPanel)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
