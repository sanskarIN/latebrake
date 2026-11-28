"""Source wiring for the optional finished-results replay panel."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class FleetReplayPanelBindingTests(unittest.TestCase):
    def test_results_only_real_time_and_frozen_replay(self):
        source = read("Source/Latebrake/Private/UI/LatebrakeFleetReplayHUD.cpp")
        for marker in ("Mode->GetMenuPage()==latebrake::MenuPage::Results",
                       "Mode->GetRace().IsFinished()", "Mode->IsExperimentalFleetRequested()",
                       "ReplayViewer.Tick(FPlatformTime::Seconds()",
                       "Mode->EvaluateFleetReplay(State.CursorSeconds)",
                       "BuildFleetReplayMap(Frame,Layout,State)"):
            self.assertIn(marker, source)
        for forbidden in ("SetActorLocation", "SetActorTransform", "AdvancePlayerRaceClock",
                          "RestartRace(", "SaveRecords("):
            self.assertNotIn(forbidden, source)

    def test_actual_rendered_geometry_gates_pointer(self):
        source = read("Source/Latebrake/Private/UI/LatebrakeFleetReplayHUD.cpp")
        self.assertIn("PresentedReplay.ViewportWidth!=Canvas->SizeX", source)
        self.assertIn("PresentedReplay.ViewportHeight!=Canvas->SizeY", source)
        self.assertIn("Review.Attempt!=PresentedReplayAttempt", source)
        self.assertIn("HitFleetReplayPanel(PresentedReplay,X,Y)", source)
        self.assertIn("HitFleetReplayMapCar(PresentedReplay,PresentedReplayMap,X,Y)", source)
        self.assertIn("ReplayViewer.SelectFocus(Slot,PresentedReplaySeconds,Review)", source)
        self.assertIn("PresentedReplayMap=Map", source)
        self.assertIn("PresentedReplaySeconds=State.CursorSeconds", source)
        self.assertIn("Layout.ControlsStatus", source)
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("DrawFleetReplayPanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD)", hud)
        self.assertIn("ReplayViewer.Reset()", hud)

    def test_paused_input_does_not_override_normal_menu(self):
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        mouse = pawn.split("void ALatebrakeCarPawn::MenuMousePressed()", 1)[1]
        self.assertLess(mouse.index("TryHandleFleetReplayPointer(MouseX,MouseY)"),
                        mouse.index("HitTestMenuMouse(MouseX, MouseY)"))
        dispatch = pawn.split("void ALatebrakeCarPawn::DispatchFleetReplayAction(", 1)[1].split(
            "void ALatebrakeCarPawn::FleetReplayPlayPause()", 1)[0]
        self.assertIn("Mode->GetMenuPage()!=latebrake::MenuPage::Results", dispatch)
        self.assertIn("Mode->GetRace().IsFinished()", dispatch)
        self.assertIn("PC->IsLocalController()", dispatch)
        config = read("Config/DefaultInput.ini")
        for action, key in (("FleetReplayPrevious", "J"), ("FleetReplayPlayPause", "K"),
                            ("FleetReplayNext", "L"), ("FleetReplayHighlight", "H")):
            self.assertIn(f'ActionName="{action}",Key={key}', config)
            self.assertIn(f'BindAction(TEXT("{action}")', pawn)
        self.assertIn("add_latebrake_portable_research_test(FleetReplayPanel)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
