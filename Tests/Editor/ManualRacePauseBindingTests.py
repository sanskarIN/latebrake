"""Source-level Unreal integration contracts for manual PAUSE fallback.

This checks real game C++ callsites but cannot prove UE 5.8/UHT/Chaos
compilation, a genuine packaged world-pause failure, or Android touch events.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2] / "Source/Latebrake"
MODE = ROOT / "Private/Game/LatebrakeRaceGameMode.cpp"
MODE_H = ROOT / "Public/Game/LatebrakeRaceGameMode.h"
PAWN = ROOT / "Private/Vehicles/LatebrakeCarPawn.cpp"
RIVAL = ROOT / "Private/AI/LatebrakeRivalCar.cpp"
CHECKPOINT = ROOT / "Private/Racing/LatebrakeCheckpoint.cpp"
HUD = ROOT / "Private/UI/LatebrakeRaceHUD.cpp"


def body(source: str, owner: str, method: str) -> str:
    prefix = f"void {owner}::{method}("
    if source.count(prefix) != 1:
        raise AssertionError("method absent/ambiguous: " + prefix)
    return source.split(prefix, 1)[1].split("\nvoid " + owner + "::", 1)[0]


class ManualRacePauseBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.mode_h = MODE_H.read_text(encoding="utf-8")
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.rival = RIVAL.read_text(encoding="utf-8")
        cls.checkpoint = CHECKPOINT.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")

    def test_manual_pause_latches_before_fallible_world_call(self):
        pause = body(self.mode, "ALatebrakeRaceGameMode", "ToggleRacePause")
        self.assertIn("ManualPause.OnToggleRequested(bWasWorldPaused)", pause)
        self.assertIn("UGameplayStatics::SetGamePaused(this, bRequestWorldPause)", pause)
        self.assertLess(pause.index("ManualPause.OnToggleRequested("),
                        pause.index("UGameplayStatics::SetGamePaused("))
        self.assertIn("bRequestWorldPause || bWasManualPause ||", pause)
        self.assertIn("CheckpointSession.OnPauseTransition();", pause)
        self.assertIn("ManualPause.OnExplicitResume();", pause)
        self.assertIn("AppFocusGuard.OnExplicitResume();", pause)
        self.assertIn("manual world pause failed", pause)

    def test_all_authoritative_race_boundaries_observe_manual_pause(self):
        self.assertIn('#include "Racing/ManualRacePauseGuard.h"', self.mode_h)
        self.assertIn("latebrake::ManualRacePauseGuard ManualPause;", self.mode_h)
        self.assertIn("bool IsManualRacePauseActive() const", self.mode_h)
        tick = body(self.mode, "ALatebrakeRaceGameMode", "Tick")
        scoring = body(self.mode, "ALatebrakeRaceGameMode", "RegisterCheckpoint")
        recovery = body(self.mode, "ALatebrakeRaceGameMode", "RecoverPlayer")
        for label, code in (("clock", tick), ("scoring", scoring), ("recovery", recovery)):
            with self.subTest(boundary=label):
                self.assertIn("ManualPause.IsSuspended()", code)
        self.assertIn("ManualPause.IsSuspended()", self.mode)

    def test_unreal_checkpoint_and_rival_observe_fallback_gate(self):
        self.assertIn("Mode->IsManualRacePauseActive()", self.checkpoint)
        self.assertLess(self.checkpoint.index("Mode->IsManualRacePauseActive()"),
                        self.checkpoint.index("MotionTrace.SampleDetailed("))
        self.assertLess(self.checkpoint.index("Mode->IsManualRacePauseActive()"),
                        self.checkpoint.index("Mode->RegisterCheckpoint("))
        tick = body(self.rival, "ALatebrakeRivalCar", "Tick")
        self.assertIn("Mode->IsManualRacePauseActive()", tick)
        self.assertLess(tick.index("Mode->IsManualRacePauseActive()"),
                        tick.index("SetAutonomousControls(0.f, 0.f, true)"))

    def test_player_never_applies_force_when_manual_pause_requested(self):
        tick = body(self.pawn, "ALatebrakeCarPawn", "Tick")
        self.assertIn("Mode->IsManualRacePauseActive()", tick)
        self.assertLess(tick.index("Mode->IsManualRacePauseActive()"),
                        tick.index("NeutralizeMotionWhenDrivingBlocked();"))
        self.assertLess(tick.index("NeutralizeMotionWhenDrivingBlocked();"),
                        tick.index("Chassis->AddForce("))
        self.assertLess(tick.index("NeutralizeMotionWhenDrivingBlocked();"),
                        tick.index("Chassis->AddTorqueInRadians("))
        touch = body(self.pawn, "ALatebrakeCarPawn", "ApplyTouch")
        self.assertIn("(Mode && Mode->IsManualRacePauseActive())", touch)
        self.assertIn("if (bAllowAcquire && Region == latebrake::TouchControl::Pause)",
                      touch)
        self.assertLess(touch.index("Mode->IsManualRacePauseActive()"),
                        touch.index("TouchDrive.Press("))

    def test_manual_pause_visible_without_engine_world_paused(self):
        self.assertIn('TEXT("PAUSED  -  ESC TO RESUME")', self.hud)
        self.assertGreaterEqual(
            self.hud.count("Mode->IsManualRacePauseActive()"), 2,
            "fallback pause must show in status AND pressed touch pad")

    def test_explicit_menu_return_and_restart_clear_fallback(self):
        for method in ("CloseMenuForRace", "ToggleMenu", "RestartRace"):
            with self.subTest(method=method):
                self.assertIn("ManualPause.OnExplicitResume();",
                              body(self.mode, "ALatebrakeRaceGameMode", method))

    def test_policy_regression_mutations_fail_contract(self):
        broken = self.mode.replace(
            "AppFocusGuard.IsSuspended() || ManualPause.IsSuspended(),",
            "AppFocusGuard.IsSuspended(),",
        )
        self.assertNotIn("ManualPause.IsSuspended()",
                         body(broken, "ALatebrakeRaceGameMode", "Tick"))
        no_player_gate = self.pawn.replace(
            "Mode->IsManualRacePauseActive(),",
            "false,",
        )
        self.assertNotIn("Mode->IsManualRacePauseActive()",
                         body(no_player_gate, "ALatebrakeCarPawn", "Tick"))


if __name__ == "__main__":
    unittest.main()
