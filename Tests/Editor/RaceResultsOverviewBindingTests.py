"""Native results card source bindings; does not verify installed-engine pixels."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RaceResultsOverviewBindingTests(unittest.TestCase):
    def setUp(self):
        self.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.renderer = read("Source/Latebrake/Private/UI/LatebrakeResultsOverviewHUD.cpp")
        self.policy = read("Source/Latebrake/Public/UI/RaceResultsOverview.h")

    def test_real_race_and_save_context(self):
        start = self.hud.index("const auto Overview=latebrake::ReadResultsOverview(Race,")
        end = self.hud.index("if (DrawResultsOverview", start)
        block = self.hud[start:end]
        for marker in (
            "Mode->GetSelectedEvent()", "Mode->IsCareerRace()", "Mode->IsPracticeRace()",
            "Mode->IsExperimentalFleetRequested()", "Mode->IsProfileReadOnly()",
            "Mode->HasProgressSaveError()", "Mode->GetAttemptPreviousBestSeconds()",
            "Mode->GetEventBestSeconds()",
        ):
            self.assertIn(marker, block)
        for marker in (
            "ValidateFinishTiming(race)", "race.GetElapsedSeconds()",
            "race.GetTotalPenaltySeconds()", "BuildFinishedRaceInsights(race)",
            "ReviewQuickRaceRecord(context.Event", "out.ActiveSeconds=elapsed-fee",
        ):
            self.assertIn(marker, self.policy)

    def test_layout_keeps_pager_and_existing_right_panels(self):
        self.assertIn("Layout,MenuPlan,ReadablePanel,SafeTextWidth,Y,DrawnMenuRows,true,bTouchHUD",
                      self.hud)
        self.assertIn("DrawnMenuRows+=OverviewLayout.ReservedRows", self.hud)
        self.assertIn("Y=static_cast<float>(OverviewLayout.NextDetailY)", self.hud)
        self.assertLess(self.hud.index("if (DrawResultsOverview"),
                        self.hud.index("MenuPager.Synchronize("))
        for marker in (
            "DrawFleetReplayPanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD)",
            "DrawLapMotionPanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD)",
            "DrawPracticeResultsPanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD)",
            "DrawSectorMotionPanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD)",
            "VerticalPlan.SaveWarningY", "MenuPlan.SaveWarningY",
        ):
            self.assertIn(marker, self.hud)
        self.assertIn("vertical.OverflowNoticeY-1.25*step", self.policy)
        self.assertIn("HudRadarContains(owner,out.Panel)", self.policy)

    def test_persistence_is_not_inferred_from_a_finish(self):
        for marker in (
            "ResultsOverviewPersistence::SessionOnly", "ResultsOverviewPersistence::ReadOnly",
            "ResultsOverviewPersistence::SaveFailed", "ResultsOverviewPersistence::NotVerified",
            "ResultsOverviewOutcome::InvalidFinish", "if(context.Career)return out;",
        ):
            self.assertIn(marker, self.policy)
        self.assertIn("SESSION ONLY / NO SAVED REWARDS", self.policy)
        self.assertIn("SAVE FAILED / NOT PERSISTED", self.policy)
        self.assertIn("CHECK DETAILS FOR SAVE STATUS", self.policy)

    def test_native_font_measurement_and_missing_clock(self):
        for marker in (
            "GetTextSize(Text,W,H,Font,FontScale)",
            "CanDrawResultsOverview(Layout,Sizes)",
            "BuildResultsOverviewTimingBar(Layout.TimingTrack,Readout)",
            "Readout.BestLapValid?FormatClock", "Readout.ClockValid?FormatClock",
            "TIMING BREAKDOWN UNAVAILABLE", "ACTIVE %.2fs | FEES %.2fs",
        ):
            self.assertIn(marker, self.renderer)
        self.assertLess(self.renderer.index("if(ValueScale<=0)return false;"),
                        self.renderer.index("Rect(Layout.Panel,Panel)"))
        self.assertIn("Rect(Bar.Active,Mint)", self.renderer)
        self.assertIn("Rect(Bar.Fees,Coral)", self.renderer)

    def test_presentation_has_no_authority(self):
        for forbidden in (
            "SaveRecords(", "EarnCredits(", "AddTimePenalty(", "PassCheckpoint(",
            "SetAutonomousControls(", "RestartRace(", "HitTest",
        ):
            self.assertNotIn(forbidden, self.policy)
            self.assertNotIn(forbidden, self.renderer)
        cmake = read("Tests/Standalone/CMakeLists.txt")
        self.assertIn("add_latebrake_portable_research_test(RaceResultsOverview)", cmake)
        self.assertIn("target_sources(latebrake_research_RaceResultsOverview PRIVATE", cmake)


if __name__ == "__main__":
    unittest.main()
