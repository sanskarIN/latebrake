"""Source-level binding for the normal finished-race sector panel."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class SectorMotionResultsPanelBindingTests(unittest.TestCase):
    def test_frozen_actual_history(self):
        source = read("Source/Latebrake/Private/Game/LatebrakeSectorMotionRuntime.cpp")
        block = source.split("ALatebrakeRaceGameMode::GetPlayerSectorMotionHistory()", 1)[1]
        self.assertIn("if (!Race.IsFinished()) return {}", block)
        self.assertIn("PlayerSectorMotion.ReadFinishedHistory(", block)
        self.assertIn("Race.GetElapsedSeconds()", block)
        self.assertIn("Race.GetTotalPenaltySeconds()", block)

    def test_render_and_input_share_actual_layout(self):
        source = read("Source/Latebrake/Private/UI/LatebrakeSectorMotionHUD.cpp")
        for marker in ("Mode->GetMenuPage()==latebrake::MenuPage::Results",
                       "Mode->GetRace().IsFinished()", "!Mode->IsExperimentalFleetRequested()",
                       "PresentedSector.ViewportWidth!=Canvas->SizeX",
                       "PresentedSector.ViewportHeight!=Canvas->SizeY",
                       "PresentedSectorRevision!=Mode->GetCheckpointSessionRevision()",
                       "HitSectorPanel(PresentedSector,X,Y)", "SectorViewer.Matches(",
                       "CalculateSectorPanelLayout(Canvas->SizeX,Canvas->SizeY",
                       "BuildSectorPanelBarGeometry(Row,Bar,Layout.Scale)",
                       "Mode->GetPlayerSectorMotionHistory()"):
            self.assertIn(marker, source)
        for marker in ("RestartRace(", "Race.Advance(", "SaveGame", "SetActorLocation("):
            self.assertNotIn(marker, source)

    def test_no_interception_of_normal_menu_outside_panel(self):
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        block = pawn.split("void ALatebrakeCarPawn::MenuMousePressed()", 1)[1]
        self.assertLess(block.index("TryHandleSectorMotionPointer(MouseX,MouseY)"),
                        block.index("HitTestMenuMouse(MouseX, MouseY)"))
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("DrawSectorMotionPanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD)", hud)
        self.assertIn("SectorViewer.Reset()", hud)
        self.assertIn("add_latebrake_portable_research_test(SectorMotionResultsPanel)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
