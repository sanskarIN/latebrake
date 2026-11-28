"""Direct practice trial pointer input must remain separate from launch and garage."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")

class PracticeTrialPanelBindingTests(unittest.TestCase):
    def test_pointer_dispatch_checks_real_presentation(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeQuickRaceHUD.cpp")
        handler=hud.split("bool ALatebrakeRaceHUD::TryHandleQuickRaceChoicePointer",1)[1].split(
            "void ALatebrakeRaceHUD::DrawQuickRaceChoicePanel",1)[0]
        for token in ("PlayerOwner->IsLocalController()", "GetPendingPracticeTrial()",
                      "IsManualRacePauseActive()", "HitPresentedPracticeTrial(PresentedTrial",
                      "if(TrialHit.Accepted)Mode->HandlePracticeTrialInput", "PresentedTrial={};"):
            self.assertIn(token,handler)
        self.assertLess(handler.index("HitPresentedPracticeTrial"),handler.index("HitPresentedQuickRaceChoice"))
        for forbidden in ("StartQuickPractice(", "SaveRecords(", "GarageUnlockNext(", "MenuActivate("):
            self.assertNotIn(forbidden,handler)
        self.assertIn("TrialPresentation.Rendered[I]=Label",hud)
        self.assertIn("FREE PRACTICE SETUP | NEVER SAVED",hud)
        self.assertIn("CalculatePracticeTrialPanel(Layout,Text)",hud)
        self.assertIn("PresentedTrial=TrialPresentation;",hud)

    def test_original_keyboard_callbacks_own_trial_shortcuts(self):
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        for method,action in (("UnlockGarageVehicle","Toggle"),("UpgradeGarageEngine","NextEngine"),
                              ("UpgradeGarageGrip","NextGrip"),("NextGarageVehicle","NextCar")):
            block=pawn.split(f"void ALatebrakeCarPawn::{method}()",1)[1].split(
                "\nvoid ALatebrakeCarPawn::",1)[0]
            self.assertIn(f"HandlePracticeTrialInput(latebrake::PracticeTrialAction::{action})",block)
        self.assertIn("add_latebrake_portable_research_test(PracticeTrialPanel)",
                      read("Tests/Standalone/CMakeLists.txt"))

if __name__=="__main__":unittest.main()
