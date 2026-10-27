"""Check that the portable ghost policy connects only to real race events.

Source contracts complement but DO NOT replace actual UE5.8/UHT,
packaged renderer, controller or installed-gaming QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class BestLapGhostBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def read(path):
            return (ROOT / path).read_text(encoding="utf-8")
        cls.game = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.game_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.policy = read("Source/Latebrake/Public/Racing/BestLapGhost.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_real_clock_and_chassis_sample_first(self):
        tick = self.game.split("void ALatebrakeRaceGameMode::Tick(", 1)[1].split(
            "void ALatebrakeRaceGameMode::UpdateBestLapGhostMarker(", 1
        )[0]
        for marker in (
            "HasLocalPlayerDriver()",
            "Race.IsFinished()",
            "StartLights.Advance(",
            "AdvancePlayerRaceClockIfReady(",
            "bStartQualified && StartLights.IsGreen() && LiveDelta > 0.0",
            "GhostDriver->IsRacePhysicsReady()",
            "GhostDriver->GetTeleportRevision()",
            "CheckpointSession.Current()",
            "GhostRecorder.InvalidateCurrentLap();",
            "Race.GetCurrentLapSeconds()",
            "GhostDriver->GetActorLocation()",
            "GhostDriver->GetActorRotation().Yaw",
            "UpdateBestLapGhostMarker(bStartQualified && StartLights.IsGreen());",
        ):
            self.assertIn(marker, tick)
        self.assertLess(tick.index("AdvancePlayerRaceClockIfReady("),
                        tick.index("GhostRecorder.Record("))
        self.assertLess(tick.index("GhostDriver->GetTeleportRevision()"),
                        tick.index("GhostRecorder.Record("))

    def test_records_only_after_real_authoritative_completed_gate(self):
        source = self.game.split(
            "void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1
        )[1].split("void ALatebrakeRaceGameMode::", 1)[0]
        self.assertIn("CanScorePlayerCheckpoint(", source)
        self.assertIn("if (!Race.PassCheckpoint(CheckpointIndex))", source)
        self.assertIn("Race.GetCompletedLaps() > LapsBeforeGate", source)
        self.assertIn("Race.GetActiveTimedLapCount() == Race.GetCompletedLaps()", source)
        self.assertIn("Race.GetLastLapPenaltySeconds() == 0.0", source)
        self.assertIn("GhostRecorder.FinishLap(Race.GetLastLapSeconds(),", source)
        self.assertLess(source.index("if (!Race.PassCheckpoint(CheckpointIndex))"),
                        source.index("GhostRecorder.FinishLap("))
        self.assertLess(source.index("GhostRecorder.FinishLap("),
                        source.index("ValidateFinishTiming(Race)"))

    def test_ghost_actor_no_collision_or_gameplay_authority(self):
        ghost = self.game.split(
            "void ALatebrakeRaceGameMode::UpdateBestLapGhostMarker(", 1
        )[1].split("AStaticMeshActor* ALatebrakeRaceGameMode::SpawnBlock(", 1)[0]
        for required in (
            "Race.GetCompletedLaps() > 0",
            "GhostRecorder.Evaluate(",
            "SetActorHiddenInGame(true)",
            'TEXT("/Engine/BasicShapes/Cube.Cube")',
            "SetMobility(EComponentMobility::Movable)",
            "SetSimulatePhysics(false)",
            "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
            "SetGenerateOverlapEvents(false)",
            "SetCastShadow(false)",
            "SetActorEnableCollision(false)",
            "SetActorLocationAndRotation(",
            "ETeleportType::TeleportPhysics",
        ):
            self.assertIn(required, ghost)
        for forbidden in (
            "AddTimePenalty", "AddForce(", "SaveRecords(", "Award(",
            "PassCheckpoint(", "GetGarage()", "AddCredits(",
        ):
            self.assertNotIn(forbidden, ghost)

    def test_restart_and_source_run_isolation(self):
        restart = self.game.split(
            "void ALatebrakeRaceGameMode::RestartRace()", 1
        )[1].split("void ALatebrakeRaceGameMode::", 1)[0]
        for marker in (
            "GhostRecorder.ResetRun()",
            "GhostTrackedPawn.Reset()",
            "GhostTeleportRevision = 0",
            "GhostSessionRevision = CheckpointSession.Current()",
            "BestLapGhostMarker->SetActorHiddenInGame(true)",
        ):
            self.assertIn(marker, restart)
        self.assertIn("GhostRecorder.ResetRun()", self.game.split(
            "void ALatebrakeRaceGameMode::BeginPlay()", 1
        )[1].split("void ALatebrakeRaceGameMode::Tick(", 1)[0])
        self.assertIn("std::array<LapGhostPose, MaxSamples>", self.policy)
        self.assertIn("static constexpr std::size_t MaxSamples = 4096", self.policy)
        self.assertIn("NAME BestLapGhost", self.cmake)

    def test_visual_status_never_replaces_official_hud_lines(self):
        self.assertIn("Mode->HasBestLapGhost()", self.hud)
        self.assertIn("SESSION GHOST BEACON:", self.hud)
        self.assertIn("BEST-LAP GHOST BEACON", self.hud)
        self.assertIn("GetBestLapGhostSeconds()", self.game_h)
        self.assertNotIn("GhostRecorder", self.game.split(
            "void ALatebrakeRaceGameMode::SaveRecords()", 1
        )[1].split("void ALatebrakeRaceGameMode::", 1)[0])


if __name__ == "__main__":
    unittest.main()
