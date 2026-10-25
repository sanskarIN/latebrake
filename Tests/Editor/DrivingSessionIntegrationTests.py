"""Unreal source integration contract for measured per-race driving stats.

This does not compile UE 5.8 or verify actual Chaos vehicle behavior.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN_H = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
MODE = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
TESTS = ROOT / "Tests/Standalone/DrivingSessionStatsTests.cpp"
PROD = ROOT / "Source/Latebrake/Private/Racing/DrivingSessionStats.cpp"

class DrivingSessionIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.h = PAWN_H.read_text()
        cls.p = PAWN.read_text()
        cls.m = MODE.read_text()
        cls.hud = HUD.read_text()
        cls.cmake = CMAKE.read_text()
        cls.tests = TESTS.read_text()
        cls.prod = PROD.read_text()

    def test_live_pawn_measures_only_after_speed_check(self):
        self.assertIn("latebrake::DrivingSessionStats DrivingStats;", self.h)
        self.assertIn("GetDrivingStats() const", self.h)
        self.assertIn("void ResetDrivingStats()", self.h)
        tick = self.p.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "bool ALatebrakeCarPawn::IsRacePhysicsReady()", 1)[0]
        for expected in (
            "if (!IsRacePhysicsReady())",
            "if (!FMath::IsFinite(Speed)",
            "DrivingStats.Observe({",
            "static_cast<double>(DeltaSeconds)",
            "static_cast<double>(LateralSpeed)",
            "bEffectiveBraking"
        ):
            self.assertIn(expected, tick)
        self.assertLess(tick.index("if (!FMath::IsFinite(Speed)"),
                        tick.index("DrivingStats.Observe({"))
        self.assertLess(tick.index("DrivingStats.Observe({"),
                        tick.index("Chassis->AddForce(Forward * DriveForce"))

    def test_pause_and_teleport_break_sample_continuity(self):
        neutral = self.p.split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1
        )[1].split("void ALatebrakeCarPawn::ThrottleInput(", 1)[0]
        recovery = self.p.split("bool ALatebrakeCarPawn::TryRecoverTo(", 1)[1].split(
            "void ALatebrakeCarPawn::RecoverTo(", 1)[0]
        self.assertIn("DrivingStats.Suspend();", neutral)
        self.assertIn("DrivingStats.Suspend();", recovery)
        self.assertLess(recovery.index("DrivingStats.Suspend();"),
                        recovery.index("SetActorLocationAndRotation("))

    def test_new_race_clears_stats_and_hud_is_observational(self):
        restart = self.m.split(
            "void ALatebrakeRaceGameMode::RestartRace()", 1
        )[1].split("void ALatebrakeRaceGameMode::RecoverPlayer(", 1)[0]
        self.assertIn("Car->ResetDrivingStats();", restart)
        self.assertLess(restart.index("Race.Reset();"),
                        restart.index("Car->ResetDrivingStats();"))
        self.assertLess(restart.index("Car->ResetDrivingStats();"),
                        restart.index("Car->TryRecoverTo("))
        for signal in ("GetDrivingStats()", "GetDistanceKm()",
                       "GetPeakSpeedKmH()", "GetBrakeApplications()",
                       "GetHighSlipSeconds()"):
            self.assertIn(signal, self.hud)
        self.assertNotIn("PassCheckpoint(", self.prod)
        self.assertNotIn("SaveGame", self.prod)

    def test_production_policy_is_linked_into_strict_ci(self):
        for marker in ("DrivingSessionStatsTests.cpp",
                       "Private/Racing/DrivingSessionStats.cpp",
                       "NAME DrivingSessionStats"):
            self.assertIn(marker, self.cmake)
        for marker in ("stats.Suspend();", "stats.Reset();",
                       "stats.GetDistanceKm()", "quiet_NaN()"):
            self.assertIn(marker, self.tests)

if __name__ == "__main__":
    unittest.main()
