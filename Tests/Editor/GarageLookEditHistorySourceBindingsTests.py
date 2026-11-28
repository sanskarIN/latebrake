"""Contract tests for the transactional, menu-only Unreal Garage history."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT/path).read_text(encoding="utf-8")


class GarageLookEditHistoryBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.mode_header=read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn_header=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.config=read("Config/DefaultInput.ini")
        cls.hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")

    def test_undo_redo_are_guards_and_save_transactions_not_just_preview(self):
        self.assertIn('"Garage/GarageLookEditHistory.h"',self.mode_header)
        self.assertIn("latebrake::GarageLookEditHistory GarageLookHistory;",self.mode_header)
        block=self.mode.split("void ALatebrakeRaceGameMode::CommitGarageAppearanceHistory(",1)[1].split(
            "void ALatebrakeRaceGameMode::GarageUnlockNext()",1)[0]
        for marker in ("RejectGarageMutationIfBlocked()",
                       "GaragePurchaseConfirm.Reset()",
                       "CanCustomizeGarage()",
                       "GarageLookHistory.Preview(",
                       "Proposal.ProposedLook.Paint",
                       "Proposal.ProposedLook.Stripe",
                       "Proposal.ProposedLook.Wheels",
                       "ApplyGarageToPlayer();", "SaveRecords();",
                       "if (bProgressSaveFailed)", "GarageLooks=Before;",
                       "GarageLookHistory.Commit(Proposal",
                       "SyncGarageShowroomPreview();"):
            self.assertIn(marker,block)
        self.assertLess(block.index("SaveRecords();"),
                        block.index("GarageLookHistory.Commit(Proposal"),
                        "history may change only after successful save")
        self.assertLess(block.index("if (bProgressSaveFailed)"),
                        block.index("GarageLookHistory.Commit(Proposal"))
        self.assertIn("GarageLookHistory.Reset();",self.mode)

    def test_normal_style_actions_record_only_successful_edits(self):
        block=self.mode.split("void ALatebrakeRaceGameMode::GarageCycleAppearance(",1)[1].split(
            "void ALatebrakeRaceGameMode::GarageChooseAppearance(",1)[0]
        self.assertIn("GarageLookHistory.RecordCommittedChange(",block)
        self.assertLess(block.index("SaveRecords();"),
                        block.index("GarageLookHistory.RecordCommittedChange("))
        self.assertLess(block.index("if (bProgressSaveFailed)"),
                        block.index("GarageLookHistory.RecordCommittedChange("))
        self.assertIn("GarageLooks = Before;",block)

    def test_paused_garage_only_keyboard_dispatch(self):
        for action,key,method in (
            ("GarageUndoLook","Z","UndoGarageLook"),
            ("GarageRedoLook","Y","RedoGarageLook"),
        ):
            with self.subTest(action=action):
                self.assertEqual(self.config.count(
                    f'+ActionMappings=(ActionName="{action}",Key={key},bCtrl=True)'),1)
                self.assertIn(f'TEXT("{action}"), IE_Pressed, this, &ALatebrakeCarPawn::{method}',
                              self.pawn)
                self.assertIn(f"{method}();",self.pawn_header)
        self.assertIn("UndoLookKey.bExecuteWhenPaused = true;",self.pawn)
        self.assertIn("RedoLookKey.bExecuteWhenPaused = true;",self.pawn)
        for marker in ("CanConsumeGarageShortcut(","ClearDriverInputs();",
                       "Mode->GarageUndoAppearance();", "Mode->GarageRedoAppearance();"):
            self.assertIn(marker,self.pawn)
        self.assertIn("CTRL+Z UNDO LOOK / CTRL+Y REDO LOOK",self.hud)

    def test_read_only_policy_is_cross_platform_compiled(self):
        self.assertIn("GarageLookEditHistoryTests.cpp",self.cmake)
        self.assertIn("NAME GarageLookEditHistory",self.cmake)
        policy=read("Source/Latebrake/Public/Garage/GarageLookEditHistory.h")
        for prohibited in ("SaveRecords(","EarnCredits(", "Garage.Unlock(",
                           "SetActorTransform(","FPlatformTime"):
            self.assertNotIn(prohibited,policy)


if __name__=="__main__":
    unittest.main()
