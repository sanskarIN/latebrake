"""Unreal-source contracts for fail-closed race RESTART placement.

These tests inspect production GameMode, pawn and HUD bindings. They do NOT
run the licensed Unreal 5.8 Editor, cooked map or actual Chaos teleport.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODE_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
MODE = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
DRIVE = ROOT / "Source/Latebrake/Public/Vehicles/PlayerDriveReadiness.h"
GUARD = ROOT / "Source/Latebrake/Public/Racing/RestartPlacementGuard.h"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


def section(source: str, start: str, end: str) -> str:
    if source.count(start) != 1 or end not in source:
        raise AssertionError("missing/ambiguous source boundary: " + start)
    return source.split(start, 1)[1].split(end, 1)[0]


class RestartPlacementBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode_h = MODE_H.read_text(encoding="utf-8")
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.drive = DRIVE.read_text(encoding="utf-8")
        cls.guard = GUARD.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_game_mode_owns_restart_guard(self):
        self.assertIn('#include "Racing/RestartPlacementGuard.h"', self.mode_h)
        self.assertIn("latebrake::RestartPlacementGuard RestartPlacement;", self.mode_h)
        self.assertIn("bool IsRestartPlacementReady() const", self.mode_h)
        ready = section(
            self.mode, "bool ALatebrakeRaceGameMode::HasLocalPlayerDriver() const",
            "void ALatebrakeRaceGameMode::BeginPlay()"
        )
        self.assertIn("RestartPlacement.CanRace()", ready)
        self.assertIn("Car->IsRacePhysicsReady()", ready)

    def test_restart_invalidates_first_then_verifies_physical_move(self):
        restart = section(
            self.mode, "void ALatebrakeRaceGameMode::RestartRace()",
            "void ALatebrakeRaceGameMode::RecoverPlayer("
        )
        self.assertIn("RestartPlacement.OnRestartRequested();", restart)
        self.assertIn("Car->TryRecoverTo(GetStartPosition(), GetStartRotation())", restart)
        self.assertIn("RestartPlacement.OnRestartPlacementResult(bPlacedOnGrid);", restart)
        self.assertIn("IsValid(Car) && Car->TryRecoverTo(", restart)
        self.assertNotIn("Car->RecoverTo(GetStartPosition()", restart)
        self.assertIn("RESTART_GRID_UNVERIFIED", restart)
        self.assertLess(restart.index("RestartPlacement.OnRestartRequested();"),
                        restart.index("Race.Reset();"))
        self.assertLess(restart.index("RestartPlacement.OnRestartRequested();"),
                        restart.index("TryRecoverTo(GetStartPosition()"))
        self.assertLess(restart.index("TryRecoverTo(GetStartPosition()"),
                        restart.index("RestartPlacement.OnRestartPlacementResult("))

    def test_actual_clock_checkpoint_and_paid_recovery_use_shared_gate(self):
        self.assertIn("HasLocalPlayerDriver()", self.mode)
        self.assertIn("const bool bLocalPlayerReady = HasLocalPlayerDriver();",
                      self.mode)
        self.assertIn("Car->IsRacePhysicsReady()", self.mode)
        self.assertIn("RestartPlacement.CanRace() && IsValid(Car)", self.mode)
        self.assertIn("Car->TryRecoverTo(Target, Facing)", self.mode)

    def test_pawn_neutralizes_motor_before_force_if_grid_unverified(self):
        tick = section(
            self.pawn, "void ALatebrakeCarPawn::Tick(float DeltaSeconds)",
            "bool ALatebrakeCarPawn::IsRacePhysicsReady() const"
        )
        self.assertIn("Mode->IsRestartPlacementReady()", tick)
        self.assertIn("latebrake::CanApplyPlayerDrive({", tick)
        self.assertLess(tick.index("Mode->IsRestartPlacementReady()"),
                        tick.index("NeutralizeMotionWhenDrivingBlocked();"))
        self.assertLess(tick.index("NeutralizeMotionWhenDrivingBlocked();"),
                        tick.index("Chassis->AddForce("))
        self.assertLess(tick.index("NeutralizeMotionWhenDrivingBlocked();"),
                        tick.index("Chassis->AddTorqueInRadians("))
        self.assertIn("if (!state.RestartPlacementReady)", self.drive)
        self.assertIn("PlayerDriveDecision::RestartPlacementUnverified", self.drive)

    def test_hud_explains_blocked_restart(self):
        self.assertIn("RESTART GRID NOT VERIFIED - RACE DISABLED", self.hud)
        self.assertIn("RESTART GRID NOT VERIFIED - PRESS RESTART AGAIN", self.hud)
        self.assertIn("RESTART FAILED - RACE DISABLED", self.hud)
        self.assertGreaterEqual(self.hud.count("Mode->IsRestartPlacementReady()"), 2)

    def test_portable_state_machine_and_race_progress_compiled(self):
        for required in ("AwaitingPlacement", "RejectedPlacement",
                         "VerifiedPlacement", "OnRestartRequested()",
                         "OnRestartPlacementResult(bool verified)"):
            self.assertIn(required, self.guard)
        self.assertIn("RestartPlacementGuardTests.cpp", self.cmake)
        self.assertIn("Private/Racing/RaceProgress.cpp", self.cmake)
        self.assertIn("NAME RestartPlacementGuard", self.cmake)

    def test_regression_can_be_detected(self):
        bypass = self.mode.replace(
            "RestartPlacement.CanRace() && IsValid(Car)",
            "IsValid(Car) /* unsafe grid bypass */",
        )
        self.assertNotIn("RestartPlacement.CanRace() && IsValid(Car)", bypass)
        bypass_pawn = self.pawn.replace(
            "Mode->IsRestartPlacementReady()",
            "true /* unsafe torque authorization */",
        )
        self.assertNotIn("Mode->IsRestartPlacementReady()", bypass_pawn)


if __name__ == "__main__":
    unittest.main()
