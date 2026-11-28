"""Native source binding checks, not Unreal gameplay qualification."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class FleetGateTimingBindingTests(unittest.TestCase):
    def test_reset_and_binding_follow_actual_roster(self):
        source = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        restart = source.split("void ALatebrakeRaceGameMode::BeginExperimentalFleetAttempt()", 1)[1]
        self.assertLess(restart.index("FleetGateTiming.Reset()"), restart.index("FleetAttempt.Begin("))
        bind = source.index("BindFleetReviewLedger(FleetReviewLedger")
        self.assertGreater(source.index("FleetGateTiming.Configure(", bind), bind)
        self.assertIn("LedgerBound && !FleetGateTiming.Configure", source)
        self.assertIn("FleetAttempt.BoundIdentity(),Race.GetCheckpointCount(),Race.GetRequiredLaps()", source)

    def test_only_accepted_gate_callbacks_record_timing(self):
        mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        start = mode.index("void ALatebrakeRaceGameMode::RegisterCheckpoint(")
        player_record = mode.index("FleetGateTiming.Record({FleetAttemptToken", start)
        self.assertLess(mode.index("Race.PassCheckpoint(CheckpointIndex)", start), player_record)
        self.assertLess(player_record, mode.index("FleetFinishReview=FinishTiming", player_record))
        self.assertIn("Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds()", mode[start:player_record+350])
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        rival = runtime.split("void ALatebrakeRaceGameMode::ObserveFleetRivalGate(", 1)[1].split(
            "void ALatebrakeRaceGameMode::ObserveFleetRivalRecovery(", 1)[0]
        self.assertLess(rival.index("FleetReviewLedger.RecordAcceptedGate"), rival.index("FleetGateTiming.Record"))
        self.assertIn("IsExperimentalFleetDriveReady()", rival)
        self.assertIn("IsRegisteredRaceRival(Actor)", rival)
        self.assertIn("Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds()", rival)

    def test_review_and_hud_use_matched_gate_evidence(self):
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        review = runtime.split("ALatebrakeRaceGameMode::CollectFleetRaceReview()", 1)[1].split(
            "ALatebrakeRaceGameMode::GetFleetRaceReview()", 1)[0]
        self.assertLess(review.index("BuildFleetRaceReview("), review.index("ReviewFleetGateTiming("))
        self.assertIn("IsExperimentalFleetDriveReady()", review)
        self.assertIn("if (Race.IsFinished()) return FleetFinishReview;", runtime)
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for marker in ("Review.Timing.Rivals", "FleetReview.Timing.Rivals",
                       "Timing.CommonCohortValid", "MATCHED ACTIVE-CLOCK SPLITS",
                       "FleetTimingRelation::SameClockWindow"):
            self.assertIn(marker, hud)
        self.assertIn("add_latebrake_portable_research_test(FleetGateTimingJournal)",
                      read("Tests/Standalone/CMakeLists.txt"))

    def test_sector_feedback_is_from_live_and_frozen_matched_receipts(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        frozen = hud.index("const auto Review=Mode->GetFleetRaceReview()")
        frozen_sector = hud.index("if (Review.Timing.CohortSector.Valid)", frozen)
        frozen_end = hud.index("const auto& Traffic=Review.TrafficHistory;", frozen_sector)
        block = hud[frozen_sector:frozen_end]
        for marker in ("Sector.ToOrdinal", "Sector.RivalsGained", "Sector.RivalsClose",
                       "Sector.RivalsLost", "Sector.PlayerSeconds",
                       "Sector.MinimumRivalSeconds", "Sector.MaximumRivalSeconds"):
            self.assertIn(marker, block)
        live = hud.index("const auto& Timing=FleetReview.Timing;")
        live_sector = hud.index("if (!bCompact && Timing.CohortSector.Valid)", live)
        self.assertGreater(live_sector, live)
        row = hud.index("Timing.PreviousSectorTrend!=latebrake::FleetSectorTrend::Unavailable", live_sector)
        self.assertIn("FleetSectorTrendName", hud[row:row+500])
        policy = read("Source/Latebrake/Public/AI/FleetGateTimingJournal.h")
        self.assertIn("out.CohortSector=BuildCohortSector(cohortOrdinal);", policy)
        self.assertIn("2.0*FleetTimingUncertainSeconds", policy)
        self.assertIn("row.RetainedSectors=BuildRetainedSectorWindow(player,rival);", policy)
        self.assertIn("Rival.RetainedSectors", hud)
        self.assertIn("retained sectors G%d-%d", hud)


if __name__ == "__main__":
    unittest.main()
