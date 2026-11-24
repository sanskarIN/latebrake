"""Production binding only, not Unreal render or package QA."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class MeasuredLapSplitReferenceBindingTests(unittest.TestCase):
    def test_real_ordered_gate_clock_is_the_only_source(self):
        race=read("Source/Latebrake/Private/Racing/RaceProgress.cpp")
        checkpoint=race.split("bool RaceProgress::PassCheckpoint(",1)[1]
        self.assertLess(checkpoint.index("checkpointIndex != NextCheckpoint"),
                        checkpoint.index("MeasuredSplits.Observe("))
        self.assertIn("ActiveClockAdvancedThisSector = true;",race)
        self.assertIn("ActiveClockAdvancedThisSector = false;",checkpoint)
        self.assertIn("MeasuredSplits.Complete(CompletedLaps, LastLapSeconds, LastLapPenaltySeconds);",race)
        self.assertIn("MeasuredSplits.Reset(CheckpointCount);",race)

    def test_hud_has_actual_reference_and_discrete_gate_delta(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for mark in ("Race.GetMeasuredLapSplits().LatestGate()",
                     "Race.GetMeasuredLapSplits().LastComparison()",
                     "GATE %d | VS FEE-FREE LAP %d",
                     "FINAL LAP %d VS FEE-FREE LAP %d",
                     "GATES FASTER %d | MATCHED %d | SLOWER %d",
                     "Row.CumulativeDeltaSeconds, Row.IncludedFeeSeconds"):
            self.assertIn(mark,hud)
        policy=read("Source/Latebrake/Public/Racing/MeasuredLapSplitReference.h")
        for forbidden in ("SaveRecords(", "AddTimePenalty(", "PassCheckpoint(",
                          "SetActorLocation", "std::vector"):
            self.assertNotIn(forbidden,policy)
        self.assertIn("last_=CompareMeasuredLaps(current_,best_);",policy)
        self.assertIn("NAME MeasuredLapSplitReference",read("Tests/Standalone/CMakeLists.txt"))

if __name__=="__main__":unittest.main()
