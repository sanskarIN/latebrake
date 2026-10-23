"""Structural tests for local QA logs — these are NOT Unreal runtime logs."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from AnalyzeCheckpointQA import QALogError, summarize_log  # noqa: E402


def record(gate=0, samples=100, crossed=2, teleport=1, invalid=0,
           wrong=3, travel=1, lane=1, height=0, no_plane=50,
           invalid_velocity=None):
    suffix = (
        "" if invalid_velocity is None
        else f" invalid-velocity={invalid_velocity}"
    )
    return (
        f"LogTemp: Display: Latebrake checkpoint={gate} "
        f"qa-samples={samples} crossed={crossed} "
        f"teleport-rebase={teleport} invalid-position={invalid} "
        f"wrong-way={wrong} frame-travel={travel} lane-miss={lane} "
        f"height-miss={height} no-plane={no_plane}{suffix}"
    )


class CheckpointQALogTests(unittest.TestCase):
    def test_one_gate_sums_multiple_intervals(self):
        report = summarize_log(record() + "\n" + record(samples=40, crossed=0, no_plane=20))
        self.assertEqual(report["windows"], 2)
        self.assertEqual(report["gate_summaries"][0]["samples"], 140)
        self.assertEqual(report["gate_summaries"][0]["crossed"], 2)
        self.assertFalse(report["source_authenticity_verified"])
        self.assertFalse(report["unreal_build_or_gameplay_verified"])

    def test_all_four_gates_required_for_full_race_evidence(self):
        lines = "\n".join(record(gate=gate) for gate in range(4))
        report = summarize_log(lines, require_all_gates=True)
        self.assertEqual([r["gate"] for r in report["gate_summaries"]],
                         [0, 1, 2, 3])
        with self.assertRaisesRegex(QALogError, "missing summaries"):
            summarize_log(record(gate=0), require_all_gates=True)

    def test_skip_unrelated_engine_and_v053_mode_one_logs(self):
        text = (
            "LogInit: unreal startup\n"
            "LogTemp: Display: Latebrake checkpoint=0 trace=crossed "
            "reason=crossed dt=0.0167\n"
            + record(gate=1) + "\n"
        )
        report = summarize_log(text)
        self.assertEqual(report["windows"], 1)
        self.assertEqual(report["gate_summaries"][0]["gate"], 1)

    def test_missing_or_partial_summary_is_not_evidence(self):
        with self.assertRaisesRegex(QALogError, "no v0.5.4"):
            summarize_log("LogTemp: Display: nothing recorded")
        with self.assertRaisesRegex(QALogError, "malformed"):
            summarize_log(record()[:-10])

    def test_reject_index_overflow_and_inconsistent_buckets(self):
        with self.assertRaisesRegex(QALogError, "index"):
            summarize_log(record(gate=4))
        with self.assertRaisesRegex(QALogError, "uint32"):
            summarize_log(record(samples=2 ** 32))
        with self.assertRaisesRegex(QALogError, "exceed sampled"):
            summarize_log(record(samples=10, no_plane=10))
        with self.assertRaisesRegex(QALogError, "fewer than ten"):
            summarize_log(record(samples=9, crossed=0, no_plane=0))

    def test_new_invalid_velocity_field_aggregates_with_legacy_rows(self):
        logs = (
            record(gate=2, samples=100, crossed=1, invalid_velocity=7)
            + "\n" +
            record(gate=2, samples=100, crossed=0, invalid_velocity=9)
            + "\n" +
            record(gate=2, samples=100, crossed=1)
        )
        report = summarize_log(logs)
        row = report["gate_summaries"][0]
        self.assertEqual(row["gate"], 2)
        self.assertEqual(row["windows"], 3)
        self.assertEqual(row["samples"], 300)
        self.assertEqual(row["invalid_velocity"], 16)
        self.assertFalse(report["unreal_build_or_gameplay_verified"])

    def test_invalid_velocity_column_contributes_to_sample_limit(self):
        with self.assertRaisesRegex(QALogError, "exceed sampled"):
            summarize_log(record(samples=55, no_plane=50,
                                 invalid_velocity=3, crossed=2, wrong=3))
        with self.assertRaisesRegex(QALogError, "uint32"):
            summarize_log(record(invalid_velocity=2 ** 32))

    def test_corrupt_velocity_column_is_rejected_instead_of_ignored(self):
        for suffix in ("oops", "-1", "1.5", "4294967296garbage"):
            with self.subTest(suffix=suffix):
                with self.assertRaisesRegex(QALogError, "malformed"):
                    summarize_log(record(invalid_velocity=suffix))
        with self.assertRaisesRegex(QALogError, "malformed"):
            summarize_log(record(invalid_velocity=2)
                          + " invalid-velocity=4")

    def test_legacy_rows_report_zero_velocity_without_claiming_evidence(self):
        old = summarize_log(record())
        self.assertEqual(old["gate_summaries"][0]["invalid_velocity"], 0)
        self.assertEqual(old["version"], "v0.5.4-source-development")
        self.assertFalse(old["source_authenticity_verified"])

    def test_summary_cannot_claim_engine_verification(self):
        report = summarize_log(record())
        self.assertIn("verify the installed Unreal game", report["warning"])


if __name__ == "__main__":
    unittest.main()
