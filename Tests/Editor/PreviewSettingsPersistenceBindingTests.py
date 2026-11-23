"""Actual Win32 2D settings.dat preservation and transactional UX bindings.

Portable header tests and the native --self-test supplement these source checks;
this does not certify Unreal Editor, hardware gameplay or visual design.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class PreviewSettingsPersistenceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app=read("Experiments/Win32Preview/LateBrakePreview.cpp")
        cls.policy=read("Experiments/Win32Preview/PreviewSettingsPersistence.h")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
        cls.quickstart=read("Experiments/Win32Preview/TRIAL_QUICKSTART.md")

    def test_future_settings_are_detected_on_load_and_before_write(self):
        self.assertIn("IsNewerPreviewSettingsSchema(d.magic,d.version)",self.app)
        self.assertIn("settingsReadOnly=true;return;",self.app)
        self.assertIn("static bool newerSettingsFileOnDisk(const char* file)",self.app)
        self.assertIn("if(newerSettingsFileOnDisk(file))",self.app)
        self.assertIn("if(settingsReadOnly)return false;",self.app)
        self.assertIn("version>PreviewSettingsCurrentVersion",self.policy)
        self.assertIn("NEWER SETTINGS: EQUIP/OPTIONS READ ONLY",self.app)

    def test_saves_have_transactional_durability_and_feedback(self):
        save=self.app.split("static bool saveSettings()",1)[1].split(
            "// Independent livery save:",1)[0]
        for marker in ("FlushFileBuffers(h)", "CloseHandle(h)",
                       "MoveFileExA(tmp,file,0x1u|0x8u)",
                       "if(!flushed||!closed){DeleteFileA(tmp);return false;}",
                       "if(testing)return !simulateSettingsSaveFailure;"):
            self.assertIn(marker,save)
        commit=self.app.split("static bool commitSettingsChange(",1)[1].split(
            "static bool setAppearance(",1)[0]
        self.assertIn("ResolvePreviewSettingsSave(",commit)
        self.assertIn("SETTINGS SAVE FAILED / CHANGE REVERTED",commit)
        self.assertIn("NEWER SETTINGS SAVE: READ ONLY",commit)
        for path in ("Tests/Standalone/PreviewSettingsPersistenceTests.cpp",
                     "Experiments/Win32Preview/PreviewSettingsPersistence.h"):
            self.assertTrue((ROOT/path).is_file())
        self.assertIn("NAME PreviewSettingsPersistence",self.cmake)

    def test_real_actions_rollback_and_headless_native_smoke(self):
        for marker in (
            "if(before.Car!=selectedCar&&!commitSettingsChange(before))return false;",
            "selectedTrack=i;commitSettingsChange(before);",
            "selectedChallenge=i;commitSettingsChange(before);",
            "highContrast=!highContrast;commitSettingsChange(before);",
            "if(commitSettingsChange(before))",
            "if(selectedTrack!=before.Track){",
            "if(selectedChallenge!=before.Event)commitSettingsChange(before);",
            "selfTestStage=92;",
            "simulateSettingsSaveFailure=true;",
            "raceWithViewedOwnedCar();",
            "if(page!=GARAGE||selectedCar!=0||garageViewCar!=5)return false;",
        ):
            self.assertIn(marker,self.app)
        self.assertIn("settings.dat",self.quickstart)
        self.assertIn("rolled back",self.quickstart)

if __name__=="__main__":
    unittest.main()
