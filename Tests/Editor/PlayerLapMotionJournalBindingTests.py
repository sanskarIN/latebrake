"""Full lap motion uses actual accepted player gate receipts, not estimates."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class PlayerLapMotionJournalBindingTests(unittest.TestCase):
    def test_receipt_and_reset_binding(self):
        runtime=read("Source/Latebrake/Private/Game/LatebrakeSectorMotionRuntime.cpp")
        self.assertIn("PlayerLapMotion.Reset(Race.GetCheckpointCount(),Race.GetRequiredLaps())",runtime)
        gate=runtime.split("ALatebrakeRaceGameMode::ObservePlayerSectorGate(",1)[1].split(
            "ALatebrakeRaceGameMode::GetPlayerLapMotionReview()",1)[0]
        self.assertLess(gate.index("PlayerSectorMotion.AcceptGate(Receipt)"),
                        gate.index("PlayerLapMotion.Accept(Receipt"))
        self.assertIn("Race.GetLastSectorSeconds()",gate)
        self.assertIn("Sector.Valid?Sector.Last:latebrake::SectorMotionRow{}",gate)
        self.assertNotIn("PassCheckpoint(",gate)
    def test_read_only_results(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for marker in ("Mode->GetPlayerLapMotionReview()", "FULL LAP MOTION | ACTUAL OBSERVATIONS",
                       "BEST QUALIFIED ROLLING LAP", "LapMotion.At(Lap)", "OBSERVED CLOCK"):
            self.assertIn(marker,hud)
        policy=read("Source/Latebrake/Public/Racing/PlayerLapMotionJournal.h")
        for forbidden in ("SaveRecords(", "EarnCredits(", "AddTimePenalty(", "PassCheckpoint("):
            self.assertNotIn(forbidden,policy)
        self.assertIn("add_latebrake_portable_research_test(PlayerLapMotionJournal)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
