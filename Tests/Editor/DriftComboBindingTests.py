"""v0.5.73 actual Unreal source binding for advisory arcade drift meter.

These source contracts are not UHT, Unreal physics, render tests or
evidence of a packaged game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN_HEADER = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
MODE = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
METER = ROOT / "Source/Latebrake/Public/Racing/DriftComboMeter.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
TEST = ROOT / "Tests/Standalone/DriftComboMeterTests.cpp"


class DriftComboBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = PAWN_HEADER.read_text(encoding="utf-8")
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.meter = METER.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")

    def test_real_pawn_uses_observed_chassis_velocity_only(self):
        self.assertIn('#include "Racing/DriftComboMeter.h"', self.header)
        self.assertIn("latebrake::DriftComboMeter DriftMeter;", self.header)
        self.assertIn("GetDriftMeter() const", self.header)
        self.assertIn("void ResetDrivingStats()", self.header)
        self.assertIn("DrivingStats.Reset(); DriftMeter.Reset();", self.header)
        start = self.pawn.index("void ALatebrakeCarPawn::Tick(")
        end = self.pawn.index("bool ALatebrakeCarPawn::TryRecoverTo(")
        tick = self.pawn[start:end]
        for required in (
            "if (!IsRacePhysicsReady())",
            "if (!FMath::IsFinite(Speed)",
            "static_cast<double>(DeltaSeconds)",
            "static_cast<double>(LateralSpeed)",
            "DriftMeter.Observe({",
            "DrivingStats.Observe({",
        ):
            self.assertIn(required, tick)
        self.assertLess(tick.index("if (!FMath::IsFinite(Speed)"),
                        tick.index("DriftMeter.Observe({"))
        self.assertLess(tick.index("DrivingStats.Observe({"),
                        tick.index("DriftMeter.Observe({"))
        self.assertLess(tick.index("DriftMeter.Observe({"),
                        tick.index("Chassis->AddForce("))

    def test_restart_pause_and_recovery_stop_ghost_combos(self):
        neutral = self.pawn.split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1
        )[1].split("void ALatebrakeCarPawn::ThrottleInput(", 1)[0]
        recovery = self.pawn.split(
            "bool ALatebrakeCarPawn::TryRecoverTo(", 1
        )[1].split("void ALatebrakeCarPawn::RecoverTo(", 1)[0]
        for snippet in (neutral, recovery):
            self.assertIn("DrivingStats.Suspend();", snippet)
            self.assertIn("DriftMeter.Suspend();", snippet)
        self.assertLess(recovery.index("DriftMeter.Suspend();"),
                        recovery.index("SetActorLocationAndRotation("))
        self.assertIn("Car->ResetDrivingStats();", self.mode)

    def test_source_hud_never_changes_rewards(self):
        self.assertIn("Car->GetDriftMeter()", self.hud)
        self.assertIn('TEXT("ARCADE DRIFT SCORE (NO CREDITS)")', self.hud)
        self.assertIn('TEXT("DRIFT COMBO %.0f pts | %.1fs")', self.hud)
        self.assertIn("Drift.GetCurrentComboPoints()", self.hud)
        self.assertIn("Drift.GetTotalPoints()", self.hud)
        self.assertIn("Drift.GetTotalDriftSeconds()", self.hud)
        self.assertIn("if (latebrake::CanAwardFinishedRace(Race))", self.hud)
        for prohibited in ("PassCheckpoint(", "AddTimePenalty(",
                           "SaveGame", "SetActorLocation(", "CareerAward("):
            self.assertNotIn(prohibited, self.meter)

    def test_strict_reproducible_portable_regression(self):
        for phrase in (
            "DriftComboMeterTests.cpp", "NAME DriftComboMeter",
            "cxx_std_20", "/W4 /WX", "-Wpedantic -Werror"
        ):
            self.assertIn(phrase, self.cmake)
        for case in (
            "m.Suspend()", "m.Reset()", "quiet_NaN()",
            "low-speed lateral movement", "third qualifying sample",
            "return to grip banks combo", "repeated suspension"
        ):
            self.assertIn(case, self.test)


if __name__ == "__main__":
    unittest.main()
