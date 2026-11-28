"""Source bindings for observed whole-run pace, not native gameplay tests."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class FleetPaceReviewBindingTests(unittest.TestCase):
    def test_only_accepted_gate_receipts_enter_pace_stream(self):
        journal = read("Source/Latebrake/Public/AI/FleetGateTimingJournal.h")
        record = journal.split("bool Record(const FleetTimingGateReceipt& receipt)", 1)[1].split(
            "FleetTimingReadout Review(", 1)[0]
        for marker in ("receipt.Attempt != Attempt", "receipt.ActorToken",
                       "receipt.Ordinal != stream.LastOrdinal + 1",
                       "receipt.ActiveSeconds <= stream.LastSeconds",
                       "stream.Pace.Record(receipt.Ordinal,receipt.ActiveSeconds)"):
            self.assertIn(marker, record)
        self.assertLess(record.index("receipt.Ordinal != stream.LastOrdinal + 1"),
                        record.index("stream.Pace.Record("))
        self.assertLess(record.index("stream.Pace.Record("),
                        record.index("stream.LastOrdinal = receipt.Ordinal"))

    def test_existing_route_evidence_and_finish_snapshot(self):
        review = read("Source/Latebrake/Public/AI/FleetRaceReview.h")
        self.assertIn("FleetTimingEvidenceFromReview(review)", review)
        self.assertIn("row.Slot == slot && row.BodyObserved && row.RouteEvidenceValid", review)
        self.assertIn("!review.PlayerFinished", review)
        journal = read("Source/Latebrake/Public/AI/FleetGateTimingJournal.h")
        pace = journal.split("FleetPaceReadout ReviewPace(", 1)[1]
        self.assertIn("MatchesEvidence(player,evidence[0],activeSeconds,false)", pace)
        self.assertIn("MatchesEvidence(rival,evidence[index],activeSeconds,true)", pace)
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        self.assertIn("if (Review.PlayerFinished)\n        Review.Pace=", runtime)
        self.assertIn("if (Race.IsFinished()) return FleetFinishReview;", runtime)

    def test_hud_is_observational_and_explicitly_labelled(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("OBSERVED ACTIVE-CLOCK PACE | NOT CLEAN LAP RECORDS", hud)
        self.assertIn("mean AI-minus-you", hud)
        self.assertIn("Sector averages need equal completed-lap coverage", hud)
        self.assertNotIn("ReviewFleetWholeRunPace(", hud)
        self.assertNotIn("Pace.Record(", hud)
        self.assertIn("add_latebrake_portable_research_test(FleetPaceReview)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
