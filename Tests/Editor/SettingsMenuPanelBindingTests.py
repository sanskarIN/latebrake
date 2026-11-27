"""Settings navigation source checks, not installed controller/touch proof."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class SettingsMenuPanelBindingTests(unittest.TestCase):
    def test_contextual_browse_and_select_do_not_change_career(self):
        mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        browse = mode.split("void ALatebrakeRaceGameMode::CycleCareerEvent()", 1)[1]
        settings = browse.split("const int32 Original = CareerMenuSelection;", 1)[0]
        self.assertIn("SettingsSelection.Browse(GetSettingsMenuSnapshot().Access)", settings)
        self.assertIn("return; // Never change a career selection", settings)
        select = mode.split("void ALatebrakeRaceGameMode::MenuActivate()", 1)[1]
        self.assertIn("ActivateSettingsControl(SettingsSelection.Current());", select)
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.assertIn("Region == latebrake::TouchControl::MenuBrowse) CareerNext();", pawn)
        self.assertIn("Region == latebrake::TouchControl::CareerStart) MenuActivate();", pawn)
        self.assertIn("Mode->CycleCareerEvent();", pawn)
        ini = read("Config/DefaultInput.ini")
        self.assertIn('ActionName="CareerNext",Key=Gamepad_DPad_Right', ini)
        self.assertIn('ActionName="MenuActivate",Key=Gamepad_FaceButton_Top', ini)

    def test_final_mutation_rechecks_real_authority_and_save_rollback(self):
        mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        snap = mode.split("ALatebrakeRaceGameMode::GetSettingsMenuSnapshot() const", 1)[1]
        snap, activate = snap.split("void ALatebrakeRaceGameMode::ActivateSettingsControl", 1)
        activate = activate.split("void ALatebrakeRaceGameMode::MenuNext()", 1)[0]
        for marker in ("Controller->IsLocalController()", "Controller->GetPawn()==Player",
                       "HasLocalPlayerDriver()", "AppFocusGuard.IsSuspended()",
                       "ManualPause.IsSuspended()", "bFutureSaveSchema",
                       "RuntimeSun->GetLightComponent()", "RuntimeSky->GetLightComponent()"):
            self.assertIn(marker, snap)
        self.assertIn("if(!Current.Access.Allows(Control)", activate)
        self.assertIn("SaveRecords();", activate)
        self.assertIn("bMilesPerHour = PreviousMilesPerHour;", activate)
        self.assertIn("CycleHudTelemetryMode();", activate)
        self.assertIn("ToggleCircuitLightingPreview();", activate)
        self.assertEqual(activate.count("SaveRecords();"), 1)

    def test_pointer_only_after_full_caption_and_fresh_state(self):
        source = read("Source/Latebrake/Private/UI/LatebrakeSettingsHUD.cpp")
        self.assertIn("Presentation.Rendered[i]=Title && Value && Status", source)
        self.assertIn("HitPresentedSettingsPanel(PresentedSettings", source)
        self.assertIn("Mode->GetSettingsMenuSnapshot(),Canvas->SizeX,Canvas->SizeY", source)
        self.assertIn("PresentedSettings={};", source)
        self.assertIn("Mode->ActivateSettingsControl(Hit.Action);", source)
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("DrawSettingsPanel(Mode,Layout,MouseMenu,ReadablePanel,bTouchHUD)", hud)
        self.assertIn("BROWSE: choose setting | SELECT: change", hud)
        self.assertIn("SELECTED: %s | %s", hud)
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        block = pawn.split("void ALatebrakeCarPawn::MenuMousePressed()", 1)[1]
        self.assertLess(block.index("TryHandleSettingsPointer(MouseX,MouseY)"),
                        block.index("HitTestMenuMouse(MouseX, MouseY)"))
        self.assertIn("add_latebrake_portable_research_test(SettingsMenuPanel)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
