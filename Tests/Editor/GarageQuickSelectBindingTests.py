"""Static wiring checks for native six-car F-key garage quick-select.

These checks verify the *source* remains connected, not actual UE input
dispatch, win64 packaging, graphical focus, or a successful SaveGame write.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]

def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageQuickSelectBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ini = read("Config/DefaultInput.ini")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.mode_header = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn_header = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy = read("Source/Latebrake/Public/Garage/GarageQuickSelect.h")
        cls.tests = read("Tests/Standalone/GarageQuickSelectTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_each_of_six_keys_has_a_single_paused_input_handler(self):
        for slot in range(1, 7):
            with self.subTest(slot=slot):
                mapping = (
                    f'+ActionMappings=(ActionName="GarageSlot{slot}",Key=F{slot})')
                self.assertEqual(self.ini.count(mapping), 1)
                method = f'GarageSlot{slot}'
                self.assertIn(f'void {method}();', self.pawn_header)
                self.assertIn(f'ALatebrakeCarPawn::{method}()', self.pawn)
                self.assertIn(f'SelectGarageSlot({slot-1});', self.pawn)
                bind = re.search(
                    rf'FInputActionBinding& Slot{slot} = '
                    rf'PlayerInputComponent->BindAction\(\s*'
                    rf'TEXT\("GarageSlot{slot}"\), IE_Pressed, this, '
                    rf'&ALatebrakeCarPawn::GarageSlot{slot}\);',
                    self.pawn)
                self.assertIsNotNone(bind)
                self.assertIn(f'Slot{slot}.bExecuteWhenPaused = true;', self.pawn)

        # Existing race keys cannot be overwritten by the new menu shortcuts.
        for action, key in (
            ("ArcadePreset", "One"), ("SimulationPreset", "Two"),
            ("CircuitEvent", "Three"), ("TimeAttackEvent", "Four")
        ):
            self.assertIn(
                f'+ActionMappings=(ActionName="{action}",Key={key})', self.ini)

    def test_input_double_gated_by_pawn_and_mode(self):
        pawn_slot = self.pawn.split(
            "void ALatebrakeCarPawn::SelectGarageSlot(", 1
        )[1].split("void ALatebrakeCarPawn::GarageSlot1()", 1)[0]
        self.assertIn("Mode->IsMenuOpen()", pawn_slot)
        self.assertIn("Mode->GetMenuPage() == latebrake::MenuPage::Garage", pawn_slot)
        self.assertIn("Mode->GarageSelectVehicle(VehicleId)", pawn_slot)

        mode_slot = self.mode.split(
            "void ALatebrakeRaceGameMode::GarageSelectVehicle(", 1
        )[1].split("void ALatebrakeRaceGameMode::GarageUnlockNext()", 1)[0]
        for guard in ("Menu.IsOpen()", "Menu.Current()", 
                      "CanEditGarageDuringRace", "PreviewGarageQuickSelect",
                      "GarageQuickSelectStatus::NotOwned",
                      "GarageQuickSelectStatus::AlreadyEquipped"):
            self.assertIn(guard, mode_slot)
        self.assertEqual(mode_slot.count("CommitGarageVehicleSelection("), 1)
        self.assertNotIn("Garage.Unlock(", mode_slot)
        self.assertNotIn("Garage.Upgrade(", mode_slot)
        self.assertNotIn("Garage.EarnCredits(", mode_slot)

    def test_both_paths_share_authoritative_save_rollback(self):
        self.assertIn("void GarageSelectVehicle(int32 VehicleId);", self.mode_header)
        self.assertIn("void CommitGarageVehicleSelection(int32 VehicleId);",
                      self.mode_header)
        shared = self.mode.split(
            "void ALatebrakeRaceGameMode::CommitGarageVehicleSelection(", 1
        )[1].split("void ALatebrakeRaceGameMode::GarageNextVehicle()", 1)[0]
        for exact in ("ProfileWriteCheckpoint", "Garage.Select(VehicleId)",
                      "ApplyGarageToPlayer();", "SaveRecords();",
                      "SavedProfile.Rollback();", "SavedProfile.Commit();"):
            self.assertIn(exact, shared)
        self.assertNotIn("Garage.Unlock(", shared)
        next_vehicle = self.mode.split(
            "void ALatebrakeRaceGameMode::GarageNextVehicle()", 1
        )[1].split("void ALatebrakeRaceGameMode::GarageSelectVehicle(", 1)[0]
        self.assertIn("CommitGarageVehicleSelection(Next)", next_vehicle)

    def test_live_garage_menu_is_desktop_keyboard_context_aware(self):
        page = self.hud.split("case latebrake::MenuPage::Garage:", 1)[1].split(
            "case latebrake::MenuPage::Results:", 1)[0]
        self.assertIn("if (!bTouchHUD)", page)
        self.assertIn("F1-F6: EQUIP OWNED CARS DIRECTLY", page)
        self.assertIn('FString::Printf(TEXT("F%d")', page)
        self.assertIn('FString::Printf(TEXT("%d.")', page)
        self.assertIn("Mode->GetGarageFeedback()", page)
        self.assertNotIn("Garage.Select(", page)

    def test_pure_policy_in_registered_portable_tests(self):
        self.assertIn("GarageQuickSelect", self.cmake)
        self.assertIn('#include "Garage/GarageQuickSelect.h"', self.tests)
        self.assertIn("for (int mask = -1; mask <= 127; ++mask)", self.tests)
        self.assertIn("observed.Load(saved);", self.policy)
        for mutation in ("observed.Select(", "observed.Unlock(",
                         "observed.Upgrade(", "observed.EarnCredits("):
            self.assertNotIn(mutation, self.policy)


if __name__ == "__main__":
    unittest.main()
