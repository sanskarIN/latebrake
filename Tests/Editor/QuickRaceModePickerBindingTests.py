"""Source-bound actual Quick Race menu and mode wiring, not a UE runtime test."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class QuickRaceModePickerBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode_header = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.input = read("Config/DefaultInput.ini")
        cls.policy = read("Source/Latebrake/Public/Racing/QuickRaceModePicker.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_original_keyboard_gamepad_and_touch_browse_paths(self):
        for key in ("Key=N", "Key=Gamepad_DPad_Right"):
            self.assertIn('ActionName="CareerNext",' + key, self.input)
        self.assertIn("Mode->CycleCareerEvent();", self.pawn)
        self.assertIn("TouchControl::MenuBrowse", self.pawn)
        cycle = self.mode.split("void ALatebrakeRaceGameMode::CycleCareerEvent()", 1)[1].split(
            "void ALatebrakeRaceGameMode::StartNextAvailableCareerEvent()", 1)[0]
        self.assertIn("Menu.Current() == latebrake::MenuPage::QuickRace", cycle)
        self.assertIn("NextQuickRaceMenuChoice(", cycle)
        self.assertIn("QuickRaceDifficultySelection = Next.Difficulty;", cycle)
        self.assertIn("bQuickRaceSoloTimeAttackSelection = Next.SoloTimeAttack;", cycle)
        self.assertLess(cycle.index("return;"),
                        cycle.index("const int32 Original = CareerMenuSelection;"))

    def test_menu_launches_existing_qualified_single_lap_time_attack(self):
        activate = self.mode.split("void ALatebrakeRaceGameMode::MenuActivate()", 1)[1].split(
            "void ALatebrakeRaceGameMode::SelectRaceEvent(", 1)[0]
        self.assertIn("QuickRaceMenuEvent(", activate)
        self.assertIn("SelectedEvent != ChosenEvent", activate)
        self.assertIn("SelectRaceEvent(ChosenEvent);", activate)
        self.assertIn("RestartRace();", activate)
        self.assertIn("CloseMenuForRace();", activate)
        self.assertIn("Race = latebrake::RaceProgress(Definition.CheckpointCount, Definition.RequiredLaps);",
                      self.mode)
        self.assertIn("RefreshRival();", self.mode)
        self.assertIn("RivalDifficultyPreset ActiveRivalDifficulty", self.mode_header)
        self.assertIn("QuickRaceDifficultySelection, bCareerRace ||", self.mode)
        self.assertIn("SelectedEvent == latebrake::EventKind::TimeAttack", self.mode)
        self.assertIn("GetQuickRaceSoloTimeAttackSelection() const", self.mode_header)

    def test_quick_race_menu_explains_event_and_browse(self):
        page = self.hud.split("case latebrake::MenuPage::QuickRace:", 1)[1].split(
            "case latebrake::MenuPage::Career:", 1)[0]
        for phrase in ("GetQuickRaceSoloTimeAttackSelection()",
                       "GetQuickRaceDifficultySelection()",
                       "RivalDifficultyName(",
                       "SOLO TIME ATTACK", "NO RIVAL",
                       "BROWSE/N", "Career uses STANDARD"):
            self.assertIn(phrase, page)
        for forbidden in (
            "SaveRecords(", "AwardCredits(", "PassCheckpoint(",
            "RaceProgress", "SetActorLocation(", "AddForce(",
            "UCLASS(", "SetAutonomousControls("
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("NAME QuickRaceModePicker", self.cmake)
        self.assertIn("QuickRaceModePickerTests.cpp", self.cmake)


if __name__ == "__main__":
    unittest.main()
