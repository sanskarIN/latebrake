"""Direct desktop Quick Race choice only mutates the pending configuration."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT/path).read_text(encoding="utf-8")


class QuickRaceChoicePanelBindingTests(unittest.TestCase):
    def test_pending_choice_no_launch(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        body=mode.split("bool ALatebrakeRaceGameMode::SelectQuickRaceChoice(",1)[1].split(
            "void ALatebrakeRaceGameMode::MenuNext()",1)[0]
        for marker in ("Snapshot.CanSelect()", "QuickRaceChoiceAt(Index)",
                       "QuickRaceDifficultySelection=Next.Difficulty",
                       "bQuickRaceSoloTimeAttackSelection=Next.SoloTimeAttack",
                       "QuickPracticeSelection=Next.Practice", "BumpQuickChoiceRevision()"):
            self.assertIn(marker,body)
        for forbidden in ("RestartRace(", "StartQuickPractice(", "SaveRecords(",
                          "SelectedEvent =", "ActiveRivalDifficulty", "CloseMenuForRace("):
            self.assertNotIn(forbidden,body)
        self.assertIn("SettingsSelection.Revision(),QuickChoiceRevision",mode)

    def test_actual_rendered_cards_and_pointer_dispatch(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeQuickRaceHUD.cpp")
        self.assertIn("Presentation.Rendered[I]=Title && DetailShown && Status;",hud)
        self.assertIn("HitPresentedQuickRaceChoice(PresentedQuickRace",hud)
        self.assertIn("Mode->GetQuickRaceChoiceSnapshot()",hud)
        self.assertLess(hud.index("PresentedQuickRace={};"),hud.index("Mode->SelectQuickRaceChoice("))
        self.assertIn("if(Hit.Index>=0)Mode->SelectQuickRaceChoice(Hit.Index)",hud)
        self.assertNotIn("MenuActivate(",hud)
        main=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("DrawQuickRaceChoicePanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD)",main)
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.assertLess(pawn.index("Hud->TryHandleQuickRaceChoicePointer(MouseX,MouseY)"),
                        pawn.index("Hud->HitTestMenuMouse(MouseX, MouseY)"))
        self.assertIn("add_latebrake_portable_research_test(QuickRaceChoicePanel)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__=="__main__":
    unittest.main()
