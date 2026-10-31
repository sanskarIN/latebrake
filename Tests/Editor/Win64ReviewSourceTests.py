"""Disposable, source-only tests for the manual Win64 driving QA recorder.

No real Unreal assets, Windows game, hardware or runtime are exercised.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/Review-WindowsPlaytest.py"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class Win64ReviewSourceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="latebrake-review-fixture-")
        self.addCleanup(self.temp.cleanup)
        # macOS /var is commonly a symlink to /private/var.
        self.root = Path(self.temp.name).resolve()
        self.candidate = self.root / "candidate"
        self.candidate.mkdir()
        self.private = self.root / "private"
        self.private.mkdir()
        self.exe = self.candidate / "Latebrake.exe"
        self.manifest = self.candidate / "Latebrake-candidate-manifest.json"
        # Deliberately NOT an executable: the tool must never run these bytes.
        self.exe.write_bytes(b"MZ--SYNTHETIC-NOT-A-GAME--")
        # A schema-2 manifest with a single synthetic file. It is NOT UAT
        # output; the reviewer only checks byte identity and archive closure.
        self.manifest.write_text(json.dumps({
            "schema_version": 2,
            "project": "LateBrake",
            "platform": "Win64",
            "status": "UNVERIFIED_CANDIDATE_NOT_PLAYTESTED",
            "source_commit": "a" * 40,
            "source_tree_tracked_changes": False,
            "unreal_engine_version": "5.8",
            "unreal_engine_build_sha256": "b" * 64,
            "starter_map_sha256": "c" * 64,
            "created_utc": "2026-10-31T00:00:00Z",
            "files": [{
                "path": "Latebrake.exe",
                "bytes": self.exe.stat().st_size,
                "sha256": sha(self.exe.read_bytes()),
            }],
        }), encoding="utf-8")
        self.receipt = self.private / "session.json"
        self.receipt_data = {
            "schema_version": 1,
            "status": "UNVERIFIED_PERSONAL_RUNTIME_PROBE",
            "gameplay_verified": False,
            "source": "locally_launched_packaged_win64_candidate",
            "candidate_directory": str(self.candidate),
            "executable_path": str(self.exe),
            "executable_sha256": sha(self.exe.read_bytes()),
            "candidate_manifest_sha256": sha(self.manifest.read_bytes()),
            "process_exit_code": 0,
            "unreal_log_present": False,
            "startup_log_markers_pass": False,
        }
        self.write_receipt()
        self.checklist = self.private / "manual.json"
        self.evaluation = self.private / "review.json"

    def add_synthetic_cooked_file(self):
        # An additional inert byte fixture, never a cooked Unreal asset.
        fake = self.candidate / "Latebrake" / "Content" / "Paks" / "Synthetic.pak"
        fake.parent.mkdir(parents=True)
        fake.write_bytes(b"SYNTHETIC-ASSET-CONTENT-NOT-A-GAME")
        data = json.loads(self.manifest.read_text(encoding="utf-8"))
        data["files"].append({
            "path": fake.relative_to(self.candidate).as_posix(),
            "bytes": fake.stat().st_size,
            "sha256": sha(fake.read_bytes()),
        })
        self.manifest.write_text(json.dumps(data), encoding="utf-8")
        self.receipt_data["candidate_manifest_sha256"] = sha(self.manifest.read_bytes())
        self.write_receipt()
        return fake

    def set_schema2_log(self, contents: bytes | None = b"Unreal synthetic log -- not a real game"):
        # These are inert byte fixtures, never a real Unreal runtime.
        self.receipt_data["schema_version"] = 2
        logfile = self.receipt.with_suffix(".log")
        self.receipt_data["unreal_log_path"] = str(logfile)
        self.receipt_data["unreal_log_present"] = contents is not None
        self.receipt_data["startup_log_markers_pass"] = False
        if contents is None:
            self.receipt_data["unreal_log_sha256"] = None
            self.receipt_data["unreal_log_bytes"] = None
        else:
            logfile.write_bytes(contents)
            self.receipt_data["unreal_log_sha256"] = sha(contents)
            self.receipt_data["unreal_log_bytes"] = len(contents)
        self.write_receipt()
        return logfile

    def write_receipt(self):
        self.receipt.write_text(json.dumps(self.receipt_data), encoding="utf-8")

    def invoke(self, *arguments):
        return subprocess.run(
            [sys.executable, str(SCRIPT), *map(str, arguments)],
            capture_output=True, text=True, check=False, timeout=15,
        )

    def make_template(self):
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 0, result.stderr)
        return json.loads(self.checklist.read_text(encoding="utf-8"))

    def test_template_is_hash_linked_and_never_claims_playable(self):
        template = self.make_template()
        self.assertFalse(template["gameplay_verified"])
        self.assertEqual(template["status"], "MANUAL_WIN64_QA_UNVERIFIED")
        self.assertEqual(template["receipt_sha256"], sha(self.receipt.read_bytes()))
        self.assertEqual(len(template["checks"]), 9)
        self.assertTrue(all(x["result"] == "not_tested" for x in template["checks"].values()))
        output = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(output.returncode, 0, output.stderr)
        review = json.loads(self.evaluation.read_text(encoding="utf-8"))
        self.assertFalse(review["gameplay_verified"])
        self.assertFalse(review["ready_for_release"])
        self.assertFalse(review["all_checks_self_reported_pass"])
        self.assertEqual(review["manual_counts"]["not_tested"], 9)

    def test_all_manual_passes_are_still_not_a_certified_game(self):
        template = self.make_template()
        for check in template["checks"].values():
            check.update(result="pass", notes="Observed personally on an authorized local workstation.")
        template["environment"]["windows_version"] = "synthetic Windows fixture"
        self.checklist.write_text(json.dumps(template), encoding="utf-8")
        result = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(result.returncode, 0, result.stderr)
        review = json.loads(self.evaluation.read_text(encoding="utf-8"))
        self.assertTrue(review["all_checks_self_reported_pass"])
        self.assertFalse(review["gameplay_verified"])
        self.assertFalse(review["ready_for_release"])
        self.assertEqual(review["manual_counts"]["pass"], 9)

    def test_modified_candidate_exe_rejected(self):
        self.exe.write_bytes(self.exe.read_bytes() + b"tamper")
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("EXE SHA256", result.stderr)
        self.assertFalse(self.checklist.exists())

    def test_modified_manifest_rejected(self):
        self.manifest.write_bytes(self.manifest.read_bytes() + b"tamper")
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Manifest SHA256", result.stderr)
        self.assertFalse(self.checklist.exists())

    def test_different_runtime_receipt_cannot_reuse_checklist(self):
        self.make_template()
        self.receipt_data["runtime_seconds"] = 99
        self.write_receipt()
        result = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(result.returncode, 2)
        self.assertIn("different runtime session", result.stderr)
        self.assertFalse(self.evaluation.exists())

    def test_no_auto_pass_when_process_crashed(self):
        self.receipt_data["process_exit_code"] = 7
        self.write_receipt()
        self.make_template()
        result = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(result.returncode, 0, result.stderr)
        review = json.loads(self.evaluation.read_text(encoding="utf-8"))
        self.assertFalse(review["runtime_exit_ok"])
        self.assertFalse(review["ready_for_release"])

    def test_manual_pass_requires_descriptive_observation(self):
        template = self.make_template()
        first = next(iter(template["checks"].values()))
        first.update(result="pass", notes="ok")
        self.checklist.write_text(json.dumps(template), encoding="utf-8")
        result = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(result.returncode, 2)
        self.assertIn("descriptive observation", result.stderr)

    def test_cannot_add_or_remove_required_checks(self):
        template = self.make_template()
        template["checks"]["invented_automated_test"] = {"result": "pass", "notes": "I did not test this."}
        self.checklist.write_text(json.dumps(template), encoding="utf-8")
        result = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(result.returncode, 2)
        self.assertIn("cannot be missing", result.stderr)

    def test_user_cannot_assert_gameplay_certification(self):
        template = self.make_template()
        template["gameplay_verified"] = True
        self.checklist.write_text(json.dumps(template), encoding="utf-8")
        result = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(result.returncode, 2)
        self.assertIn("cannot assert gameplay certification", result.stderr)

    def test_cannot_write_review_into_the_sealed_candidate(self):
        target = self.candidate / "review.json"
        result = self.invoke("init", "--receipt", self.receipt, "--output", target)
        self.assertEqual(result.returncode, 2)
        self.assertIn("OUTSIDE", result.stderr)
        self.assertFalse(target.exists())

    def test_refuses_to_overwrite_evidence(self):
        self.make_template()
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Refusing to overwrite", result.stderr)

    def test_duplicate_receipt_keys_rejected(self):
        self.receipt.write_text('{"schema_version":1,"schema_version":2}', encoding="utf-8")
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Duplicate JSON property", result.stderr)


    def test_complete_synthetic_tree_can_be_reviewed_but_not_qualified(self):
        self.add_synthetic_cooked_file()
        template = self.make_template()
        self.assertFalse(template["gameplay_verified"])
        self.assertFalse(template["warning"].startswith("VERIFIED"))

    def test_tampered_cooked_asset_refuses_review_even_if_exe_unchanged(self):
        asset = self.add_synthetic_cooked_file()
        asset.write_bytes(asset.read_bytes() + b"TAMPER")
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("file length or SHA256 mismatch", result.stderr)
        self.assertFalse(self.checklist.exists())

    def test_missing_cooked_asset_refuses_review(self):
        asset = self.add_synthetic_cooked_file()
        asset.unlink()
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("missing listed files", result.stderr)

    def test_unlisted_cooked_asset_refuses_review(self):
        self.add_synthetic_cooked_file()
        unexpected = self.candidate / "unlisted.ini"
        unexpected.write_text("not in manifest", encoding="utf-8")
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Unexpected or duplicate candidate file", result.stderr)

    def test_invalid_manifest_file_paths_are_rejected(self):
        original = json.loads(self.manifest.read_text(encoding="utf-8"))
        for unsafe in ("../escape.bin", "/absolute.exe", "x\\\\bad", "a//b", "a/./b", "C:/other.exe"):
            with self.subTest(unsafe=unsafe):
                manifest = dict(original)
                manifest["files"] = [dict(original["files"][0], path=unsafe)]
                self.manifest.write_text(json.dumps(manifest), encoding="utf-8")
                self.receipt_data["candidate_manifest_sha256"] = sha(self.manifest.read_bytes())
                self.write_receipt()
                result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
                self.assertEqual(result.returncode, 2, result.stdout)
                self.assertIn("Invalid file path", result.stderr)
                self.assertFalse(self.checklist.exists())

    def test_case_colliding_manifest_entries_are_rejected(self):
        data = json.loads(self.manifest.read_text(encoding="utf-8"))
        duplicate = dict(data["files"][0])
        duplicate["path"] = "latebrake.EXE"
        data["files"].append(duplicate)
        self.manifest.write_text(json.dumps(data), encoding="utf-8")
        self.receipt_data["candidate_manifest_sha256"] = sha(self.manifest.read_bytes())
        self.write_receipt()
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Duplicate or case-colliding", result.stderr)

    def test_even_empty_linked_archive_directory_is_rejected(self):
        outside = self.root / "outside"
        outside.mkdir()
        link = self.candidate / "linked-assets"
        try:
            link.symlink_to(outside, target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("Creating directory links requires permissions on this Windows host")
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Linked archive entry", result.stderr)

    def test_modified_manifest_hash_fails_before_scanning_assets(self):
        data = json.loads(self.manifest.read_text(encoding="utf-8"))
        data["files"][0]["bytes"] = 0
        self.manifest.write_text(json.dumps(data), encoding="utf-8")
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Manifest SHA256", result.stderr)



    def test_version_two_log_integrity_is_confirmed_not_gameplay(self):
        self.set_schema2_log()
        self.make_template()
        result = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(result.returncode, 0, result.stderr)
        review = json.loads(self.evaluation.read_text(encoding="utf-8"))
        self.assertTrue(review["runtime_log_integrity_verified"])
        self.assertFalse(review["gameplay_verified"])
        self.assertFalse(review["ready_for_release"])
        self.assertFalse(review["startup_log_markers_pass"])

    def test_old_version_one_receipts_remain_usable_but_unbound(self):
        self.make_template()
        result = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(result.returncode, 0, result.stderr)
        review = json.loads(self.evaluation.read_text(encoding="utf-8"))
        self.assertFalse(review["runtime_log_integrity_verified"])
        self.assertFalse(review["ready_for_release"])

    def test_schema_two_crash_without_log_is_not_a_false_success(self):
        self.set_schema2_log(None)
        self.receipt_data["process_exit_code"] = 11
        self.write_receipt()
        self.make_template()
        result = self.invoke("evaluate", "--receipt", self.receipt,
                             "--checklist", self.checklist, "--output", self.evaluation)
        self.assertEqual(result.returncode, 0, result.stderr)
        review = json.loads(self.evaluation.read_text(encoding="utf-8"))
        self.assertFalse(review["runtime_exit_ok"])
        self.assertFalse(review["runtime_log_integrity_verified"])
        self.assertFalse(review["ready_for_release"])

    def test_changed_runtime_log_rejects_new_review(self):
        logfile = self.set_schema2_log()
        logfile.write_bytes(logfile.read_bytes() + b"tamper")
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Runtime log changed", result.stderr)
        self.assertFalse(self.checklist.exists())

    def test_missing_runtime_log_rejects_new_review(self):
        logfile = self.set_schema2_log()
        logfile.unlink()
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Recorded runtime log is missing", result.stderr)

    def test_renamed_log_cannot_be_substituted_from_another_session(self):
        logfile = self.set_schema2_log()
        renamed = logfile.with_name("different-session.log")
        logfile.rename(renamed)
        self.receipt_data["unreal_log_path"] = str(renamed)
        self.write_receipt()
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("does not belong to this receipt", result.stderr)

    def test_log_size_cannot_be_boolean_or_malformed(self):
        logfile = self.set_schema2_log()
        for invalid in (True, -1, 0, "42"):
            with self.subTest(invalid=invalid):
                self.receipt_data["unreal_log_bytes"] = invalid
                self.write_receipt()
                result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
                self.assertEqual(result.returncode, 2)
                self.assertFalse(self.checklist.exists())
        self.assertTrue(logfile.exists())

    def test_mismatched_runtime_log_hash_rejected(self):
        self.set_schema2_log()
        self.receipt_data["unreal_log_sha256"] = "0" * 64
        self.write_receipt()
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Runtime log changed", result.stderr)

    def test_log_markers_cannot_pass_when_log_was_absent(self):
        self.set_schema2_log(None)
        self.receipt_data["startup_log_markers_pass"] = True
        self.write_receipt()
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("absent runtime log", result.stderr)

    def test_unexpected_log_after_absent_log_receipt_is_rejected(self):
        logfile = self.set_schema2_log(None)
        logfile.write_bytes(b"unexpected later output")
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("log exists now", result.stderr)

    def test_linked_runtime_log_is_rejected(self):
        target = self.root / "other-runtime.log"
        target.write_bytes(b"synthetic")
        logfile = self.set_schema2_log(None)
        try:
            logfile.symlink_to(target)
        except (OSError, NotImplementedError):
            self.skipTest("Symlink creation unavailable on this host")
        self.receipt_data["unreal_log_present"] = True
        self.receipt_data["unreal_log_sha256"] = sha(target.read_bytes())
        self.receipt_data["unreal_log_bytes"] = target.stat().st_size
        self.write_receipt()
        result = self.invoke("init", "--receipt", self.receipt, "--output", self.checklist)
        self.assertEqual(result.returncode, 2)
        self.assertIn("does not belong to this receipt", result.stderr)



    def synthetic_race_log(self, finish=True):
        """Artificial text only; cannot qualify a cooked Unreal package."""
        lines = [
            "LogInit: synthetic fixture, not an Unreal runtime",
            "LogLatebrakeRaceAudit: Display: LATEBRAKE_RACE_AUDIT v=1 "
            "action=begin attempt=1 gates=4 laps=1 track=1 grid=-1",
        ]
        for i in range(4 if finish else 2):
            lap = 1 if i == 3 else 0
            lines.append(
                "LogLatebrakeRaceAudit: Display: LATEBRAKE_RACE_AUDIT v=1 "
                f"action=gate attempt=1 gate={i} lap={lap} next={(i + 1) % 4} "
                f"elapsed={i + 1}.000000 active={lap} finished={lap}"
            )
        return ("\n".join(lines) + "\n").encode("utf-8")

    def audit(self, *extra):
        return self.invoke("audit", "--receipt", self.receipt,
                           "--output", self.evaluation, *extra)

    def test_hash_bound_race_audit_is_reported_but_never_gameplay(self):
        log = self.set_schema2_log(self.synthetic_race_log())
        result = self.audit("--require-finish")
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(self.evaluation.read_text(encoding="utf-8"))
        self.assertEqual(report["status"], "HASH_BOUND_SOURCE_RACE_AUDIT_UNVERIFIED")
        self.assertTrue(report["runtime_log_integrity_verified"])
        self.assertEqual(report["runtime_log_sha256"], sha(log.read_bytes()))
        self.assertEqual(report["receipt_sha256"], sha(self.receipt.read_bytes()))
        self.assertEqual(report["executable_sha256"], sha(self.exe.read_bytes()))
        self.assertEqual(report["source_commit"], "a" * 40)
        self.assertEqual(report["unreal_engine_build_sha256"], "b" * 64)
        self.assertEqual(report["starter_map_sha256"], "c" * 64)
        self.assertEqual(report["candidate_file_count"], 1)
        self.assertEqual(report["race_audit"]["reported_finished_attempts"], 1)
        self.assertEqual(report["race_audit"]["accepted_gate_events"], 4)
        self.assertFalse(report["gameplay_verified"])
        self.assertFalse(report["source_authenticity_verified"])
        self.assertFalse(report["ready_for_release"])

    def test_audit_requires_actual_hashed_schema2_receipt(self):
        no_schema2 = self.audit()
        self.assertEqual(no_schema2.returncode, 2)
        self.assertIn("schema-2 receipt", no_schema2.stderr)
        self.set_schema2_log(None)
        missing_log = self.audit()
        self.assertEqual(missing_log.returncode, 2)
        self.assertIn("present, hashed runtime log", missing_log.stderr)
        self.assertFalse(self.evaluation.exists())

    def test_unfinished_race_is_never_falsely_completed(self):
        self.set_schema2_log(self.synthetic_race_log(finish=False))
        refused = self.audit("--require-finish")
        self.assertEqual(refused.returncode, 2)
        self.assertIn("no complete ordered race finish", refused.stderr)
        self.assertFalse(self.evaluation.exists())
        allowed = self.audit()
        self.assertEqual(allowed.returncode, 0, allowed.stderr)
        report = json.loads(self.evaluation.read_text(encoding="utf-8"))
        self.assertEqual(report["race_audit"]["accepted_gate_events"], 2)
        self.assertEqual(report["race_audit"]["reported_finished_attempts"], 0)
        self.assertFalse(report["ready_for_release"])

    def test_corrupt_ordered_race_event_rejected(self):
        bad = self.synthetic_race_log().replace(
            b"gate=1 lap=0 next=2", b"gate=3 lap=0 next=0")
        self.set_schema2_log(bad)
        result = self.audit("--require-finish")
        self.assertEqual(result.returncode, 2)
        self.assertIn("Race audit rejected", result.stderr)
        self.assertIn("out-of-order", result.stderr)
        self.assertFalse(self.evaluation.exists())

    def test_changed_runtime_log_blocks_report_even_if_events_remain_valid(self):
        log = self.set_schema2_log(self.synthetic_race_log())
        log.write_bytes(log.read_bytes() + b"\nLogTemp: unrelated change")
        result = self.audit("--require-finish")
        self.assertEqual(result.returncode, 2)
        self.assertIn("Runtime log changed", result.stderr)
        self.assertFalse(self.evaluation.exists())

    def test_swapping_another_session_log_blocks_race_report(self):
        self.set_schema2_log(self.synthetic_race_log())
        alternative = self.private / "other-session.log"
        alternative.write_bytes(self.synthetic_race_log())
        self.receipt_data["unreal_log_path"] = str(alternative)
        self.write_receipt()
        result = self.audit("--require-finish")
        self.assertEqual(result.returncode, 2)
        self.assertIn("does not belong to this receipt", result.stderr)

    def test_executable_and_cooked_file_changes_block_race_report(self):
        self.add_synthetic_cooked_file()
        self.set_schema2_log(self.synthetic_race_log())
        cooked = self.candidate / "Latebrake" / "Content" / "Paks" / "Synthetic.pak"
        cooked.write_bytes(cooked.read_bytes() + b"TAMPER")
        result = self.audit("--require-finish")
        self.assertEqual(result.returncode, 2)
        self.assertIn("Candidate file length or SHA256 mismatch", result.stderr)
        cooked.write_bytes(b"SYNTHETIC-ASSET-CONTENT-NOT-A-GAME")
        self.exe.write_bytes(self.exe.read_bytes() + b"TAMPER")
        result = self.audit("--require-finish")
        self.assertEqual(result.returncode, 2)
        self.assertIn("EXE SHA256", result.stderr)
        self.assertFalse(self.evaluation.exists())

    def test_failed_process_can_be_audited_but_never_claim_success(self):
        self.set_schema2_log(self.synthetic_race_log())
        self.receipt_data["process_exit_code"] = 13
        self.write_receipt()
        result = self.audit("--require-finish")
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(self.evaluation.read_text(encoding="utf-8"))
        self.assertFalse(report["runtime_exit_ok"])
        self.assertFalse(report["ready_for_release"])

    def test_no_output_inside_archive_or_existing_evidence(self):
        self.set_schema2_log(self.synthetic_race_log())
        forbidden = self.candidate / "race-review.json"
        result = self.invoke("audit", "--receipt", self.receipt,
                             "--output", forbidden, "--require-finish")
        self.assertEqual(result.returncode, 2)
        self.assertIn("OUTSIDE", result.stderr)
        self.assertFalse(forbidden.exists())
        self.assertEqual(self.audit("--require-finish").returncode, 0)
        attempt_overwrite = self.audit("--require-finish")
        self.assertEqual(attempt_overwrite.returncode, 2)
        self.assertIn("Refusing to overwrite", attempt_overwrite.stderr)

    def test_report_parses_same_bytes_it_rehashes(self):
        """Catch a future regression where audit checks one file, reads another."""
        source = SCRIPT.read_text(encoding="utf-8")
        audit = source.split("def audit_race(", 1)[1].split("def main()", 1)[0]
        for marker in (
            "log_bytes = raw_log.read_bytes()",
            "hashlib.sha256(log_bytes).hexdigest()",
            "log_bytes.decode(\"utf-8-sig\")",
            "manifest_bytes = manifest.read_bytes()",
            "hashlib.sha256(manifest_bytes).hexdigest()",
            "manifest_bytes.decode(\"utf-8-sig\")",
            "sha256(args.receipt) != receipt_hash",
        ):
            self.assertIn(marker, audit)
        self.assertLess(audit.index("hashlib.sha256(log_bytes).hexdigest()"),
                        audit.index("log_bytes.decode(\"utf-8-sig\")"))
        self.assertLess(audit.index("hashlib.sha256(manifest_bytes).hexdigest()"),
                        audit.index("manifest_bytes.decode(\"utf-8-sig\")"))

    def test_oversized_log_is_rejected_before_decode(self):
        log = self.set_schema2_log(self.synthetic_race_log())
        # sparse synthetic test fixture; never a real engine log.
        with log.open("ab") as stream:
            stream.truncate(32 * 1024 * 1024 + 1)
        payload = log.stat().st_size
        self.receipt_data["unreal_log_bytes"] = payload
        self.receipt_data["unreal_log_sha256"] = sha(log.read_bytes())
        self.write_receipt()
        result = self.audit()
        self.assertEqual(result.returncode, 2)
        self.assertIn("32 MiB", result.stderr)
        self.assertFalse(self.evaluation.exists())



if __name__ == "__main__":
    unittest.main()
