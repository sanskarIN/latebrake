"""Run-local ghost samples are isolated by actual applied physical configuration."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class GhostSetupGuardBindingTests(unittest.TestCase):
    def test_actual_tuning_and_actor_are_required(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        guard=mode.split("void ALatebrakeRaceGameMode::RefreshGhostConfiguration(",1)[1].split(
            "void ALatebrakeRaceGameMode::SuspendBestLapGhost()",1)[0]
        for marker in ("GetActivePracticeTrial().Key","LastTunedPawn.Get()==Car",
                       "LastAppliedTuning==Key","Car->GetSelectedVehicleId()==Key.Selected",
                       "Car->GetHandlingMode()", "Car->GetUniqueID()", "RefreshGhostSetup(",
                       "Race.GetCurrentLapSeconds()==0.0", "SetActorHiddenInGame(true)"):
            self.assertIn(marker,guard)
        tick=mode.split("void ALatebrakeRaceGameMode::Tick(",1)[1].split(
            "void ALatebrakeRaceGameMode::RefreshGhostConfiguration(",1)[0]
        self.assertLess(tick.index("RefreshGhostConfiguration(GhostDriver)"),tick.index("GhostRecorder.Record("))
        self.assertIn("GhostSetup.IsBound()",tick)
        apply=mode.split("void ALatebrakeRaceGameMode::ApplyGarageToPlayer()",1)[1].split(
            "void ALatebrakeRaceGameMode::DismissGaragePalette()",1)[0]
        self.assertLess(apply.index("LastAppliedTuning = Requested;"),apply.index("RefreshGhostConfiguration(Car)"))
        handling=mode.split("void ALatebrakeRaceGameMode::NotifyPracticeHandlingChange(",1)[1].split(
            "FString ALatebrakeRaceGameMode::GetCareerEventName",1)[0]
        self.assertIn("RefreshGhostConfiguration(Car)",handling)
    def test_real_lap_finish_and_reset(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        gate=mode.split("void ALatebrakeRaceGameMode::RegisterCheckpoint(",1)[1].split(
            "void ALatebrakeRaceGameMode::",1)[0]
        self.assertLess(gate.index("RefreshGhostConfiguration(LapDriver)"),gate.index("GhostRecorder.FinishLap("))
        self.assertIn("bRealLapHadActiveTime && GhostSetup.IsBound()",gate)
        self.assertEqual(mode.count("GhostSetup.Reset();"),2)
        self.assertIn("add_latebrake_portable_research_test(GhostSetupGuard)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
