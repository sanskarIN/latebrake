"""Source-level quick race difficulty integration; not an Unreal build."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class RivalDifficultyBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.input = read("Config/DefaultInput.ini")
        cls.policy = read("Source/Latebrake/Public/AI/RivalDifficultyPreset.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_quick_race_browse_uses_existing_keyboard_gamepad_and_touch(self):
        self.assertIn('ActionName="CareerNext",Key=N', self.input)
        self.assertIn('ActionName="CareerNext",Key=Gamepad_DPad_Right', self.input)
        self.assertIn("Mode->CycleCareerEvent();", self.pawn)
        self.assertIn("latebrake::TouchControl::MenuBrowse", self.pawn)
        cycle = self.mode.split("void ALatebrakeRaceGameMode::CycleCareerEvent()", 1)[1].split(
            "void ALatebrakeRaceGameMode::StartNextAvailableCareerEvent()", 1)[0]
        self.assertIn("Menu.Current() == latebrake::MenuPage::QuickRace", cycle)
        self.assertIn("NextQuickRaceMenuChoice(", cycle)
        self.assertIn("QuickRaceDifficultySelection = Next.Difficulty;", cycle)
        self.assertIn("return;", cycle)
        self.assertIn("const int32 Original = CareerMenuSelection;", cycle)
        quick_menu = self.hud.split("case latebrake::MenuPage::QuickRace:", 1)[1].split(
            "case latebrake::MenuPage::Career:", 1)[0]
        self.assertIn("GetQuickRaceDifficultySelection()", quick_menu)
        self.assertIn("RivalDifficultyName(", quick_menu)
        self.assertIn("BROWSE/N", quick_menu)

    def test_selected_tier_is_latched_only_at_new_race_and_never_in_career(self):
        self.assertIn('#include "AI/RivalDifficultyPreset.h"', self.mode_h)
        self.assertIn("QuickRaceDifficultySelection", self.mode_h)
        self.assertIn("ActiveRivalDifficulty", self.mode_h)
        self.assertIn("GetActiveRivalDifficulty() const", self.mode_h)
        self.assertIn("EffectiveRivalDifficulty(", self.mode)
        restart = self.mode.split("void ALatebrakeRaceGameMode::RestartRace()", 1)[1].split(
            "void ALatebrakeRaceGameMode::RecoverPlayer(", 1)[0]
        self.assertLess(restart.index("ReplayWrite.Commit();"),
                        restart.index("ActiveRivalDifficulty = latebrake::EffectiveRivalDifficulty("))
        self.assertIn("bCareerRace", restart)
        self.assertIn("Mode->GetActiveRivalDifficulty()", self.rival)
        self.assertIn("latebrake::ApplyRivalDifficulty(", self.rival)

    def test_safety_governors_still_run_after_difficulty_selection(self):
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        for before, after in (
            ("AnticipateRivalCorner(", "ApplyRivalDifficulty("),
            ("ApplyRivalDifficulty(", "GripPace.Apply("),
            ("ApplyRivalDifficulty(", "YieldRivalToPlayer("),
            ("ApplyRivalDifficulty(", "ModerateRivalEngineHeatPace("),
            ("ApplyRivalDifficulty(", "SpinThrottleGuard.Apply("),
            ("ApplyRivalDifficulty(", "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)"),
        ):
            self.assertLess(tick.index(before), tick.index(after))
        for forbidden in (
            "PassCheckpoint(", "EarnCredits(", "SaveRecords(",
            "RaceProgress", "SetActorLocation(", "AddForce(",
            "UCLASS(", "UPROPERTY(", "SetAutonomousControls("
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("RivalDifficultyPresetTests.cpp", self.cmake)
        self.assertIn("NAME RivalDifficultyPreset", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)


if __name__ == "__main__":
    unittest.main()
