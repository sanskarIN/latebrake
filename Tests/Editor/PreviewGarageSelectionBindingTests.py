"""2D Win64 garage UX wiring: viewing is not buying or equipping.

This source-binding test supplements the actual Windows --self-test and
portable C++ tests, not a UE5 editor or a human UX playtest.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class PreviewGarageSelectionBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app=read("Experiments/Win32Preview/LateBrakePreview.cpp")
        cls.policy=read("Experiments/Win32Preview/PreviewGarageSelection.h")
        cls.tests=read("Tests/Standalone/PreviewGarageSelectionTests.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
        cls.quickstart=read("Experiments/Win32Preview/TRIAL_QUICKSTART.md")

    def test_showroom_is_separate_from_equipped_and_saved_settings(self):
        for marker in (
            "static int selectedCar=0,garageViewCar=0;",
            "selectedCar=latebrake_preview::SafeEquippedPreviewCar(",
            "garageViewCar=selectedCar;",
            "browseGarageCar(page*latebrake_preview::PreviewCarsPerPage+i)",
            "const int page=garageViewCar/latebrake_preview::PreviewCarsPerPage;",
            "if(garageViewCar!=11||selectedCar!=0)return false;",
            "selfTestStage=89;",
        ):
            self.assertIn(marker,self.app)
        self.assertIn("(unsigned)selectedCar,(unsigned)selectedTrack",self.app)
        self.assertNotIn("(unsigned)garageViewCar,(unsigned)selectedTrack",self.app)
        self.assertIn("same V4 settings field",self.quickstart)

    def test_viewing_and_equip_are_separate_live_input_paths(self):
        selected=self.app.split("static bool equipViewedCarOnly()",1)[1].split(
            "static void raceWithViewedOwnedCar()",1)[0]
        self.assertIn("if(!equipGarageViewed())",selected)
        self.assertIn("if(before.Car!=selectedCar&&!commitSettingsChange(before))return false;",selected)
        self.assertIn("return true;",selected)
        race=self.app.split("static void raceWithViewedOwnedCar()",1)[1].split(
            "static void activateSelection()",1)[0]
        self.assertIn("if(equipViewedCarOnly())beginRace();",race)
        actual=self.app.split("static void activateSelection()",1)[1].split(
            "static void mouseClick(",1)[0]
        self.assertIn("else if(page==GARAGE)raceWithViewedOwnedCar();",actual)
        mouse=self.app.split("static void mouseClick(",1)[1].split(
            "static void advanceRaceFixed(",1)[0]
        self.assertIn("browseGarageCar(page*latebrake_preview::PreviewCarsPerPage+i);",mouse)
        self.assertIn("raceWithViewedOwnedCar();",mouse)
        self.assertIn("unlockSelectedCar();",mouse)
        self.assertIn("INSUFFICIENT CREDITS / KEEP RACING",mouse)

    def test_unlock_has_no_hidden_equip_authority(self):
        buy=self.app.split("static void unlockSelectedCar()",1)[1].split(
            "static void raceWithViewedOwnedCar()",1)[0]
        self.assertIn("if(page!=GARAGE||!latebrake_preview::ValidPreviewCar(garageViewCar))return;",buy)
        self.assertIn("credits-=carPrices[garageViewCar];ownedMask|=1<<garageViewCar;",buy)
        self.assertIn("if(saveProgress())",buy)
        self.assertIn("credits=oldCredits;ownedMask=oldMask;",buy)
        self.assertNotIn("selectedCar=",buy)
        self.assertNotIn("saveSettings()",buy)

    def test_policy_matches_live_navigation_and_ui(self):
        for marker in (
            "Equipped=SafeEquippedPreviewCar",
            "Viewed=Equipped",
            "if (!OwnsPreviewCar(ownedMask,Viewed)) return false;",
            "Equipped=Viewed;",
            "ValidPreviewCar(Viewed)",
        ):
            self.assertIn(marker,self.policy)
        self.assertIn("300000",self.tests)
        self.assertIn("NAME PreviewGarageSelection",self.cmake)
        self.assertIn("static bool browseGarageDelta(int direction)",self.app)
        self.assertIn("cursor.BrowseDelta(direction)",self.app)
        self.assertIn("static bool equipGarageViewed()",self.app)
        self.assertIn("cursor.EquipViewed(static_cast<unsigned int>(ownedMask))",self.app)
        self.assertIn("EQUIPPED / OWNED",self.app)
        self.assertIn("LOCKED / EARN CREDITS",self.app)
        self.assertIn("E/X EQUIP",self.app)
        self.assertIn("ENTER RACE",self.app)

if __name__=="__main__":
    unittest.main()
