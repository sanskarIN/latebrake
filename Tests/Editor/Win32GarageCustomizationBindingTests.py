"""Source-binding checks for the genuinely wired Win32 2D garage.

These complement strict C++20 state-machine tests and a native Win64
headless EXE smoke test; they do not certify a UE5 3D showroom.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "Experiments/Win32Preview/LateBrakePreview.cpp"
POLICY = ROOT / "Experiments/Win32Preview/PreviewGarageStyle.h"
STANDALONE = ROOT / "Tests/Standalone/PreviewGarageStyleTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
GUIDE = ROOT / "Experiments/Win32Preview/TRIAL_QUICKSTART.md"

class Win32GarageCustomizationBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = APP.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.test = STANDALONE.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.guide = GUIDE.read_text(encoding="utf-8")

    def test_12_car_stable_catalog_and_save_masks(self):
        self.assertIn('#include "PreviewGarageStyle.h"', self.app)
        self.assertIn("inline constexpr int PreviewCarCount = 12;", self.policy)
        self.assertIn("carNames[latebrake_preview::PreviewCarCount]", self.app)
        self.assertIn("carPrices[latebrake_preview::PreviewCarCount]", self.app)
        self.assertIn("d.unlocked>latebrake_preview::PreviewKnownCarMask", self.app)
        self.assertIn("old.car>=latebrake_preview::PreviewCarCount", self.app)
        self.assertIn("d.car>=latebrake_preview::PreviewCarCount", self.app)
        self.assertIn("if(rgt||down)browseGarageDelta(1);", self.app)
        self.assertIn("if(l||up)browseGarageDelta(-1);", self.app)

    def test_real_race_and_showroom_use_same_selected_style(self):
        for marker in (
            "static unsigned int selectedPaint(int id)",
            "const auto style=garageStyles[id];",
            "static void heroCar(int cx,int cy,int id)",
            "static void drawCar(float x,float y,float fx,float fy",
            "selectedPaint(id)",
            "garageStyles[model].Vinyl",
            "garageStyles[model].Wheels",
            "drawCar(race.x,race.y,race.fx,race.fy,selectedPaint(selectedCar),color(255,220,141),selectedCar)",
        ):
            self.assertIn(marker, self.app)

    def test_clicks_keys_and_ownership_authority(self):
        click = self.app.split("static void mouseClick(int x,int y)", 1)[1]
        for marker in (
            "if(page==GARAGE)",
            "setAppearance(0,(unsigned)i)",
            "setAppearance(1,(unsigned)i)",
            "setAppearance(2,(unsigned)i)",
            "browseGarageCar(page*latebrake_preview::PreviewCarsPerPage+i)",
            "unlockSelectedCar()",
        ):
            self.assertIn(marker, click)
        for marker in (
            "bool paint=pressed('V',lastPaint)",
            "stripe=pressed('B',lastStripe)",
            "wheels=pressed('N',lastWheels)",
            "if(paint)cycleAppearance(0);",
            "if(stripe)cycleAppearance(1);",
            "if(wheels)cycleAppearance(2);",
            "if(!(ownedMask&(1<<garageViewCar)))",
        ):
            self.assertIn(marker, self.app)

    def test_additive_versioned_atomic_per_car_persistence(self):
        section = self.app.split("struct GarageAppearanceFile",1)[1].split(
            "// A separate additive V4 lap-record",1)[0]
        for marker in (
            "garage-appearance-v1.dat",
            "garage-appearance-v1.tmp",
            "PreviewAppearanceChecksum(",
            "ValidatePreviewAppearanceArray(",
            "if(testing)return !simulateAppearanceSaveFailure;",
            "FlushFileBuffers(h)",
            "MoveFileExA(tmp,file,0x1u|0x8u)",
            "DeleteFileA(tmp);return false;",
        ):
            self.assertIn(marker, section)
        self.assertIn("const auto before=garageStyles[garageViewCar];", self.app)
        self.assertIn("garageStyles[garageViewCar]=before;", self.app)
        self.assertIn("loadGarageAppearance();", self.app)

    def test_future_appearance_version_never_downgrades_saved_file(self):
        app = self.app
        load = app.split("static void loadGarageAppearance()",1)[1].split(
            "static bool saveGarageAppearance()",1)[0]
        save = app.split("static bool saveGarageAppearance()",1)[1].split(
            "// A separate additive V4 lap-record",1)[0]
        edit = app.split("static bool setAppearance(",1)[1].split(
            "static void cycleAppearance(",1)[0]
        self.assertIn("IsNewerPreviewAppearanceSchema",self.policy)
        self.assertIn("PreviewLookFileVersion = 1u;",self.policy)
        self.assertIn("garageAppearanceReadOnly=true;return;",load)
        self.assertLess(load.index("IsNewerPreviewAppearanceSchema"),
                        load.index("ValidatePreviewAppearanceArray"))
        self.assertIn("if(garageAppearanceReadOnly)return false;",save)
        self.assertEqual(save.count("newerGarageAppearanceFileOnDisk(file)"),2)
        self.assertIn("garageAppearanceReadOnly=true;return false;",save)
        self.assertIn("if(garageAppearanceReadOnly)",edit)
        self.assertIn("garageStyles[garageViewCar]=before;",edit)
        self.assertIn("NEWER LOOK SAVE: READ ONLY / FILE PRESERVED",app)
        self.assertIn("selfTestStage=86;",app)
        self.assertIn("TestFutureSchemaDowngradeGuard();",self.test)
        self.assertIn("newer saved cosmetic format",self.guide)

    def test_independent_tests_and_trial_instructions(self):
        self.assertIn("NAME PreviewGarageStyle", self.cmake)
        self.assertIn("PreviewGarageStyleTests.cpp", self.cmake)
        self.assertIn("250000", self.test)
        self.assertIn("selfTestStage=85;", self.app)
        self.assertIn("Twelve", self.guide)
        self.assertIn("V", self.guide)
        self.assertIn("not a trial", self.guide.lower())

if __name__ == "__main__":
    unittest.main()
