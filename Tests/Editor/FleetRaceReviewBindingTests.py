"""Source contracts for per-rival evidence, not official race ranking."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class FleetRaceReviewBindingTests(unittest.TestCase):
    def test_actual_gate_recovery_receipts(self):
        rival=read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        self.assertLess(rival.index("Route.ObserveVerifiedGate("),
                        rival.index("Mode->ObserveFleetRivalGate(this)"))
        self.assertLess(rival.index("Route.ReportRecoveryResult(bRecovered)"),
                        rival.index("Mode->ObserveFleetRivalRecovery(this,bRecovered)"))
        mode=read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        for marker in ("BindFleetReviewLedger(FleetReviewLedger",
                       "FleetReviewLedger.RecordAcceptedGate",
                       "FleetReviewLedger.RecordRecoveryResult",
                       "Rival->GetPilotSuccessfulRecoveries()",
                       "Rival->GetPilotFailedRecoveries()"):
            self.assertIn(marker,mode)
    def test_finish_review_precedes_revocation_and_save(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        a=mode.index("FleetFinishReview=CollectFleetRaceReview()")
        b=mode.index("FleetAttempt.Finish(FleetAttemptToken)")
        c=mode.index("latebrake::ProfileWriteCheckpoint SavedProfile",b)
        self.assertLess(a,b);self.assertLess(b,c)
        self.assertIn("return;",mode[b:c])
    def test_fleet_radar_does_not_reuse_single_rival(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for marker in ("!bExperimentalFleet && !bTimeAttack && Mode->HasRival()",
                       "GroupFleetRadar(FleetReview,0.075)",
                       "EXPERIMENTAL FLEET | NO SAVED REWARDS OR RANK",
                       "FleetReview.UnavailableRivals"):
            self.assertIn(marker,hud)
        self.assertIn("add_latebrake_portable_research_test(FleetRaceReview)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
