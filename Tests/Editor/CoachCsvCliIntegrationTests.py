"""Host-side smoke/abuse tests for the local CSV coaching inspector binary.

Set LATEBRAKE_COACH_INSPECTOR=/path/to/portable/binary to execute. These
fixtures are edited CSV, not authentic vehicle, hardware or Unreal evidence.
"""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

EXE = os.environ.get("LATEBRAKE_COACH_INSPECTOR", "")
HEADER = "kind,delta_seconds,speed_cmps,lateral_cmps,braking\n"


def fixture(sample_count=120, *, windows=False):
    lines = [HEADER]
    for i in range(sample_count):
        if windows and i and i % 40 == 0:
            lines.append("suspend,,,,\n")
        speed = 1300 if i % 3 else 1900
        lateral = 900 if i % 17 < 3 else 80
        brake = 1 if i % 13 < 4 else 0
        lines.append(f"sample,0.100,{speed},{lateral},{brake}\n")
    lines.append("end,,,,\n")
    return "".join(lines).encode("ascii")


@unittest.skipUnless(EXE, "Set LATEBRAKE_COACH_INSPECTOR to built portable executable")
class CoachCsvCliIntegrationTests(unittest.TestCase):
    def run_with(self, data, *, source=None):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "telemetry.csv"
            path.write_bytes(data)
            result = subprocess.run([EXE, str(path if source is None else source)],
                                    capture_output=True, text=True,
                                    timeout=12, check=False)
            self.assertEqual(path.read_bytes(), data,
                             "inspector must not modify the source file")
            return result

    def test_real_inspector_process_emits_typed_read_only_json(self):
        data = fixture()
        result = self.run_with(data)
        self.assertEqual(result.returncode, 0, result.stderr)
        parsed = json.loads(result.stdout)
        self.assertTrue(parsed["valid"])
        self.assertEqual(parsed["schema"], 1)
        self.assertEqual(parsed["kind"], "latebrake-local-coach-csv-audit")
        self.assertEqual(parsed["accepted_rows"], 120)
        self.assertEqual(parsed["suspension_rows"], 0)
        self.assertTrue(parsed["advice_allowed"])
        self.assertTrue(parsed["source_sampled_integrity"])
        self.assertEqual(len(parsed["fingerprint_fnv64"]), 16)
        for name in ("source_authenticated", "native_unreal_verified",
                     "race_timing_verified", "saved_data_read", "saved_data_written"):
            self.assertIs(parsed[name], False)
        self.assertIn("Editable user-supplied CSV", parsed["warning"])

    def test_short_sample_has_no_improvement_claim(self):
        result = self.run_with(fixture(3))
        self.assertEqual(result.returncode, 0, result.stderr)
        parsed = json.loads(result.stdout)
        self.assertEqual(parsed["accepted_rows"], 3)
        self.assertFalse(parsed["advice_allowed"])
        self.assertFalse(parsed["race_timing_verified"])

    def test_discontinuous_trace_never_grants_race_evidence(self):
        result = self.run_with(fixture(160, windows=True))
        self.assertEqual(result.returncode, 0, result.stderr)
        parsed = json.loads(result.stdout)
        self.assertEqual(parsed["suspension_rows"], 3)
        self.assertTrue(parsed["source_sampled_integrity"])
        self.assertFalse(parsed["source_authenticated"])

    def test_bad_rows_rejected_without_json_or_data_changes(self):
        for bad in (b"", b"wrong\n", HEADER.encode("ascii") + b"sample,0.4,4,0,0\n",
                    HEADER.encode("ascii") + b"sample,0.1,nan,0,0\n",
                    HEADER.encode("ascii") + b"sample,0.1,1000,0,0\0FOO\n",
                    HEADER.encode("ascii") + b"end,,,,\nend,,,,\n"):
            with self.subTest(data=repr(bad)):
                result = self.run_with(bad)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, "",
                                 "rejected input cannot output a false accepted report")

    def test_directory_and_missing_file_are_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            result = subprocess.run([EXE, temporary], capture_output=True, text=True)
            self.assertEqual(result.returncode, 66)
            result = subprocess.run([EXE, str(Path(temporary) / "missing.csv")],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 66)

    def test_symlink_is_not_followed(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            real = root / "fixture.csv"
            real.write_bytes(fixture())
            link = root / "linked.csv"
            try:
                link.symlink_to(real)
            except (OSError, NotImplementedError):
                self.skipTest("Symlink creation unavailable on this host")
            result = subprocess.run([EXE, str(link)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 66)
            self.assertEqual(real.read_bytes(), fixture())

    def compare_with(self, before, after):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            baseline = root / "baseline.csv"
            candidate = root / "candidate.csv"
            baseline.write_bytes(before)
            candidate.write_bytes(after)
            result = subprocess.run([EXE, "--compare", str(baseline), str(candidate)],
                                    capture_output=True, text=True,
                                    timeout=12, check=False)
            self.assertEqual(baseline.read_bytes(), before)
            self.assertEqual(candidate.read_bytes(), after)
            self.assertFalse(any(root.glob("*.json")),
                             "comparison must not write local evidence files")
            return result

    def test_comparison_reports_only_user_claimed_context_and_observed_deltas(self):
        baseline = fixture(300).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,2,race\n", 1)
        candidate = fixture(300).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,2,race\n", 1)
        candidate = candidate.replace(b"sample,0.100,1300,", b"sample,0.100,2100,")
        result = self.compare_with(baseline, candidate)
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertTrue(report["valid"])
        self.assertEqual(report["kind"], "latebrake-local-coach-csv-comparison")
        self.assertEqual(report["baseline"]["accepted_rows"], 300)
        self.assertEqual(report["candidate"]["accepted_rows"], 300)
        self.assertEqual(report["claimed_context"]["circuit_id"], 0)
        self.assertEqual(report["claimed_context"]["vehicle_id"], 2)
        self.assertIn("mean_speed_kmh", report["metrics"])
        self.assertGreater(report["metrics"]["mean_speed_kmh"]["delta"], 0)
        self.assertNotEqual(report["baseline"]["fingerprint_fnv64"],
                            report["candidate"]["fingerprint_fnv64"])
        for name in ("native_unreal_verified", "source_authenticated",
                     "race_timing_verified", "same_authenticated_track",
                     "same_authenticated_car", "competitive_improvement_verified",
                     "saved_data_read", "saved_data_written", "official_lap_evidence"):
            self.assertIs(report[name], False)

    def test_comparison_rejects_different_and_missing_context(self):
        complete = fixture(250).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,2,race\n", 1)
        different = fixture(250).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,3,race\n", 1)
        for data in (different, fixture(250)):
            with self.subTest(second=data[:80]):
                result = self.compare_with(complete, data)
                self.assertEqual(result.returncode, 3)
                self.assertEqual(result.stdout, "")
                self.assertIn("comparison refused", result.stderr)

    def test_quality_discloses_short_unfinished_context_free_trace(self):
        data = fixture(50).replace(b"end,,,,\n", b"")
        result = self.run_with(data)
        self.assertEqual(result.returncode, 0, result.stderr)
        quality = json.loads(result.stdout)["quality"]
        self.assertTrue(quality)
        self.assertFalse(quality["official_race_evidence"])
        self.assertFalse(quality["source_authenticated"])
        self.assertEqual(quality["sample_count"], 50)
        self.assertIn("EXPLICIT END MARKER ABSENT", " ".join(quality["notices"]))
        self.assertIn("FEWER THAN 120", " ".join(quality["notices"]))
        self.assertIn("USER-CLAIMED", " ".join(quality["notices"]))

    def test_quality_uses_existing_chassis_speed_band_data(self):
        data = fixture(220).replace(HEADER.encode(),
            HEADER.encode() + b"context,1,1,1,race\n", 1)
        result = self.run_with(data)
        self.assertEqual(result.returncode, 0, result.stderr)
        quality = json.loads(result.stdout)["quality"]
        self.assertIn("occupied_speed_bands", quality)
        self.assertGreaterEqual(quality["occupied_speed_bands"], 1)
        self.assertGreaterEqual(quality["speed_band_diversity"], 0)
        self.assertLessEqual(quality["speed_band_diversity"], 1)
        self.assertLessEqual(quality["dominant_band_concentration"], 1)
        self.assertNotIn("EXPLICIT END MARKER ABSENT", " ".join(quality["notices"]))
        self.assertEqual(json.loads(result.stdout)["claimed_context"]["vehicle_id"], 1)

    def test_comparison_refuses_low_coverage_and_bad_candidate(self):
        baseline = fixture(300).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,2,race\n", 1)
        short = fixture(3).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,2,race\n", 1)
        refused = self.compare_with(baseline, short)
        self.assertEqual(refused.returncode, 3)
        self.assertEqual(refused.stdout, "")
        bad = self.compare_with(baseline, b"not csv\n")
        self.assertEqual(bad.returncode, 2)
        self.assertEqual(bad.stdout, "")

    def test_comparison_rejects_symlink_as_second_input(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source.csv"
            source.write_bytes(fixture(250))
            linked = root / "linked.csv"
            try:
                linked.symlink_to(source)
            except (OSError, NotImplementedError):
                self.skipTest("Symlinks unavailable on host")
            run = subprocess.run([EXE, "--compare", str(source), str(linked)],
                                 capture_output=True, text=True, check=False)
            self.assertEqual(run.returncode, 66)
            self.assertEqual(run.stdout, "")
            self.assertEqual(source.read_bytes(), fixture(250))

    def test_output_fingerprint_changes_on_exact_source_bytes(self):
        a = json.loads(self.run_with(fixture()).stdout)
        b = json.loads(self.run_with(fixture().replace(b"0.100", b"0.10", 1)).stdout)
        self.assertEqual(a["accepted_rows"], b["accepted_rows"])
        self.assertNotEqual(a["fingerprint_fnv64"], b["fingerprint_fnv64"])


if __name__ == "__main__":
    unittest.main()


@unittest.skipUnless(EXE, "Set LATEBRAKE_COACH_INSPECTOR to portable executable")
class CoachCsvBatchCliIntegrationTests(unittest.TestCase):
    """Real portable process boundaries for bounded, read-only CSV cohorts."""

    def run_batch(self, contents, *, alias=None):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            originals = []
            paths = []
            for index, body in enumerate(contents):
                path = root / f"sample-{index:02d}.csv"
                path.write_bytes(body)
                paths.append(path)
                originals.append(body)
            arguments = [str(path) for path in paths]
            if alias is not None:
                arguments[alias[0]] = str(alias[1](root, paths))
            result = subprocess.run([EXE, "--batch", *arguments],
                                    capture_output=True, text=True,
                                    timeout=20, check=False)
            for path, data in zip(paths, originals):
                self.assertEqual(path.read_bytes(), data,
                                 "batch must not mutate original CSV evidence")
            self.assertFalse(list(root.glob("*.json")),
                             "batch JSON must be printed only, never saved")
            self.assertEqual(len(list(root.iterdir())), len(contents) +
                             (1 if alias is not None else 0))
            return result

    def test_two_valid_sources_have_distinct_rows_and_false_authority(self):
        first = fixture(220).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,2,race\n", 1)
        second = fixture(260).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,2,race\n", 1)
        result = self.run_batch([first, second])
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["kind"], "latebrake-local-coach-csv-batch")
        self.assertTrue(report["valid"])
        self.assertEqual(report["file_count"], 2)
        self.assertIn("NOT RANKED AS RACES", report["status"])
        self.assertTrue(report["all_share_claimed_context"])
        self.assertEqual([row["accepted_samples"] for row in report["entries"]],
                         [220, 260])
        self.assertEqual(report["repeated_input_fingerprint_pairs"], 0)
        self.assertEqual(report["pair_review"]["pairs"], 1)
        self.assertEqual(report["pair_review"]["compatible_claim_pairs"], 1)
        self.assertFalse(report["pair_review"]["authenticity_established"])
        self.assertFalse(report["pair_review"]["official_performance_ranked"])
        self.assertLessEqual(report["minimum_mean_speed_kmh"],
                             report["maximum_mean_speed_kmh"])
        for key in ("source_authenticated", "official_race_ranking",
                    "source_sessions_independent", "identical_track_conditions_verified",
                    "modifies_game_profile"):
            self.assertIs(report[key], False)

    def test_identical_bytes_report_possible_reuse_not_fraud(self):
        data = fixture(220).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,2,race\n", 1)
        result = self.run_batch([data, data, data])
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["repeated_input_fingerprint_pairs"], 3)
        self.assertEqual(report["pair_review"]["reused_input_pairs"], 3)
        self.assertEqual(report["pair_review"]["pairs"], 3)
        self.assertIn("REPEATED SOURCE MAY EXIST", report["status"])
        self.assertFalse(report["source_authenticated"])
        self.assertFalse(report["source_sessions_independent"])
        self.assertEqual(len(set(row["fingerprint_fnv64"]
                                 for row in report["entries"])), 1)

    def test_missing_metadata_is_not_equivalent_to_matching_metadata(self):
        one = fixture(240).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,1,2,race\n", 1)
        no_context = fixture(241)
        other = fixture(242).replace(HEADER.encode(),
            HEADER.encode() + b"context,2,1,2,race\n", 1)
        for data, expected in ((no_context, "WITHOUT OPTIONAL USER-CLAIMED CONTEXT"),
                               (other, "DIFFERENT CIRCUIT/EVENT/CAR/MODE")):
            with self.subTest(expected=expected):
                result = self.run_batch([one, data])
                self.assertEqual(result.returncode, 0, result.stderr)
                report = json.loads(result.stdout)
                self.assertIn(expected, report["status"])
                self.assertFalse(report["all_share_claimed_context"])
                key = ("missing_claim_pairs" if data == no_context else
                       "different_claim_pairs")
                self.assertEqual(report["pair_review"][key], 1)
                self.assertFalse(report["identical_track_conditions_verified"])

    def test_invalid_member_rejects_entire_batch_without_partial_json(self):
        valid = fixture(180)
        invalid = HEADER.encode() + b"sample,0.1,NaN,0,0\n"
        for bodies in ([invalid, valid], [valid, invalid], [valid, valid, invalid]):
            with self.subTest(count=len(bodies)):
                result = self.run_batch(bodies)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, "",
                                 "valid prefix must not leak a half-valid report")
                self.assertIn("rejected", result.stderr)

    def test_source_symlink_is_rejected_in_batch(self):
        def make_link(root, paths):
            link = root / "alias.csv"
            try:
                link.symlink_to(paths[0])
            except (OSError, NotImplementedError):
                self.skipTest("Symlink creation unavailable")
            return link
        result = self.run_batch([fixture(210), fixture(220)],
                                alias=(1, make_link))
        self.assertEqual(result.returncode, 66)
        self.assertEqual(result.stdout, "")
        self.assertIn("non-symlink", result.stderr)

    def test_arity_bounds_before_reading_files(self):
        for paths in ([], ["one.csv"], ["x.csv"] * 13):
            result = subprocess.run([EXE, "--batch", *paths],
                                    capture_output=True, text=True,
                                    timeout=10, check=False)
            self.assertEqual(result.returncode, 64)
            self.assertEqual(result.stdout, "")
            self.assertIn("Usage:", result.stderr)

    def test_twelve_distinct_files_preserve_all_source_rows(self):
        contents = [fixture(160 + i).replace(HEADER.encode(),
            HEADER.encode() + b"context,1,2,3,race\n", 1)
            for i in range(12)]
        result = self.run_batch(contents)
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["file_count"], 12)
        self.assertEqual(len(report["entries"]), 12)
        self.assertEqual([row["accepted_samples"] for row in report["entries"]],
                         list(range(160, 172)))
        self.assertEqual(report["repeated_input_fingerprint_pairs"], 0)
        self.assertEqual(report["pair_review"]["pairs"], 66)
        self.assertEqual(report["pair_review"]["compatible_claim_pairs"], 66)
        self.assertTrue(report["all_share_claimed_context"])
        self.assertTrue(all(row["claimed_context"] for row in report["entries"]))


@unittest.skipUnless(EXE, "Set LATEBRAKE_COACH_INSPECTOR to portable executable")
class CoachCsvWindowCliIntegrationTests(unittest.TestCase):
    def inspect(self, data):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "local-samples.csv"
            path.write_bytes(data)
            result = subprocess.run([EXE, str(path)], capture_output=True,
                                    text=True, timeout=15, check=False)
            self.assertEqual(path.read_bytes(), data)
            self.assertEqual(list(Path(temporary).iterdir()), [path])
            self.assertEqual(result.returncode, 0, result.stderr)
            return json.loads(result.stdout)

    def test_retained_window_rows_use_actual_chassis_sample_count(self):
        report = self.inspect(fixture(123))
        windows = report["retained_window_audit"]
        self.assertTrue(windows["valid"])
        self.assertEqual(windows["source_samples"], 123)
        self.assertEqual(windows["retained_samples"], 123)
        self.assertEqual(windows["retained_window_count"], 3)
        self.assertEqual(windows["complete_windows"], 2)
        self.assertEqual(windows["partial_windows"], 1)
        self.assertFalse(windows["older_windows_evicted"])
        self.assertTrue(windows["newest_first"])
        self.assertEqual([row["accepted_samples"] for row in windows["windows"]],
                         [23, 50, 50])
        self.assertEqual([row["complete"] for row in windows["windows"]],
                         [False, True, True])
        self.assertAlmostEqual(windows["retained_coverage_fraction"], 1.0)
        self.assertFalse(windows["official_lap_timing_verified"])
        self.assertFalse(windows["source_authenticity_verified"])
        self.assertFalse(windows["writes_save_data"])

    def test_retained_window_respects_source_suspension_epochs(self):
        report = self.inspect(fixture(160, windows=True))
        windows = report["retained_window_audit"]
        self.assertEqual(windows["observed_epoch_changes"], 3)
        self.assertGreater(windows["retained_window_count"], 3)
        rows = windows["windows"]
        for earlier, later in zip(rows, rows[1:]):
            self.assertGreater(earlier["sequence"], later["sequence"])
            self.assertGreaterEqual(earlier["epoch"], later["epoch"])
            self.assertGreaterEqual(earlier["started_seconds"] + 1.e-7,
                                    later["ended_seconds"])

    def test_ring_eviction_is_visible_in_json(self):
        report = self.inspect(fixture(1100))
        windows = report["retained_window_audit"]
        self.assertEqual(windows["retained_window_count"], 16)
        self.assertTrue(windows["older_windows_evicted"])
        self.assertLess(windows["retained_samples"], windows["source_samples"])
        self.assertLess(windows["retained_coverage_fraction"], 1.0)
        self.assertFalse(windows["official_lap_timing_verified"])

    def test_short_but_valid_source_has_one_partial_window(self):
        report = self.inspect(fixture(4).replace(b"end,,,,\n", b""))
        windows = report["retained_window_audit"]
        self.assertTrue(windows["valid"])
        self.assertEqual(windows["retained_window_count"], 1)
        self.assertEqual(windows["partial_windows"], 1)
        self.assertFalse(windows["older_windows_evicted"])
        self.assertEqual(windows["retained_samples"], 4)
        self.assertFalse(report["end_receipt"])


@unittest.skipUnless(EXE, "Set LATEBRAKE_COACH_INSPECTOR to portable executable")
class CoachCsvFurtherBoundaryIntegrationTests(unittest.TestCase):
    """Exercise cross-mode provenance and diagnostic boundaries of the real CLI."""

    def batch(self, bodies):
        with tempfile.TemporaryDirectory() as directory:
            paths = []
            for index, body in enumerate(bodies):
                path = Path(directory) / f"{index:02}.csv"
                path.write_bytes(body)
                paths.append(path)
            result = subprocess.run([EXE, "--batch", *(str(p) for p in paths)],
                                    capture_output=True, text=True,
                                    timeout=20, check=False)
            for path, body in zip(paths, bodies):
                self.assertEqual(path.read_bytes(), body)
            return result

    def single(self, body):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.csv"
            path.write_bytes(body)
            result = subprocess.run([EXE, str(path)], capture_output=True,
                                    text=True, timeout=20, check=False)
            self.assertEqual(path.read_bytes(), body)
            return result

    def test_six_context_groups_must_not_make_a_shared_racing_claim(self):
        bodies = [fixture(180 + index).replace(HEADER.encode(),
            HEADER.encode() +
            f"context,1,0,{index % 6},race\n".encode(), 1)
            for index in range(12)]
        result = self.batch(bodies)
        self.assertEqual(result.returncode, 0, result.stderr)
        data = json.loads(result.stdout)
        pairs = data["pair_review"]
        self.assertEqual(pairs["pairs"], 66)
        self.assertEqual(pairs["compatible_claim_pairs"], 6)
        self.assertEqual(pairs["different_claim_pairs"], 60)
        self.assertEqual(pairs["missing_claim_pairs"], 0)
        self.assertEqual(pairs["reused_input_pairs"], 0)
        self.assertFalse(data["all_share_claimed_context"])
        self.assertFalse(pairs["official_performance_ranked"])

    def test_per_pair_window_duration_mismatch_is_not_ranked(self):
        short = fixture(220).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,0,0,race\n", 1)
        long = fixture(1100).replace(HEADER.encode(),
            HEADER.encode() + b"context,0,0,0,race\n", 1)
        result = self.batch([short, long])
        self.assertEqual(result.returncode, 0, result.stderr)
        data = json.loads(result.stdout)
        pairs = data["pair_review"]
        self.assertEqual(pairs["pairs"], 1)
        self.assertEqual(pairs["insufficient_pairs"], 1)
        self.assertEqual(pairs["compatible_claim_pairs"], 0)
        self.assertFalse(pairs["authenticity_established"])

    def test_missing_context_pair_count_is_independent_of_file_order(self):
        one = fixture(220).replace(HEADER.encode(),
            HEADER.encode() + b"context,1,2,3,race\n", 1)
        missing = fixture(221)
        another = fixture(222).replace(HEADER.encode(),
            HEADER.encode() + b"context,1,2,3,race\n", 1)
        for group in ([one, missing, another], [missing, another, one],
                      [another, one, missing]):
            with self.subTest(order=[len(x) for x in group]):
                result = self.batch(group)
                self.assertEqual(result.returncode, 0, result.stderr)
                report = json.loads(result.stdout)
                self.assertEqual(report["pair_review"]["pairs"], 3)
                self.assertEqual(report["pair_review"]["missing_claim_pairs"], 2)
                self.assertEqual(report["pair_review"]["compatible_claim_pairs"], 1)
                self.assertFalse(report["all_share_claimed_context"])

    def test_unfinished_csv_discloses_partial_evidence_without_lap_timing(self):
        body = fixture(75).replace(b"end,,,,\n", b"")
        result = self.single(body)
        self.assertEqual(result.returncode, 0, result.stderr)
        data = json.loads(result.stdout)
        windows = data["retained_window_audit"]
        self.assertEqual(windows["source_samples"], 75)
        self.assertEqual(windows["complete_windows"], 1)
        self.assertEqual(windows["partial_windows"], 1)
        self.assertFalse(data["end_receipt"])
        self.assertFalse(windows["official_lap_timing_verified"])
        self.assertAlmostEqual(windows["retained_coverage_fraction"], 1.0)

    def test_changes_only_csv_text_fingerprint_not_sample_windows(self):
        original = fixture(148)
        differently_formatted = original.replace(b"sample,0.100,", b"sample,0.10,")
        first = self.single(original)
        second = self.single(differently_formatted)
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(second.returncode, 0, second.stderr)
        left, right = json.loads(first.stdout), json.loads(second.stdout)
        self.assertNotEqual(left["fingerprint_fnv64"], right["fingerprint_fnv64"])
        self.assertEqual(left["accepted_rows"], right["accepted_rows"])
        self.assertEqual(left["retained_window_audit"]["retained_samples"],
                         right["retained_window_audit"]["retained_samples"])
        self.assertEqual(left["retained_window_audit"]["retained_window_count"],
                         right["retained_window_audit"]["retained_window_count"])
        self.assertFalse(left["source_authenticated"])
        self.assertFalse(right["source_authenticated"])

    def test_invalid_value_cannot_output_apparently_valid_window_review(self):
        bad = (HEADER.encode() + b"sample,0.10,1000,0,0\n" +
               b"sample,0.10,2000,0,0\n" + b"sample,nan,3000,0,0\n")
        result = self.single(bad)
        self.assertEqual(result.returncode, 2)
        self.assertEqual(result.stdout, "")
        self.assertIn("rejected", result.stderr)

    def test_unavailable_requested_batch_source_has_no_partial_stdout(self):
        good = fixture(150)
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "existing.csv"
            source.write_bytes(good)
            missing = Path(directory) / "missing.csv"
            for order in ((source, missing), (missing, source)):
                with self.subTest(order=str(order[0])):
                    result = subprocess.run([EXE, "--batch", *(str(p) for p in order)],
                                            capture_output=True, text=True, timeout=15)
                    self.assertEqual(result.returncode, 66)
                    self.assertEqual(result.stdout, "")
                    self.assertEqual(source.read_bytes(), good)
