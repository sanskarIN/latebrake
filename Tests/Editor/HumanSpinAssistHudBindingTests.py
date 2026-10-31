"""Source binding checks for advisory spin-assist motor-trim feedback.

Portable C++ verifies numeric behavior. These checks do not compile Unreal,
run Chaos physics, install a game, or qualify racing behavior on hardware.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class HumanSpinAssistHudBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.meter = read("Source/Latebrake/Public/Vehicles/HumanSpinAssistMeter.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.test = read("Tests/Standalone/HumanSpinAssistMeterTests.cpp")
        cls.docs = read("docs/HUMAN_SPIN_ASSIST_HUD_QA.md")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")

    def test_actual_final_throttle_drives_observation_before_motor(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        markers = [
            "!IsRacePhysicsReady()",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity();",
            "StabilizeCornerThrottle(",
            "GetPhysicsAngularVelocityInRadians()",
            "ModerateHumanSpinThrottle(",
            "SpinAssist.Observe({",
            "NitroBoost.Step(",
            "AllocateArcadeBoost(",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
        ]
        indices = [tick.index(m) for m in markers]
        self.assertEqual(indices, sorted(indices))
        self.assertIn("static_cast<double>(CornerThrottle)", tick)
        self.assertIn("static_cast<double>(EffectiveThrottle)", tick)
        self.assertIn("bArcade && bHumanDriver && !BrakeProfile.HandbrakeActive &&", tick)
        self.assertIn("!bEffectiveBraking", tick)
        self.assertEqual(tick.count("SpinAssist.Observe({"), 1)
        self.assertIn('#include "Vehicles/HumanSpinAssistMeter.h"', self.pawn_h)
        self.assertIn("latebrake::HumanSpinAssistMeter SpinAssist;", self.pawn_h)

    def test_pawn_interruption_and_race_restart_reset_live_display(self):
        clear = self.pawn.split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        self.assertIn("SpinAssist.Suspend();", clear)
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "const bool bArcade = HandlingMode", 1)[0]
        self.assertIn("SpinAssist.Suspend();", tick)
        self.assertIn("SpinAssist.Reset();", self.pawn_h)
        self.assertIn("ResetDrivingStats()", self.pawn_h)
        restart = self.mode.split("void ALatebrakeRaceGameMode::RestartRace()", 1)[1]
        self.assertIn("Car->ResetDrivingStats();", restart)
        self.assertIn("const latebrake::HumanSpinAssistMeter& GetSpinAssist()", self.pawn_h)

    def test_hud_displays_observed_trim_only_on_expanded_screen(self):
        selected = self.hud.split("// Optional human Arcade stability hint", 1)[1].split(
            "// Optional arcade brake-temperature feedback", 1)[0]
        for marker in ("!bCompact", "VerticalPlan.MaxTelemetryLines >= 28",
                       "Car->GetSpinAssist().IsActive()",
                       "Car->GetSpinAssist().CurrentReductionFraction() * 100.0",
                       "SPIN ASSIST", "POSITIVE THROTTLE TRIM"):
            self.assertIn(marker, selected)
        self.assertNotIn("GetPhysicsAngularVelocity", selected)
        self.assertNotIn("PassCheckpoint(", selected)
        self.assertNotIn("AwardCredits(", selected)

    def test_meter_is_safe_read_only_and_strictly_tested(self):
        for forbidden in ("UCLASS(", "AddForce(", "AddTorque(",
                          "SaveGame", "RaceProgress", "SetPhysicsLinearVelocity",
                          "PassCheckpoint", "SetActorLocation", "AwardCredits"):
            self.assertNotIn(forbidden, self.meter)
        for marker in ("std::isfinite(s.CornerThrottle)",
                       "std::isfinite(s.FinalThrottle)",
                       "MaxSampleSeconds = 0.10",
                       "MinDisplayReduction = 0.05",
                       "MaxExpectedReduction = 0.70",
                       "MaxInterventions = 65535u",
                       "void Reset() noexcept", "void Suspend() noexcept"):
            self.assertIn(marker, self.meter)
        self.assertIn("HumanSpinAssistMeterTests.cpp", self.cmake)
        self.assertIn("NAME HumanSpinAssistMeter", self.cmake)
        self.assertIn("latebrake_human_spin_assist_meter_tests PRIVATE /W4 /WX",
                      self.cmake)
        self.assertIn("200000", self.test)
        self.assertIn("Unreal Engine 5.8", self.docs)
        self.assertIn("not a playable", self.docs.lower())


if __name__ == "__main__":
    unittest.main()
