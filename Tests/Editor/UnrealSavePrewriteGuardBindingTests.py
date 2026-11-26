"""Guard against overwriting on-disk Unreal profiles written by newer builds.

This is source-level integration plus pure portable tests; only a native UE
Editor/cooked build can verify actual SaveGame platform APIs.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class UnrealSavePrewriteGuardBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.policy=read("Source/Latebrake/Public/Progression/ProfileSaveWriteGuard.h")
        cls.tests=read("Tests/Standalone/ProfileSaveWriteGuardTests.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
        cls.qa=read("docs/NATIVE_GARAGE_RECONCILIATION_QA.md")

    def test_real_save_reloads_existing_profile_before_writing(self):
        source=self.mode.split(
            "void ALatebrakeRaceGameMode::SaveRecords()",1)[1]
        for token in (
            "if (bFutureSaveSchema)",
            "UGameplayStatics::DoesSaveGameExist(SaveSlot, SaveUser)",
            "UGameplayStatics::LoadGameFromSlot(SaveSlot, SaveUser)",
            "DiskProfile ? DiskProfile->SaveSchemaVersion : 0",
            "DiskProfile ? DiskProfile->GarageAppearanceSchemaVersion : 0",
            "latebrake::AssessProfileSaveWrite(",
            "if (!latebrake::CanWriteProfileSave(DiskState))",
            "bProgressSaveFailed = true;",
            "bFutureSaveSchema = true;",
            "CreateSaveGameObject(ULatebrakePlayerSave::StaticClass())",
            "UGameplayStatics::SaveGameToSlot(Save, SaveSlot, SaveUser)",
        ):
            self.assertIn(token,source)
        self.assertLess(source.index("DoesSaveGameExist("),
                        source.index("CreateSaveGameObject("))
        self.assertLess(source.index("CanWriteProfileSave(DiskState)"),
                        source.index("SaveGameToSlot(Save, SaveSlot, SaveUser)"))
        self.assertLess(source.index("LoadGameFromSlot(SaveSlot, SaveUser)"),
                        source.index("SaveGameToSlot(Save, SaveSlot, SaveUser)"))

    def test_gate_denies_future_or_corrupt_data_without_new_schema(self):
        for marker in (
            "ProfileSaveWriteState::NewProfile",
            "ProfileSaveWriteState::CompatibleProfile",
            "ProfileSaveWriteState::UnreadableProfile",
            "ProfileSaveWriteState::CorruptSchema",
            "ProfileSaveWriteState::NewerCareer",
            "ProfileSaveWriteState::NewerAppearance",
            "careerVersion > CareerSaveSchema",
            "appearanceVersion >",
            "return state == ProfileSaveWriteState::NewProfile",
        ):
            self.assertIn(marker,self.policy)
        self.assertIn("250000",self.tests)
        self.assertIn("add_latebrake_portable_research_test(ProfileSaveWriteGuard)",self.cmake)
        self.assertIn("interprocess",self.qa)

if __name__=="__main__":
    unittest.main()
