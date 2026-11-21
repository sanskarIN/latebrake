"""Source-only integration contracts for the desktop Canvas menu mouse dock.

The standalone C++ test exercises geometric hit-testing. This does not
render Slate, run Unreal UHT or validate a real installed mouse device.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class DesktopMenuMouseBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.hud_header = source("Source/Latebrake/Public/UI/LatebrakeRaceHUD.h")
        cls.pawn = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.mode = source("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.ini = source("Config/DefaultInput.ini")
        cls.policy = source("Source/Latebrake/Public/UI/HudMenuMouseControls.h")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")

    def test_same_canvas_coordinates_for_render_and_clicks(self):
        for code in (
            "HitTestMenuMouse(", "CalculateDesktopMenuMouseLayout(",
            "latebrake::HitTestDesktopMenuMouse(",
            "static_cast<double>(Canvas->SizeX)",
            "static_cast<double>(Canvas->SizeY)",
        ):
            self.assertIn(code, self.hud)
        self.assertIn("HitTestMenuMouse(double x, double y) const",
                      self.hud_header)
        self.assertIn("MouseMenu.Left - HorizontalBounds.Left", self.hud)
        self.assertIn("Value.Len(), SafeTextWidth,", self.hud)
        self.assertIn("if (MouseMenu.Valid)", self.hud)
        self.assertIn("Button.Bounds.X", self.hud)
        self.assertIn("Button.Bounds.Width", self.hud)

    def test_clicks_require_local_open_menu_and_pawn_pause_binding(self):
        click = self.pawn.split(
            "void ALatebrakeCarPawn::MenuMousePressed()", 1)[1].split(
            "void ALatebrakeCarPawn::CareerNext()", 1)[0]
        for code in ("#if !PLATFORM_ANDROID",
                     "!Mode->IsMenuOpen()", "!PC->IsLocalController()",
                     "PC->GetMousePosition(MouseX, MouseY)",
                     "Hud->HitTestMenuMouse(MouseX, MouseY)",
                     "Action == latebrake::DesktopMenuMouseAction::None",
                     "ClearDriverInputs()", "Mode->MenuPrevious()",
                     "Mode->MenuNext()", "Hud->ScrollMenuDetails(-1)",
                     "Hud->ScrollMenuDetails(1)", "Mode->MenuActivate()",
                     "Mode->ToggleMenu()"):
            self.assertIn(code, click)
        for code in ("AddForce(", "AddTorque(", "SaveRecords(",
                     "RegisterCheckpoint(", "Garage.Unlock(", "EarnCredits(",
                     "StartCareerEvent("):
            self.assertNotIn(code, click)
        self.assertIn('TEXT("MenuMouseClick"), IE_Pressed', self.pawn)
        self.assertIn("MouseMenu.bExecuteWhenPaused = true", self.pawn)
        self.assertIn(
            'ActionName="MenuMouseClick",Key=LeftMouseButton', self.ini)

    def test_wheel_scroll_is_local_paused_menu_details_only(self):
        # Source-level integration only, not UE5.8 device or UHT runtime QA.
        wheel = self.pawn.split(
            "void ALatebrakeCarPawn::ScrollMenuMouseWheel(int direction)",
            1)[1].split(
            "void ALatebrakeCarPawn::MenuMousePressed()", 1)[0]
        for marker in ("#if !PLATFORM_ANDROID", "Mode->IsMenuOpen()",
                       "PC->IsLocalController()",
                       "PC->GetMousePosition(MouseX, MouseY)",
                       "Hud->TryScrollMenuMouse(MouseX, MouseY, direction)",
                       "ClearDriverInputs()"):
            self.assertIn(marker, wheel)
        for marker in ("Mode->MenuNext()", "Mode->MenuActivate()",
                       "StartCareerEvent(", "GarageUnlockNext(", "EarnCredits("):
            self.assertNotIn(marker, wheel)
        for action, key, handler in (
            ("MenuMouseWheelUp", "MouseScrollUp", "MenuMouseWheelUp"),
            ("MenuMouseWheelDown", "MouseScrollDown", "MenuMouseWheelDown"),
        ):
            self.assertIn(
                f'ActionName="{action}",Key={key}', self.ini)
            self.assertIn(f'TEXT("{action}"), IE_Pressed', self.pawn)
            self.assertIn(f'&ALatebrakeCarPawn::{handler}', self.pawn)
        self.assertIn("WheelUp.bExecuteWhenPaused = true", self.pawn)
        self.assertIn("WheelDown.bExecuteWhenPaused = true", self.pawn)
        self.assertIn("TryScrollMenuMouse(double x, double y, int direction)",
                      self.hud_header)
        self.assertIn("latebrake::CalculateDesktopMenuWheelZone(", self.hud)
        self.assertIn("latebrake::AdmitDesktopMenuWheel(", self.hud)
        self.assertIn("Mode->GetMenuPage(), MenuPager", self.hud)
        self.assertIn('TEXT("WHEEL: DETAILS")', self.hud)
        self.assertIn("CalculateHudMenuVerticalPlan(", self.policy)
        self.assertIn("CalculateHudTextBounds(", self.policy)

    def test_pointer_ownership_tracks_game_mode_not_hud_render(self):
        focus = self.mode.split(
            "void ALatebrakeRaceGameMode::SyncDesktopMenuPointerMode()", 1)[1].split(
            "void ALatebrakeRaceGameMode::CloseMenuForRace()", 1)[0]
        for marker in ("#if !PLATFORM_ANDROID", "PC->bShowMouseCursor",
                       "FInputModeGameAndUI", "FInputModeGameOnly",
                       "SetHideCursorDuringCapture(false)",
                       "PC->SetInputMode("):
            self.assertIn(marker, focus)
        self.assertGreaterEqual(
            self.mode.count("SyncDesktopMenuPointerMode();"), 2)
        self.assertNotIn("SyncDesktopMenuPointerMode();", self.hud)

    def test_disabled_detail_controls_match_render_and_dispatch(self):
        # This is a source integration contract, not proof of Unreal UHT.
        # The displayed muted buttons and the Pawn's click result must use
        # the exact same visible-page/pager availability policy.
        self.assertEqual(self.hud.count(
            "latebrake::AvailableDesktopMenuDetails("), 2)
        self.assertIn("Mode->GetMenuPage(), MenuPager", self.hud)
        self.assertIn("Mode->GetMenuPage(), MenuPager);", self.hud)
        self.assertIn(
            "return latebrake::HitTestDesktopMenuMouse(buttons, x, y,",
            self.hud)
        self.assertIn("MouseMenu, CursorX, CursorY, MenuAvailability)",
                      self.hud)
        self.assertIn("Button.Action, MenuAvailability", self.hud)
        self.assertIn("const bool Hover = Enabled &&", self.hud)
        self.assertIn("!Enabled ? DisabledLabel", self.hud)
        self.assertIn('TEXT("MENU PAGE %d/%d")', self.hud)
        for source_marker in (
            "visiblePage == MenuPage::Closed",
            "pager.GetPage() != visiblePage",
            "window.HasPrevious, window.HasNext",
            "DesktopMenuMouseActionEnabled(button.Action, availability)",
            "DesktopMenuMouseAction::None",
        ):
            self.assertIn(source_marker, self.policy)
        self.assertNotIn("Mode->MenuActivate(", self.hud)
        self.assertNotIn("Mode->MenuNext(", self.hud)

    def test_bounded_pure_test_policy(self):
        for term in ("touchUI || !menuOpen || layout.Compact",
                     "std::isfinite(width)", "std::isfinite(height)",
                     "x < X + Width", "y < Y + Height",
                     "HitTestDesktopMenuMouse(", "MouseAction"):
            if term == "MouseAction":
                continue
            self.assertIn(term, self.policy)
        self.assertIn("HudMenuMouseControlsTests.cpp", self.cmake)
        self.assertIn("NAME HudMenuMouseControls", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)


if __name__ == "__main__":
    unittest.main()
