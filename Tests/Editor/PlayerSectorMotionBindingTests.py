"""Source binding for observational sector motion, not Unreal playtest proof."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class PlayerSectorMotionBindingTests(unittest.TestCase):
    def test_actual_clock_and_accepted_gate(self):
        source = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        self.assertLess(source.index("AdvancePlayerRaceClockIfReady("),
                        source.index("ObservePlayerSectorMotion(LiveDelta,bActualClockAdvanced)"))
        gate = source.split("void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1)[1]
        self.assertLess(gate.index("if (!Race.PassCheckpoint(CheckpointIndex))"),
                        gate.index("ObservePlayerSectorGate(CheckpointIndex"))
        self.assertEqual(source.count("Race.Reset();\n    ResetPlayerSectorMotion();"), 2)

    def test_actual_driver_identity_and_motion(self):
        source = read("Source/Latebrake/Private/Game/LatebrakeSectorMotionRuntime.cpp")
        for marker in ("!bClockAdvanced", "Race.IsFinished()", "HasLocalPlayerDriver()",
                       "Car->IsLocallyControlled()", "Car->IsRacePhysicsReady()",
                       "SectorMotionPawn.Get()!=Car", "Car->GetActorLocation()",
                       "Car->GetVelocity()", "Car->GetActorRotation()", "GFrameCounter",
                       "CheckpointSession.Current()", "Car->GetTeleportRevision()",
                       "Race.GetElapsedSeconds()", "Race.GetTotalPenaltySeconds()",
                       "Race.GetLastSectorSeconds()", "PlayerSectorMotion.Observe(Sample)"):
            self.assertIn(marker, source)
        for marker in ("Race.Advance(", "Race.PassCheckpoint(", "Race.AddTimePenalty(",
                       "SetActorLocation(", "SaveGame"):
            self.assertNotIn(marker, source)

    def test_ui_and_portable_suite(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("SECTOR MOTION REVIEW | ACTUAL SAMPLED CHASSIS", hud)
        self.assertIn("Standing start is not compared with rolling sectors", hud)
        self.assertIn("projected off-road", hud)
        self.assertIn("GetPlayerSectorMotionReview()", hud)
        self.assertIn("add_latebrake_portable_research_test(PlayerSectorMotionReview)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
