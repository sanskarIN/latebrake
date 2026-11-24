"""Player-facing Win32 2D garage EQUIP-only / race action integration."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class PreviewGarageBottomActionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app=read("Experiments/Win32Preview/LateBrakePreview.cpp")
        cls.policy=read("Experiments/Win32Preview/PreviewGarageBottomAction.h")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
        cls.guide=read("Experiments/Win32Preview/TRIAL_QUICKSTART.md")

    def test_split_buttons_share_mouse_policy_and_rendered_geometry(self):
        for marker in (
            '#include "PreviewGarageBottomAction.h"',
            'uiButton(477,520,185,34,"E/X EQUIP",false);',
            'uiButton(672,520,198,34,"ENTER RACE",false);',
            "PreviewGarageButtonAction(",
            "PreviewGarageBottomAction::EquipOnly",
            "PreviewGarageBottomAction::Race",
            "PreviewGarageBottomAction::Unlock",
        ):
            self.assertIn(marker,self.app)
        self.assertIn("if(x<662)return PreviewGarageBottomAction::EquipOnly;",self.policy)
        self.assertIn("if(x>=672)return PreviewGarageBottomAction::Race;",self.policy)
        self.assertIn("NAME PreviewGarageBottomAction",self.cmake)

    def test_explicit_equip_does_not_start_race_and_uses_save_transaction(self):
        equip=self.app.split("static bool equipViewedCarOnly()",1)[1].split(
            "static void raceWithViewedOwnedCar()",1)[0]
        self.assertIn("if(!equipGarageViewed())",equip)
        self.assertIn("commitSettingsChange(before)",equip)
        self.assertNotIn("beginRace();",equip)
        race=self.app.split("static void raceWithViewedOwnedCar()",1)[1].split(
            "static void activateSelection()",1)[0]
        self.assertIn("if(equipViewedCarOnly())beginRace();",race)
        for marker in (
            "bool equipOnly=pressed('E',lastEquip);",
            "if(equipOnly||pad.recover)equipViewedCarOnly();",
            "selfTestStage=93;",
            "mouseClick(666,534);",
            "mouseClick(500,534);",
            "mouseClick(730,534);",
            "simulateSettingsSaveFailure=true;mouseClick(500,534);",
        ):
            self.assertIn(marker,self.app)
        self.assertIn("Equip without launching a race",self.guide)

if __name__=="__main__":
    unittest.main()
