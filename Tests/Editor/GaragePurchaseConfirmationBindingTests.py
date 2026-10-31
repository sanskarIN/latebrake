"""Source contract for two-step garage spending in the Unreal game mode.

This does not run UnrealHeaderTool, cook maps or prove saved-game behavior
on installed hardware. The portable C++ tests exercise the actual gate.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(name):
    return (ROOT / name).read_text(encoding="utf-8")


class GaragePurchaseConfirmationBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = source("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.header = source("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.policy = source("Source/Latebrake/Public/Garage/GaragePurchaseConfirmation.h")
        cls.tests = source("Tests/Standalone/GaragePurchaseConfirmationTests.cpp")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")
        cls.hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")

    def test_all_three_spend_calls_require_approval_before_mutating(self):
        methods = (
            ("GarageUnlockNext", "GarageUpgradeEngine", "Garage.Unlock(",
             "GaragePurchaseKind::UnlockVehicle"),
            ("GarageUpgradeEngine", "GarageUpgradeGrip",
             "Garage.Upgrade(latebrake::UpgradeKind::Engine)",
             "GaragePurchaseKind::UpgradeEngine"),
            ("GarageUpgradeGrip", "RefreshRival",
             "Garage.Upgrade(latebrake::UpgradeKind::Grip)",
             "GaragePurchaseKind::UpgradeGrip"),
        )
        for start, end, mutation, kind in methods:
            with self.subTest(start=start):
                body = self.mode.split(
                    f"void ALatebrakeRaceGameMode::{start}()", 1)[1].split(
                    f"void ALatebrakeRaceGameMode::{end}()", 1)[0]
                for marker in ("CanEditGarageDuringRace(",
                               "Menu.IsOpen() && Menu.Current() != latebrake::MenuPage::Garage",
                               "GaragePurchaseConfirm.ConfirmOrStage(",
                               "Garage.Snapshot()", "FPlatformTime::Seconds()",
                               "ProfileWriteCheckpoint SavedProfile(",
                               "SaveRecords()", "SavedProfile.Rollback()"):
                    self.assertIn(marker, body)
                self.assertIn(kind, body)
                self.assertLess(
                    body.index("GaragePurchaseConfirm.ConfirmOrStage("),
                    body.index(mutation))
                self.assertLess(
                    body.index("GaragePurchaseConfirm.ConfirmOrStage("),
                    body.index("ProfileWriteCheckpoint SavedProfile("))

    def test_session_page_and_selection_transitions_cancel_pending(self):
        self.assertIn('#include "Garage/GaragePurchaseConfirmation.h"',
                      self.header)
        self.assertIn("GaragePurchaseConfirmation GaragePurchaseConfirm;",
                      self.header)
        for method in ("RestartRace", "CloseMenuForRace", "ToggleMenu",
                       "MenuNext", "MenuPrevious", "MenuActivate",
                       "GarageNextVehicle"):
            section = self.mode.split(
                f"void ALatebrakeRaceGameMode::{method}()", 1)[1]
            self.assertIn("GaragePurchaseConfirm.Reset();",
                          section[:300], method)
        self.assertIn("Q/U/I press twice", self.hud)
        self.assertNotIn("GaragePurchaseConfirm", source(
            "Source/Latebrake/Public/Progression/LatebrakePlayerSave.h"))

    def test_policy_checks_snapshot_price_kind_and_monotonic_deadline(self):
        for marker in ("ConfirmWindowSeconds = 8.0", "SameSnapshot(",
                       "kind == Kind", "subject == Subject",
                       "price == Price", "elapsed >= 0.0",
                       "elapsed <= ConfirmWindowSeconds",
                       "std::isfinite(nowSeconds)", "Reset();",
                       "return true;"):
            self.assertIn(marker, self.policy)
        self.assertIn("10000", self.tests)
        self.assertIn("quiet_NaN", self.tests)
        self.assertIn("GaragePurchaseConfirmationTests.cpp", self.cmake)
        self.assertIn("NAME GaragePurchaseConfirmation", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)


if __name__ == "__main__":
    unittest.main()
