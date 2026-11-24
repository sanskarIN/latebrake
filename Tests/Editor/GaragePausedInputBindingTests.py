"""Unreal source wiring regression for the four original Garage hotkeys.

This verifies pause input delivery and the authoritative visible-page guards.
It does not pretend to run Unreal input dispatch, SaveGame or Win64 packaging.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def method(source: str, name: str) -> str:
    pattern = (
        rf"void ALatebrakeRaceGameMode::{name}\(\)\s*\{{"
        rf"(.*?)(?=\nvoid ALatebrakeRaceGameMode::|\Z)"
    )
    match = re.search(pattern, source, re.S)
    if match is None:
        raise AssertionError(f"Could not isolate {name}")
    return match.group(1)


class GaragePausedInputBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.ini = read("Config/DefaultInput.ini")
        cls.purchase_tests = read(
            "Tests/Editor/GaragePurchaseConfirmationBindingTests.py"
        )

    def test_each_original_garage_key_dispatches_during_pause(self):
        controls = (
            ("GarageNextVehicle", "NextGarageVehicle", "GarageNext", "E"),
            ("GarageUnlockVehicle", "UnlockGarageVehicle", "GarageUnlock", "Q"),
            ("GarageEngineUpgrade", "UpgradeGarageEngine", "GarageEngine", "U"),
            ("GarageGripUpgrade", "UpgradeGarageGrip", "GarageGrip", "I"),
        )
        for action, callback, binding, key in controls:
            with self.subTest(action=action):
                self.assertEqual(
                    self.ini.count(
                        f'+ActionMappings=(ActionName="{action}",Key={key})'
                    ), 1
                )
                pattern = (
                    rf"FInputActionBinding& {binding} = "
                    rf"PlayerInputComponent->BindAction\(\s*"
                    rf'TEXT\("{action}"\), IE_Pressed, this, '
                    rf"&ALatebrakeCarPawn::{callback}\);\s*"
                    rf"{binding}\.bExecuteWhenPaused = true;"
                )
                self.assertEqual(
                    len(re.findall(pattern, self.pawn)),
                    1,
                    f"{action} must be bound once and allowed during pause",
                )
        # Do not change driving axis bindings or enable gameplay actions
        # (throttle, brake, nitro, reverse) while the menu is paused.
        for action in ("Throttle", "Steer"):
            self.assertIn(f'BindAxis(TEXT("{action}")', self.pawn)

    def test_every_selection_and_purchase_requires_visible_garage(self):
        for name in (
            "GarageNextVehicle",
            "GarageUnlockNext",
            "GarageUpgradeEngine",
            "GarageUpgradeGrip",
        ):
            with self.subTest(name=name):
                body = method(self.mode, name)
                gate = (
                    "if (!Menu.IsOpen() || "
                    "Menu.Current() != latebrake::MenuPage::Garage)"
                )
                self.assertEqual(body.count(gate), 1)
                access_at = body.index(gate)
                career_at = body.index("CanEditGarageDuringRace(")
                # Each context gate must precede the first authoritative
                # vehicle/economy operation; retaining the legacy career
                # rejection before page checking is permitted.
                mutation_at = body.index("    const int32 ", access_at)
                self.assertLess(access_at, mutation_at)
                self.assertLess(career_at, mutation_at)
                # No selection, unlock, upgrade, preview approval, or
                # profile write may occur before the visible-page guard.
                before = body[:access_at]
                for forbidden in (
                    "Garage.Select(", "Garage.Unlock(", "Garage.Upgrade(",
                    "GaragePurchaseConfirm.ConfirmOrStage(",
                    "ProfileWriteCheckpoint SavedProfile(",
                    "SaveRecords()", "ApplyGarageToPlayer()",
                ):
                    self.assertNotIn(forbidden, before)
                denial = body[access_at:mutation_at]
                self.assertIn("GarageFeedback =", denial)
                self.assertIn("return;", denial)
                if name != "GarageNextVehicle":
                    self.assertIn("GaragePurchaseConfirm.Reset();", denial)
                else:
                    self.assertIn(
                        "GaragePurchaseConfirm.Reset();",
                        body[:access_at],
                    )
        # Static contract for existing Q-twice approval remains aligned.
        self.assertIn(
            '"!Menu.IsOpen() || Menu.Current() != latebrake::MenuPage::Garage"',
            self.purchase_tests,
        )

    def test_ctrl_slot_and_direct_selection_remain_menu_only(self):
        slot = method(self.mode, "GarageSelectVehicle")
        self.assertIn("Menu.IsOpen()", slot)
        self.assertIn("latebrake::MenuPage::Garage", slot)
        self.assertIn("PreviewGarageQuickSelect(", slot)
        self.assertIn("GarageQuickSelectStatus::NotOwned", slot)
        self.assertIn("CommitGarageVehicleSelection(", slot)
        pawn = self.pawn.split(
            "void ALatebrakeCarPawn::SelectGarageSlot(", 1
        )[1].split("void ALatebrakeCarPawn::GarageSlot1()", 1)[0]
        self.assertIn("Mode->IsMenuOpen()", pawn)
        self.assertIn("Mode->GetMenuPage() == latebrake::MenuPage::Garage", pawn)

    def test_pause_navigation_keeps_existing_escape_and_tab(self):
        for action, key in (("Pause", "Escape"), ("MenuToggle", "Tab")):
            self.assertIn(
                f'+ActionMappings=(ActionName="{action}",Key={key})',
                self.ini,
            )
        self.assertIn("PauseAction.bExecuteWhenPaused = true;", self.pawn)
        self.assertIn("OpenMenu.bExecuteWhenPaused = true;", self.pawn)


if __name__ == "__main__":
    unittest.main()
