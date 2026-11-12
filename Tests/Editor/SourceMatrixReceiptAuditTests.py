"""Adversarial fixtures for offline *untrusted* GitHub source check receipts."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "audit_saved_source_matrix.py"
spec = importlib.util.spec_from_file_location("saved_matrix_audit", SCRIPT)
module = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(module)

HEAD = "a" * 40
OTHER = "b" * 40


def check(name, conclusion="success", status="completed", head=HEAD):
    return {
        "name": name, "status": status, "conclusion": conclusion,
        "head_sha": head,
    }


def full_payload():
    return {"check_runs": [check(name) for name in module.REQUIRED_CHECKS] +
            [check("Deploy official Pages documentation", "skipped")]}


class SourceMatrixReceiptAuditTests(unittest.TestCase):
    def test_complete_saved_source_matrix_does_not_certify_game(self):
        report = module.analyze_saved_source_matrix(full_payload(), HEAD)
        self.assertTrue(report["source_report_structurally_all_green"])
        self.assertFalse(report["ci_authenticity_verified"])
        self.assertFalse(report["native_unreal_build_verified"])
        self.assertFalse(report["installed_gameplay_verified"])
        self.assertEqual(report["missing_required"], [])
        self.assertEqual(report["failed_or_not_success"], [])

    def test_queued_macos_is_not_success(self):
        payload = full_payload()
        payload["check_runs"][2] = check(
            "Portable C++20 tests / macos-latest", None, "queued",
        )
        report = module.analyze_saved_source_matrix(payload, HEAD)
        self.assertFalse(report["source_report_structurally_all_green"])
        self.assertIn("Portable C++20 tests / macos-latest", report["pending"])

    def test_in_progress_macos_is_not_success(self):
        payload = full_payload()
        payload["check_runs"][2] = check(
            "Portable C++20 tests / macos-latest", None, "in_progress",
        )
        self.assertFalse(
            module.analyze_saved_source_matrix(payload, HEAD)
            ["source_report_structurally_all_green"]
        )

    def test_wrong_sha_blocks_despite_green_checks(self):
        payload = full_payload()
        payload["check_runs"][1]["head_sha"] = OTHER
        report = module.analyze_saved_source_matrix(payload, HEAD)
        self.assertFalse(report["source_report_structurally_all_green"])
        self.assertEqual(report["wrong_source_commit"],
                         ["Portable C++20 tests / ubuntu-latest"])

    def test_missing_required_source_matrix_job_blocks(self):
        payload = full_payload()
        payload["check_runs"] = payload["check_runs"][1:]
        report = module.analyze_saved_source_matrix(payload, HEAD)
        self.assertIn("Portable C++20 tests / windows-latest",
                      report["missing_required"])
        self.assertFalse(report["source_report_structurally_all_green"])

    def test_failure_and_cancel_are_not_success(self):
        for value in ("failure", "cancelled", "timed_out", "neutral", "skipped"):
            with self.subTest(value=value):
                payload = full_payload()
                payload["check_runs"][0]["conclusion"] = value
                report = module.analyze_saved_source_matrix(payload, HEAD)
                self.assertFalse(report["source_report_structurally_all_green"])
                self.assertIn("Portable C++20 tests / windows-latest",
                              report["failed_or_not_success"])

    def test_duplicate_job_name_is_fatal(self):
        payload = full_payload()
        payload["check_runs"].append(check(module.REQUIRED_CHECKS[0]))
        with self.assertRaisesRegex(ValueError, "duplicate check name"):
            module.analyze_saved_source_matrix(payload, HEAD)

    def test_unknown_checks_are_reported_not_counted(self):
        payload = full_payload()
        payload["check_runs"].append(check("Random Not A Real CI Job"))
        report = module.analyze_saved_source_matrix(payload, HEAD)
        self.assertEqual(report["unrecognized_checks"],
                         ["Random Not A Real CI Job"])
        self.assertTrue(report["source_report_structurally_all_green"])

    def test_completed_without_conclusion_is_invalid(self):
        payload = full_payload()
        payload["check_runs"][0]["conclusion"] = None
        with self.assertRaisesRegex(ValueError, "invalid completed conclusion"):
            module.analyze_saved_source_matrix(payload, HEAD)

    def test_queued_with_fake_conclusion_is_invalid(self):
        payload = full_payload()
        payload["check_runs"][0]["status"] = "queued"
        with self.assertRaisesRegex(ValueError, "unfinished check"):
            module.analyze_saved_source_matrix(payload, HEAD)

    def test_reject_boolean_status_or_malformed_sha(self):
        payload = full_payload()
        payload["check_runs"][0]["status"] = True
        with self.assertRaisesRegex(ValueError, "unsupported"):
            module.analyze_saved_source_matrix(payload, HEAD)
        payload = full_payload()
        payload["check_runs"][0]["head_sha"] = "abc"
        with self.assertRaisesRegex(ValueError, "malformed"):
            module.analyze_saved_source_matrix(payload, HEAD)

    def test_reject_duplicate_json_properties_before_audit(self):
        with tempfile.TemporaryDirectory(prefix="latebrake-source-receipt-") as directory:
            path = Path(directory) / "duplicate.json"
            path.write_text('{"check_runs":[],"check_runs":[]}', encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "duplicate JSON key"):
                module.read_saved_checks(path)

    def test_cli_uses_only_saved_json_and_reports_pending(self):
        payload = full_payload()
        payload["check_runs"][2] = check(
            "Portable C++20 tests / macos-latest", None, "queued",
        )
        with tempfile.TemporaryDirectory(prefix="latebrake-source-receipt-") as directory:
            path = Path(directory) / "checks.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--checks", str(path),
                 "--expected-head", HEAD],
                text=True, capture_output=True, timeout=15, check=False,
            )
            self.assertEqual(result.returncode, 2, result.stderr)
            output = json.loads(result.stdout)
            self.assertFalse(output["source_report_structurally_all_green"])
            self.assertIn("macos-latest", " ".join(output["pending"]))

    def test_cli_all_green_still_has_untrusted_status(self):
        with tempfile.TemporaryDirectory(prefix="latebrake-source-receipt-") as directory:
            path = Path(directory) / "checks.json"
            path.write_text(json.dumps(full_payload()), encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--checks", str(path),
                 "--expected-head", HEAD],
                text=True, capture_output=True, timeout=15, check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            output = json.loads(result.stdout)
            self.assertFalse(output["ci_authenticity_verified"])
            self.assertFalse(output["installed_gameplay_verified"])

    def test_no_network_or_engine_execution(self):
        source = SCRIPT.read_text(encoding="utf-8")
        for forbidden in ("urllib.request", "requests.get", "subprocess.run",
                          "unreal.Editor", "gh pr merge"):
            self.assertNotIn(forbidden, source)


if __name__ == "__main__":
    unittest.main()
