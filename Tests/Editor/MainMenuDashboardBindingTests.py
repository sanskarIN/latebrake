"""Source integration for direct page navigation; not native UI acceptance."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class MainMenuDashboardBindingTests(unittest.TestCase):
    def setUp(self):
        self.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.renderer = read("Source/Latebrake/Private/UI/LatebrakeMainMenuHUD.cpp")
        self.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        self.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.policy = read("Source/Latebrake/Public/UI/MainMenuDashboard.h")

    def test_menu_layout_reserves_actual_rows(self):
        begin = self.hud.index("const auto Dashboard=latebrake::CalculateMainMenuDashboardLayout")
        end = self.hud.index("MenuPager.Synchronize(", begin)
        block = self.hud[begin:end]
        self.assertIn("SafeTextWidth,Y,DrawnMenuRows,bTouchHUD,Mode->GetMenuPage()", block)
        self.assertIn("DrawnMenuRows+=Dashboard.ReservedRows", block)
        self.assertIn("Y=static_cast<float>(Dashboard.NextDetailY)", block)
        self.assertIn("PresentedMainMenu={};", self.hud)
        self.assertIn("ContainsSettingsRect(owner,out.Panel)", self.policy)
        self.assertIn("vertical.OverflowNoticeY-1.25*step", self.policy)

    def test_native_page_transition_is_not_activation(self):
        block = self.mode.split("bool ALatebrakeRaceGameMode::NavigateMainMenuDashboard(", 1)[1]
        block = block.split("void ALatebrakeRaceGameMode::MenuNext()", 1)[0]
        markers = (
            "GetMainMenuDashboardSnapshot()", "Current.AllowsNavigation()", "Current!=Expected",
            "ValidMainMenuDestination(Destination)", "GaragePurchaseConfirm.Reset()",
            "DismissGaragePalette()", "Menu.Open(Destination)", "SettingsSelection.Reset()",
            "BumpQuickChoiceRevision()", "SyncGarageShowroomPreview()",
        )
        indices = [block.index(marker) for marker in markers]
        self.assertEqual(indices, sorted(indices))
        for forbidden in (
            "StartQuickPractice(", "RestartRace(", "SetGamePaused(", "SaveRecords(",
            "MenuActivate(", "GarageUnlockNext(", "CloseMenuForRace(",
        ):
            self.assertNotIn(forbidden, block)

    def test_actual_local_driver_and_revision(self):
        block = self.mode.split("ALatebrakeRaceGameMode::GetMainMenuDashboardSnapshot()", 1)[1]
        block = block.split("bool ALatebrakeRaceGameMode::NavigateMainMenuDashboard", 1)[0]
        for marker in (
            "Controller->IsLocalController()", "Controller->GetPawn()==Player",
            "HasLocalPlayerDriver()", "Menu.IsOpen()", "Menu.Current()",
            "AppFocusGuard.IsSuspended()", "SettingsSelection.Revision()",
        ):
            self.assertIn(marker, block)

    def test_measure_every_title_before_publishing_hit_targets(self):
        self.assertIn("GetTextSize(Text,W,H,Font,Scale)", self.renderer)
        self.assertLess(self.renderer.index("if(Titles[I].Scale<=0)return false;"),
                        self.renderer.index("Rect(Layout.Panel,Surface)"))
        self.assertLess(self.renderer.index("Presentation.Rendered[I]=true;"),
                        self.renderer.index("PresentedMainMenu=Presentation;"))
        self.assertIn("PresentedMainMenu={};", self.renderer)
        self.assertIn("presented.Snapshot!=current", self.policy)
        self.assertIn("presented.Rendered[i]?card.Destination", self.policy)

    def test_pointer_path_consumes_without_falling_through(self):
        block = self.renderer.split("bool ALatebrakeRaceHUD::TryHandleMainMenuDashboardPointer", 1)[1]
        block = block.split("bool ALatebrakeRaceHUD::DrawMainMenuDashboard", 1)[0]
        self.assertIn("#if PLATFORM_ANDROID", block)
        self.assertIn("PlayerOwner->IsLocalController()", block)
        self.assertIn("HitPresentedMainMenuDashboard(PresentedMainMenu", block)
        self.assertIn("if(!Hit.Consumed)return false;", block)
        self.assertIn("Mode->NavigateMainMenuDashboard(Hit.Destination,Current)", block)
        self.assertLess(self.pawn.index("Hud->TryHandleMainMenuDashboardPointer(MouseX,MouseY)"),
                        self.pawn.index("Hud->HitTestMenuMouse(MouseX, MouseY)"))
        self.assertIn("add_latebrake_portable_research_test(MainMenuDashboard)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
