"""Career cards source integration, not native engine render acceptance."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class CareerEventCardsBindingTests(unittest.TestCase):
    def setUp(self):
        self.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.renderer = read("Source/Latebrake/Private/UI/LatebrakeCareerEventCardsHUD.cpp")
        self.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        self.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.policy = read("Source/Latebrake/Public/UI/CareerEventCards.h")

    def test_actual_pending_selection_and_profile_snapshot(self):
        start = self.mode.index("ALatebrakeRaceGameMode::GetCareerEventCardsSnapshot()")
        end = self.mode.index("bool ALatebrakeRaceGameMode::SelectCareerEventCard(", start)
        block = self.mode[start:end]
        for marker in (
            "Controller->IsLocalController()", "Controller->GetPawn()==Player",
            "AppFocusGuard.IsSuspended()", "bFutureSaveSchema",
            "CareerMenuSelection", "SettingsSelection.Revision()",
            "Career.Snapshot()", "Garage.Snapshot()",
        ):
            self.assertIn(marker, block)
        self.assertIn("ResolveCareerMenuTarget(progress,page,selection)", self.policy)

    def test_click_stages_existing_cursor_not_launch_or_save(self):
        start = self.mode.index("bool ALatebrakeRaceGameMode::SelectCareerEventCard(")
        end = self.mode.index("void ALatebrakeRaceGameMode::MenuNext()", start)
        block = self.mode[start:end]
        for marker in (
            "Current!=Expected", "Description.Selectable", "Career.Status(Id)",
            "IsEventInCareerMenu(Menu.Current(),Id)", "CareerMenuSelection=Id;",
            "SettingsSelection.Changed()", "GaragePurchaseConfirm.Reset()",
        ):
            self.assertIn(marker, block)
        for forbidden in (
            "StartCareerEvent(", "Career.Select(Id)", "SaveRecords(",
            "RestartRace(", "CloseMenuForRace(", "SetGamePaused(",
        ):
            self.assertNotIn(forbidden, block)
        self.assertIn("if (StartCareerEvent(Target)) CloseMenuForRace();", self.mode)

    def test_same_page_and_real_card_states(self):
        self.assertIn("if (latebrake::IsCareerMenuPage(Mode->GetMenuPage()))", self.hud)
        self.assertIn("DrawnMenuRows+=Cards.ReservedRows", self.hud)
        self.assertIn("Y=static_cast<float>(Cards.NextDetailY)", self.hud)
        self.assertIn("PresentedCareerCards={};", self.hud)
        self.assertIn("shown.Snapshot!=current", self.policy)
        self.assertIn("shown.Layout.Page!=current.Page", self.policy)
        self.assertIn("CareerCardReadiness::NeedsCar", self.policy)
        self.assertIn("CareerCardReadiness::LockedPrerequisite", self.policy)
        self.assertIn("status!=CareerStatus::Locked", self.policy)

    def test_native_measurements_before_pointer_receipt(self):
        self.assertIn("GetTextSize(Text,W,H,Font,Scale)", self.renderer)
        self.assertIn("READ-ONLY PROFILE | BROWSE ONLY", self.renderer)
        self.assertIn("CHOOSE EVENT | SELECT TO START", self.renderer)
        self.assertLess(self.renderer.index("if(P.Name.Scale<=0 || P.Detail.Scale<=0 || P.State.Scale<=0)"),
                        self.renderer.index("Rect(Layout.Panel,Surface)"))
        self.assertLess(self.renderer.index("Presentation.Rendered[static_cast<std::size_t>(I)]=true;"),
                        self.renderer.index("PresentedCareerCards=Presentation;"))
        self.assertIn("HitPresentedCareerEventCards(PresentedCareerCards", self.renderer)
        self.assertIn("Mode->SelectCareerEventCard(Hit.EventId,Current)", self.renderer)

    def test_existing_input_dispatch_and_test_registration(self):
        self.assertLess(self.pawn.index("Hud->TryHandleCareerEventCardsPointer(MouseX,MouseY)"),
                        self.pawn.index("Hud->HitTestMenuMouse(MouseX, MouseY)"))
        self.assertIn("add_latebrake_portable_research_test(CareerEventCards)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
