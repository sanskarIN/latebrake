"""Verify that career preview and launch consume the same read-only target."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CareerMenuSelectionBindingTests(unittest.TestCase):
    def test_shared_target(self):
        header = (ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h").read_text()
        mode = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text()
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text()
        self.assertIn("ResolveCareerMenuTarget(Career, Menu.Current(), CareerMenuSelection)", header)
        self.assertIn("const int32 Target = GetMenuCareerEventId();", mode)
        self.assertIn("if (StartCareerEvent(Target)) CloseMenuForRace();", mode)
        self.assertIn("? GetMenuCareerEventId() : Career.SelectedEvent();", mode)
        self.assertIn("BrowseCareerMenuTarget(Career, Page, Original)", mode)
        self.assertIn("Mode->GetMenuCareerEventId()", hud)
        self.assertIn("Mode->GetCareerEventName()", hud)

    def test_launch_transaction_unchanged(self):
        mode = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text()
        method = mode.split("bool ALatebrakeRaceGameMode::StartCareerEvent", 1)[1].split(
            "float ALatebrakeRaceGameMode::GetEventBestSeconds", 1)[0]
        self.assertLess(method.index("StageCareerLaunch("), method.index("SaveRecords();"))
        self.assertLess(method.index("SaveRecords();"), method.index("bCareerRace = true;"))
        self.assertIn("SelectionWrite.Rollback();", method)
        self.assertIn("ActivePractice = latebrake::QuickPracticePreset::None;", method)

    def test_registration(self):
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text()
        self.assertIn("add_latebrake_portable_research_test(CareerMenuSelection)", cmake)


if __name__ == "__main__":
    unittest.main()
