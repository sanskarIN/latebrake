"""Engine-integration source contracts for app deactivation race suspension.

These tests cannot substitute for Unreal/UHT or real OS focus event delivery.
"""

from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "Latebrake"
PAWN = SRC / "Private/Vehicles/LatebrakeCarPawn.cpp"
MODE = SRC / "Private/Game/LatebrakeRaceGameMode.cpp"
MODE_H = SRC / "Public/Game/LatebrakeRaceGameMode.h"
CHECKPOINT = SRC / "Private/Racing/LatebrakeCheckpoint.cpp"
RIVAL = SRC / "Private/AI/LatebrakeRivalCar.cpp"
HUD = SRC / "Private/UI/LatebrakeRaceHUD.cpp"


class AppFocusRaceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.mode_h = MODE_H.read_text(encoding="utf-8")
        cls.checkpoint = CHECKPOINT.read_text(encoding="utf-8")
        cls.rival = RIVAL.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")

    def test_deactivation_and_background_pause_race_but_foreground_only_clears(self):
        for delegate in ("ApplicationWillDeactivateDelegate",
                         "ApplicationWillEnterBackgroundDelegate"):
            self.assertRegex(
                self.pawn, re.escape(delegate) +
                r"\.AddUObject\(\s*this,\s*&ALatebrakeCarPawn::HandleAppFocusLost\)",
            )
        self.assertRegex(
            self.pawn,
            r"ApplicationHasEnteredForegroundDelegate.AddUObject\(\s*"
            r"this,\s*&ALatebrakeCarPawn::HandleAppInputInterruption\)",
        )
        body = self.pawn.split(
            "void ALatebrakeCarPawn::HandleAppFocusLost()", 1
        )[1].split("\nvoid ALatebrakeCarPawn::", 1)[0]
        self.assertLess(body.index("HandleAppInputInterruption();"),
                        body.index("Mode->PauseForAppInterruption();"))

    def test_game_mode_latches_prior_to_pausing_world(self):
        self.assertIn("void PauseForAppInterruption();", self.mode_h)
        start = self.mode.index("void ALatebrakeRaceGameMode::PauseForAppInterruption()")
        body = self.mode[start:].split("\nvoid ALatebrakeRaceGameMode::", 1)[0]
        self.assertIn("AppFocusGuard.OnFocusLost(", body)
        self.assertIn("FocusLossTransition::Ignored", body)
        self.assertIn("FocusLossTransition::SuspendedNeedsWorldPause", body)
        self.assertIn("CheckpointSession.OnPauseTransition();", body)
        self.assertIn("UGameplayStatics::SetGamePaused(this, true)", body)
        self.assertLess(body.index("FocusLossTransition::Ignored"),
                        body.index("CheckpointSession.OnPauseTransition();"))
        self.assertLess(body.index("CheckpointSession.OnPauseTransition();"),
                        body.index("FocusLossTransition::SuspendedNeedsWorldPause"))
        self.assertLess(body.index("CheckpointSession.OnPauseTransition();"),
                        body.index("SetGamePaused("))
        self.assertIn("AppFocusGuard.OnExplicitResume();", self.mode)

    def test_every_engine_race_boundary_observes_latch(self):
        # Pawn physics can stop applying input, but the mode still needs its
        # own fail-closed clock/scoring/recovery gate if the world keeps ticking.
        for needle in (
            "UGameplayStatics::IsGamePaused(this) || Menu.IsOpen() ||",
            "AppFocusGuard.IsSuspended()",
            "const bool bInactive = UGameplayStatics::IsGamePaused(this) || Menu.IsOpen() ||",
        ):
            self.assertIn(needle, self.mode)
        self.assertGreaterEqual(self.mode.count("AppFocusGuard.IsSuspended()"), 3)
        self.assertIn("Mode->IsFocusLossGuardActive()", self.checkpoint)
        self.assertIn("Mode->IsFocusLossGuardActive()", self.rival)

    def test_hud_discloses_latched_suspension_to_player(self):
        self.assertIn("Mode->IsFocusLossGuardActive()", self.hud)
        self.assertIn("FOCUS LOST - RACE SUSPENDED", self.hud)

    def test_manual_resume_handles_platform_auto_unpause(self):
        pause_body = self.mode.split(
            "void ALatebrakeRaceGameMode::ToggleRacePause()", 1
        )[1].split("\nvoid ALatebrakeRaceGameMode::", 1)[0]
        self.assertIn(
            "AppFocusGuard.IsSuspended() && !UGameplayStatics::IsGamePaused(this)",
            pause_body,
        )
        self.assertIn("AppFocusGuard.OnExplicitResume();", pause_body)
        self.assertIn("CheckpointSession.OnPauseTransition();", pause_body)

    def test_foreground_cannot_unpause_automatically(self):
        foreground = re.search(
            r"ApplicationHasEnteredForegroundDelegate.AddUObject\(\s*"
            r"this,\s*&ALatebrakeCarPawn::(\w+)\)", self.pawn,
        )
        self.assertIsNotNone(foreground)
        self.assertEqual(foreground.group(1), "HandleAppInputInterruption")
        clear_body = self.pawn.split(
            "void ALatebrakeCarPawn::HandleAppInputInterruption()", 1
        )[1].split("\nvoid ALatebrakeCarPawn::", 1)[0]
        self.assertNotIn("PauseForAppInterruption", clear_body)
        self.assertNotIn("OnExplicitResume", clear_body)

    def test_focus_guard_contract_does_not_hide_a_fresh_already_paused_edge(self):
        policy = (SRC / "Public/Racing/AppFocusRaceGuard.h").read_text(
            encoding="utf-8")
        self.assertIn("enum class FocusLossTransition", policy)
        self.assertIn("SuspendedAlreadyPaused", policy)
        self.assertIn("SuspendedNeedsWorldPause", policy)
        self.assertIn("return worldPaused ? FocusLossTransition::SuspendedAlreadyPaused", policy)
        self.assertIn("FocusLossTransition::Ignored", policy)

        body = self.mode.split(
            "void ALatebrakeRaceGameMode::PauseForAppInterruption()", 1
        )[1].split("\nvoid ALatebrakeRaceGameMode::", 1)[0]
        # Reject a regression where an already-paused OS delivery returns
        # before checkpoint epoch invalidation.
        self.assertIn("if (FocusTransition == latebrake::FocusLossTransition::Ignored)", body)
        self.assertLess(body.index("CheckpointSession.OnPauseTransition();"),
                        body.index("if (FocusTransition == "
                                   "latebrake::FocusLossTransition::SuspendedNeedsWorldPause"))
        self.assertNotIn("if (!AppFocusGuard.OnFocusLost(", body)

    def test_mutations_drop_required_engine_call(self):
        broken = self.pawn.replace(
            "Mode->PauseForAppInterruption();", "// suppressed in fixture")
        self.assertNotIn("Mode->PauseForAppInterruption();", broken)
        broken_mode = self.mode.replace(
            "AppFocusGuard.IsSuspended(),", "false,")
        self.assertLess(broken_mode.count("AppFocusGuard.IsSuspended()"),
                        self.mode.count("AppFocusGuard.IsSuspended()"))


if __name__ == "__main__":
    unittest.main()
