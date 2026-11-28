"""Verify paused menu input ownership and focus-aware choice routing."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GaragePaletteNavigationBindingTests(unittest.TestCase):
    def setUp(self):
        self.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.header = read("Source/Latebrake/Public/UI/LatebrakeRaceHUD.h")
        self.ini = read("Config/DefaultInput.ini")

    def test_contextual_menu_navigation_preserves_original_paths(self):
        for method, direction, original in (
            ("MenuNext","Next","Mode->MenuNext()"),
            ("MenuPrevious","Previous","Mode->MenuPrevious()"),
            ("MenuDetailsNext","Next","Hud->ScrollMenuDetails(1)"),
            ("MenuDetailsPrevious","Previous","Hud->ScrollMenuDetails(-1)"),
            ("MenuActivate","Confirm","Mode->MenuActivate()"),
            ("CareerStart","Cancel","Mode->StartCareerEvent("),
        ):
            body = self.pawn.split("void ALatebrakeCarPawn::"+method+"()",1)[1]
            body = body.split("\nvoid ALatebrakeCarPawn::",1)[0]
            marker = "Hud->HandleGaragePaletteNavigation(latebrake::GaragePaletteNavigationInput::"+direction+")"
            self.assertIn(marker,body)
            self.assertIn(original,body)
            self.assertLess(body.index(marker),body.index(original))
            self.assertIn("ClearDriverInputs();\n                return;",body)
        body = self.pawn.split("void ALatebrakeCarPawn::CareerNext()",1)[1]
        body = body.split("\nvoid ALatebrakeCarPawn::",1)[0]
        self.assertLess(body.index("Hud->AdvanceGaragePaletteLayer()"),
                        body.index("Mode->CycleCareerEvent()"))

    def test_existing_input_mappings_reused_without_duplicates(self):
        mappings = (
            ("CareerNext","N"),("CareerNext","Gamepad_DPad_Right"),
            ("CareerStart","M"),("CareerStart","Gamepad_DPad_Left"),
            ("MenuNext","Gamepad_DPad_Down"),("MenuPrevious","Gamepad_DPad_Up"),
            ("MenuDetailsNext","Down"),("MenuDetailsPrevious","Up"),
            ("MenuActivate","Enter"),("MenuActivate","Gamepad_FaceButton_Top"),
        )
        for action,key in mappings:
            self.assertEqual(self.ini.count(f'+ActionMappings=(ActionName="{action}",Key={key})'),1)
        for binding in ("BrowseCareer","LaunchCareer","NextMenu","PreviousMenu",
                        "DetailsNext","DetailsPrevious","ConfirmMenu"):
            self.assertIn(binding+".bExecuteWhenPaused = true;",self.pawn)

    def test_open_chooser_owns_confirm_even_before_drawing(self):
        body=self.hud.split("bool ALatebrakeRaceHUD::HandleGaragePaletteNavigation(",1)[1]
        body=body.split("void ALatebrakeRaceHUD::ApplyGaragePaletteChoice(",1)[0]
        for marker in ("if (!GaragePalette.Open) return false;",
                       "Mode->CanCustomizeGarage()", "CloseGaragePalette();",
                       "NavigateGaragePalette(", "GaragePaletteFocus,DrawnGaragePaletteFocus,Input",
                       "GaragePaletteFocus=Decision.Choice;",
                       "ApplyGaragePaletteChoice(latebrake::GaragePaletteChoiceAction(Decision.Choice))",
                       "return true;"):
            self.assertIn(marker,body)
        for forbidden in ("Mode->MenuActivate(", "SaveRecords(", "Garage.Unlock("):
            self.assertNotIn(forbidden,body)

    def test_focus_display_and_lifecycle_reset(self):
        self.assertIn("InitialGaragePaletteFocus(GaragePalette)",self.hud)
        self.assertIn("const bool bFocused = GaragePaletteFocus == i;",self.hud)
        self.assertIn("if (bFocused && bTitle && bTag && bPreviewDrawn)",self.hud)
        self.assertIn("DrawnGaragePaletteFocus = i;",self.hud)
        close=self.hud.split("void ALatebrakeRaceHUD::CloseGaragePalette()",1)[1]
        close=close.split("bool ALatebrakeRaceHUD::ToggleGaragePalette",1)[0]
        self.assertIn("GaragePaletteFocus = -1;",close)
        self.assertIn("DrawnGaragePaletteFocus = -1;",close)
        self.assertIn("int32 GaragePaletteFocus = -1;",self.header)
        self.assertIn("N / PAD RIGHT: OPEN OR SWITCH CHOOSER.",self.hud)
        self.assertIn("UP/DOWN: FOCUS | ENTER/Y: CHOOSE | M/PAD LEFT: CLOSE.",self.hud)
        cmake=read("Tests/Standalone/CMakeLists.txt")
        self.assertIn("add_latebrake_portable_research_test(GaragePaletteNavigation)",cmake)


if __name__ == "__main__":
    unittest.main()
