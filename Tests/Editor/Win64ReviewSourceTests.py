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
        self.root = Path(self.temp.name)
        self.candidate = self.root / "candidate"
        self.candidate.mkdir()
        self.private = self.root / "private"
        self.private.mkdir()
        self.exe = self.candidate / "Latebrake.exe"
        self.manifest = self.candidate / "Latebrake-candidate-manifest.json"
        # Deliberately NOT an executable: the tool must never run these bytes.
        self.exe.write_bytes(b"MZ--SYNTHETIC-NOT-A-GAME--")
        self.manifest.write_bytes(b'{"synthetic_only":true}')
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


if __name__ == "__main__":
    unittest.main()
