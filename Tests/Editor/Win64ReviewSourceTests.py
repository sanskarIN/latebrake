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



if __name__ == "__main__":
    unittest.main()
