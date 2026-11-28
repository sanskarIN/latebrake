"""Practice results visualization uses only finished exact-gate comparisons."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT/path).read_text(encoding="utf-8")


class PracticeGateResultsPanelBindingTests(unittest.TestCase):
    def test_actual_finished_practice_and_fallback(self):
        native=read("Source/Latebrake/Private/UI/LatebrakePracticeResultsHUD.cpp")
        for marker in ("Mode->IsPracticeRace()", "Mode->GetRace().IsFinished()",
                       "Mode->GetPracticeGateReview()", "Mode->GetCheckpointSessionRevision()",
                       "CalculateSectorPanelLayout(", "PracticeViewer.Synchronize(",
                       "BuildPracticePanelBarGeometry(", "Presentation.RowsRendered[I]=Label",
                       "SECTOR ACTIVE", "FASTER LEFT"):
            self.assertIn(marker,native)
        for forbidden in ("SaveRecords(", "MenuActivate(", "RestartRace(", "AddTimePenalty("):
            self.assertNotIn(forbidden,native)
        main=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("if (!DrawPracticeResultsPanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD))",main)
        self.assertIn("DrawSectorMotionPanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD);",main)

    def test_pointer_only_changes_viewer(self):
        native=read("Source/Latebrake/Private/UI/LatebrakePracticeResultsHUD.cpp")
        handler=native.split("bool ALatebrakeRaceHUD::TryHandlePracticeResultsPointer(",1)[1].split(
            "bool ALatebrakeRaceHUD::DrawPracticeResultsPanel(",1)[0]
        self.assertIn("Mode->IsFocusLossGuardActive()",handler)
        self.assertIn("HitPresentedPracticePanel(PresentedPractice,PracticeViewer",handler)
        self.assertLess(handler.index("PresentedPractice={};"),handler.index("PracticeViewer.Act("))
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.assertLess(pawn.index("Hud->TryHandlePracticeResultsPointer(MouseX,MouseY)"),
                        pawn.index("Hud->HitTestMenuMouse(MouseX, MouseY)"))
        self.assertIn("add_latebrake_portable_research_test(PracticeGateResultsPanel)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__=="__main__":
    unittest.main()
