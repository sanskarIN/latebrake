"""v0.5.75 source bindings for a real 3/2/1/GO race start.

Source inspection is NOT Unreal Engine 5.8 compilation, physics, rendering,
cooked level validation or installed Win64/Android gameplay.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODE_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
MODE_CPP = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
RIVAL = ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
CHECKPOINT = ROOT / "Source/Latebrake/Private/Racing/LatebrakeCheckpoint.cpp"
LIGHTS = ROOT / "Source/Latebrake/Public/Racing/RaceStartLights.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
PORTABLE = ROOT / "Tests/Standalone/RaceStartLightsTests.cpp"


class RaceStartLightsBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode_h = MODE_H.read_text(encoding="utf-8")
        cls.mode = MODE_CPP.read_text(encoding="utf-8")
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.rival = RIVAL.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.checkpoint = CHECKPOINT.read_text(encoding="utf-8")
        cls.lights = LIGHTS.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.portable = PORTABLE.read_text(encoding="utf-8")

    def test_source_mode_owns_single_authoritative_start(self):
        self.assertIn("latebrake::RaceStartLights StartLights;", self.mode_h)
        self.assertIn("StartLights.IsGreen();", self.mode_h)
        self.assertIn("StartLights.Phase();", self.mode_h)
        tick = self.mode.split(
            "void ALatebrakeRaceGameMode::Tick(", 1)[1].split(
                "AStaticMeshActor* ALatebrakeRaceGameMode::SpawnBlock", 1)[0]
        for code in (
            "bCircuitReady && bLocalPlayerReady && !bInactive",
            "!Race.IsFinished() && IsRestartPlacementReady()",
            "StartLights.Advance(",
            "bInactive || !StartLights.IsGreen()",
            "bLocalPlayerReady, LiveDelta",
        ):
            self.assertIn(code, tick)
        self.assertLess(tick.index("StartLights.Advance("),
                        tick.index("AdvancePlayerRaceClockIfReady("))

    def test_gate_recovery_and_restart_must_share_start(self):
        begin = self.mode.split(
            "void ALatebrakeRaceGameMode::BeginPlay()", 1)[1].split(
                "void ALatebrakeRaceGameMode::Tick(", 1)[0]
        restart = self.mode.split(
            "void ALatebrakeRaceGameMode::RestartRace()", 1)[1].split(
                "void ALatebrakeRaceGameMode::RecoverPlayer(", 1)[0]
        checkpoint = self.mode.split(
            "void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1)[1].split(
                "void ALatebrakeRaceGameMode::", 1)[0]
        recovery = self.mode.split(
            "void ALatebrakeRaceGameMode::RecoverPlayer(", 1)[1].split(
                "void ALatebrakeRaceGameMode::LoadRecords(", 1)[0]
        for segment in (begin, restart):
            self.assertIn("Race.Reset();", segment)
            self.assertIn("StartLights.Reset();", segment)
            self.assertLess(segment.index("Race.Reset();"),
                            segment.index("StartLights.Reset();"))
        self.assertLess(checkpoint.index("!StartLights.IsGreen()"),
                        checkpoint.index("CanScorePlayerCheckpoint("))
        self.assertLess(recovery.index("!StartLights.IsGreen()"),
                        recovery.index("TryChargePlayerRecoveryAfterVerifiedTeleport("))

    def test_gate_trace_does_not_sample_motion_during_red(self):
        gate_tick = self.checkpoint.split(
            "void ALatebrakeCheckpoint::Tick(", 1)[1]
        self.assertIn("!Mode->IsRaceStartGreen()", gate_tick)
        self.assertLess(
            gate_tick.index("!Mode->IsRaceStartGreen()"),
            gate_tick.index("MotionTrace.SampleDetailed("))
        self.assertIn("MotionTrace.Reset();", gate_tick)
        self.assertLess(
            gate_tick.index("MotionTrace.Reset();"),
            gate_tick.index("MotionTrace.SampleDetailed("))
        self.assertLess(
            gate_tick.index("!Mode->IsRaceStartGreen()"),
            gate_tick.index("Mode->RegisterCheckpoint("))

    def test_pawn_and_rival_both_refuse_pregreen_forces(self):
        player_tick = self.pawn.split(
            "void ALatebrakeCarPawn::Tick(", 1)[1].split(
                "bool ALatebrakeCarPawn::IsRacePhysicsReady()", 1)[0]
        rival_tick = self.rival.split(
            "void ALatebrakeRivalCar::Tick(", 1)[1].split(
                "const FVector Position", 1)[0]
        self.assertIn("!Mode->IsRaceStartGreen()", player_tick)
        self.assertIn("!Mode->IsRaceStartGreen()", rival_tick)
        self.assertLess(player_tick.index("!Mode->IsRaceStartGreen()"),
                        player_tick.index("CanApplyPlayerDrive({"))
        self.assertLess(rival_tick.index("!Mode->IsRaceStartGreen()"),
                        rival_tick.index("CanDriveRival("))
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", player_tick)
        self.assertIn("SetAutonomousControls(0.f, 0.f, true);", rival_tick)

    def test_hud_displays_truthful_phase_after_official_clock(self):
        hud = self.hud[self.hud.index("const auto Line = [&]"):]
        for x in (
            "GetRaceStartPhase()", "RaceStartPhase::Three",
            "RaceStartPhase::Two", "RaceStartPhase::One",
            "RaceStartPhase::Go", 'TEXT("START LIGHTS  3")',
            'TEXT("START LIGHTS  2")',
            'TEXT("START LIGHTS  1")', 'TEXT("GO!")',
        ):
            self.assertIn(x, hud)
        self.assertLess(hud.index('TEXT("TIME  %02d:%05.2f")'),
                        hud.index('TEXT("START LIGHTS  3")'))
        self.assertIn("Line(TEXT(\"GO!\"), Accent, true)", hud)

    def test_policy_rejects_invalid_and_hitch_time(self):
        for required in (
            "std::isfinite(deltaSeconds)", "MaxAcceptedFrameSeconds",
            "const double liveSeconds = deltaSeconds - remaining",
            "if (!raceReady", "TimeSinceGreenSeconds",
            "if (deltaSeconds > MaxAcceptedFrameSeconds)",
            "GoVisibleSeconds", "return liveSeconds;",
        ):
            self.assertIn(required, self.lights)
        for forbidden in (
            "SetGamePaused", "AddForce(", "PassCheckpoint(",
            "AddTimePenalty(", "SaveGame", "UCLASS(",
        ):
            self.assertNotIn(forbidden, self.lights)
        for token in (
            "RaceStartLightsTests.cpp", "NAME RaceStartLights",
            "RaceProgress.cpp", "cxx_std_20", "/W4 /WX",
            "-Wpedantic -Werror",
        ):
            self.assertIn(token, self.cmake)
        for case in (
            "pause also freezes green GO announcement",
            "authoritative clock excludes all earlier lights",
            "exact start boundary never contributes time",
            "red light remains one at 2.5 eligible seconds",
            "once GO is live, real positive hitch time is counted",
        ):
            self.assertIn(case, self.portable)


if __name__ == "__main__":
    unittest.main()
