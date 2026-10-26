"""Source proof for read-only post-GO player launch input onset.

Does not compile licensed Unreal Engine, verify installed keyboard/touch
delivery, measure human reflexes, or award career/game credits.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
POLICY = ROOT / "Source/Latebrake/Public/Racing/LaunchInputOnset.h"
PAWN_H = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
MODE = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
TEST = ROOT / "Tests/Standalone/LaunchInputOnsetTests.cpp"


class LaunchInputOnsetBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.header = PAWN_H.read_text(encoding="utf-8")
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")

    def test_pawn_reads_real_game_clock_after_go_and_no_prephysics_ghost(self):
        self.assertIn('#include "Racing/LaunchInputOnset.h"', self.header)
        self.assertIn("latebrake::LaunchInputOnset LaunchOnset;", self.header)
        self.assertIn("LaunchOnset.Reset();", self.header)
        self.assertIn("Car->ResetDrivingStats();", self.mode)
        tick = self.pawn.split(
            "void ALatebrakeCarPawn::Tick(", 1)[1].split(
                "bool ALatebrakeCarPawn::IsRacePhysicsReady()", 1)[0]
        for frag in (
            "Mode->IsRaceStartGreen()",
            "IsRacePhysicsReady()",
            "FMath::IsFinite(Velocity.X)",
            "FMath::IsFinite(Speed)",
            "LaunchOnset.Observe(",
            "Mode->GetRace().GetElapsedSeconds()",
            "static_cast<double>(EffectiveThrottle)",
            "UGameplayStatics::GetPlayerPawn(this, 0) == this",
            "Mode->HasLocalPlayerDriver()",
        ):
            self.assertIn(frag, tick)
        self.assertLess(tick.index("!Mode->IsRaceStartGreen()"),
                        tick.index("LaunchOnset.Observe("))
        self.assertLess(tick.index("if (!IsRacePhysicsReady())"),
                        tick.index("LaunchOnset.Observe("))
        self.assertLess(tick.index("FMath::IsFinite(Speed)"),
                        tick.index("LaunchOnset.Observe("))

    def test_results_readout_has_no_awards_and_checks_actual_finish(self):
        self.assertIn(
            'TEXT("FIRST FORWARD INPUT AFTER GO: %.2fs (NO BONUS)")',
            self.hud)
        self.assertIn("Car->GetLaunchOnset().HasInputOnset()", self.hud)
        self.assertIn("Car->GetLaunchOnset().FirstForwardInputSeconds()",
                      self.hud)
        self.assertIn("CanAwardFinishedRace(Race)", self.hud)
        for forbidden in (
            "AddTimePenalty(", "PassCheckpoint(", "SaveGame",
            "CareerAward(", "AddForce(", "SetAutonomousControls(",
        ):
            self.assertNotIn(forbidden, self.policy)

    def test_bounded_sample_policy_and_production_tests(self):
        for token in (
            "exactLocalPlayer", "braking", "std::isfinite(officialRaceSeconds)",
            "std::isfinite(effectiveForwardThrottle)", "MaxTrackedSeconds",
            "MinimumForwardCommand", "StartInputSeconds",
            "Captured = true", "void Reset()",
        ):
            self.assertIn(token, self.policy)
        for token in (
            "LaunchInputOnsetTests.cpp", "RaceProgress.cpp",
            "NAME LaunchInputOnset", "cxx_std_20",
            "/W4 /WX", "-Wpedantic -Werror",
        ):
            self.assertIn(token, self.cmake)
        for token in (
            "AI or unpossessed pawn", "braking does not count",
            "actual authoritative race clock",
            "race restart erases old player's launch measurement",
            "held forward throttle at GO",
        ):
            self.assertIn(token, self.test)


if __name__ == "__main__":
    unittest.main()
