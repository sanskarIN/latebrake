"""Native full-lap chart owns only the finished desktop Results lane."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class LapMotionResultsPanelBindingTests(unittest.TestCase):
    def test_result_context_and_exclusive_panel(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeLapMotionHUD.cpp")
        for marker in ("Mode->GetMenuPage()==latebrake::MenuPage::Results",
                       "Mode->GetRace().IsFinished()", "!Mode->IsExperimentalFleetRequested()",
                       "Mode->GetCheckpointSessionRevision()", "GetPlayerLapMotionReview()",
                       "Presentation.RowsRendered[I]=Label", "HitPresentedLapPanel"):
            self.assertIn(marker,hud)
        for forbidden in ("RestartRace(", "SaveRecords(", "MenuActivate(", "AddTimePenalty("):
            self.assertNotIn(forbidden,hud)
        main=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertLess(main.index("if (!DrawLapMotionPanel("),main.index("if (!DrawPracticeResultsPanel("))
        self.assertIn("PresentedLapMotion={};",main)
    def test_input_is_local_and_paused(self):
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.assertIn("PC->GetPawn()!=this",pawn)
        self.assertIn("Hud->ToggleLapMotionPanel();",pawn)
        self.assertIn("&ALatebrakeCarPawn::ToggleLapMotionPanel).bExecuteWhenPaused=true",pawn)
        self.assertLess(pawn.index("Hud->TryHandleLapMotionPointer"),pawn.index("Hud->TryHandleSectorMotionPointer"))
        self.assertIn('ActionName="ToggleLapMotionPanel",Key=F11',read("Config/DefaultInput.ini"))
        self.assertIn("add_latebrake_portable_research_test(LapMotionResultsPanel)",read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
