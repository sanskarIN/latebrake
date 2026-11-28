"""Profile overview source integration, not native render qualification."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class ProfileOverviewPanelBindingTests(unittest.TestCase):
    def setUp(self):
        self.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.renderer = read("Source/Latebrake/Private/UI/LatebrakeProfileOverviewHUD.cpp")
        self.policy = read("Source/Latebrake/Public/UI/ProfileOverviewPanel.h")

    def test_actual_profile_source_and_compact_fallback(self):
        for marker in (
            "Mode->GetCareer().Snapshot()", "Mode->GetGarageSnapshot()",
            "Mode->GetCompletedRaceCount()", "Mode->GetPersonalBestSeconds()",
            "Mode->GetPersonalBestLapSeconds()", "Mode->IsProfileReadOnly()",
            "Mode->HasProgressSaveError()",
        ):
            self.assertIn(marker, self.hud)
        self.assertIn("const auto Profile=latebrake::ReadProfileOverview", self.hud)
        self.assertIn("MEDALS: BRONZE %d | SILVER %d | GOLD %d", self.hud)
        self.assertIn("if(Profile.CircuitBestValid)", self.hud)
        self.assertIn("if(Profile.BestLapValid)", self.hud)
        for marker in (
            "career.Load(source.Career)", "garage.Load(source.Garage)",
            "SanitizeCompletedRaces(source.CompletedRaces)",
            "SanitizeBestTime(source.CircuitBest)", "SanitizeBestTime(source.BestLap)",
            "!source.ReadOnly && !source.SaveFailed",
        ):
            self.assertIn(marker, self.policy)

    def test_existing_menu_budget(self):
        self.assertIn("Layout,MenuPlan,ReadablePanel,SafeTextWidth,Y,DrawnMenuRows,true,bTouchHUD",
                      self.hud)
        self.assertIn("DrawnMenuRows+=ProfileLayout.ReservedRows", self.hud)
        self.assertIn("Y=static_cast<float>(ProfileLayout.NextDetailY)", self.hud)
        self.assertIn("vertical.OverflowNoticeY-1.25*step", self.policy)
        self.assertIn("ProfileOverviewMinDetails", self.policy)
        self.assertIn("!profile || touch || hud.Compact", self.policy)

    def test_all_text_measured_before_draw(self):
        self.assertIn("GetTextSize(Text,W,H,Font,Scale)", self.renderer)
        self.assertIn("CanDrawProfileOverview(Layout,Sizes)", self.renderer)
        self.assertIn("BuildProfileOverviewBars(Layout,Data)", self.renderer)
        self.assertLess(self.renderer.index("if(ValueScale<=0)return false;"),
                        self.renderer.index("Rect(Layout.Panel,Panel)"))
        self.assertIn("if(!Valid)return FString(TEXT(\"N/A\"));", self.renderer)

    def test_read_only_presentation(self):
        for forbidden in (
            "SaveRecords(", "EarnCredits(", "TryPurchase(", "CommitFinish(",
            "RestartRace(", "SetAutonomousControls(", "HitTest",
        ):
            self.assertNotIn(forbidden, self.renderer)
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("add_latebrake_portable_research_test(ProfileOverviewPanel)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
