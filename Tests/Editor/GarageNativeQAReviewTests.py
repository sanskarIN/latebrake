"""Inert source fixtures only. Never runs an Unreal game."""
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
sys.path.insert(0, str(ROOT / "scripts"))
import garage_native_qa as policy


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class GarageNativeQAReviewTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix="garage-qa-fixture-")
        self.addCleanup(temp.cleanup)
        root = Path(temp.name).resolve()
        self.candidate = root / "candidate"
        self.private = root / "private"
        self.candidate.mkdir()
        self.private.mkdir()
        self.exe = self.candidate / "Latebrake.exe"
        self.exe.write_bytes(b"SYNTHETIC TEST BYTES NOT AN EXECUTABLE")
        self.manifest = self.candidate / "Latebrake-candidate-manifest.json"
        self.source = "a" * 40
        self.manifest.write_text(json.dumps({
            "schema_version": 2, "project": "LateBrake", "platform": "Win64",
            "status": "UNVERIFIED_CANDIDATE_NOT_PLAYTESTED",
            "source_commit": self.source, "source_tree_tracked_changes": False,
            "unreal_engine_version": "5.8", "unreal_engine_build_sha256": "b"*64,
            "starter_map_sha256": "c"*64,
            "files": [{"path": self.exe.name, "bytes": self.exe.stat().st_size,
                       "sha256": digest(self.exe)}],
        }), encoding="utf-8")
        self.receipt = self.private / "session.json"
        self.log = self.receipt.with_suffix(".log")
        self.log.write_bytes(b"SYNTHETIC LOG, NOT UNREAL")
        self.receipt_data = {
            "schema_version": 2, "status": "UNVERIFIED_PERSONAL_RUNTIME_PROBE",
            "gameplay_verified": False, "source": "locally_launched_packaged_win64_candidate",
            "candidate_directory": str(self.candidate), "executable_path": str(self.exe),
            "executable_sha256": digest(self.exe),
            "candidate_manifest_sha256": digest(self.manifest),
            "candidate_integrity_at_exit_pass": True, "candidate_integrity_at_exit_error": None,
            "process_exit_code": 0, "unreal_log_present": True, "startup_log_markers_pass": True,
            "unreal_log_path": str(self.log), "unreal_log_sha256": digest(self.log),
            "unreal_log_bytes": self.log.stat().st_size,
        }
        self.save_receipt()
        self.checklist = self.private / "garage.json"
        self.summary = self.private / "summary.json"

    def save_receipt(self):
        self.receipt.write_text(json.dumps(self.receipt_data), encoding="utf-8")

    def invoke(self, command, output, source=None):
        args = [sys.executable, str(SCRIPT), command, "--receipt", str(self.receipt),
                "--output", str(output), "--expected-source", source or self.source]
        if command == "garage-evaluate":
            args += ["--checklist", str(self.checklist)]
        return subprocess.run(args, capture_output=True, text=True, timeout=15, check=False)

    def init(self):
        result = self.invoke("garage-init", self.checklist)
        self.assertEqual(result.returncode, 0, result.stderr)
        return json.loads(self.checklist.read_text(encoding="utf-8"))

    def write_checks(self, value):
        self.checklist.write_text(json.dumps(value), encoding="utf-8")

    def evaluate(self):
        result = self.invoke("garage-evaluate", self.summary)
        self.assertEqual(result.returncode, 0, result.stderr)
        return json.loads(self.summary.read_text(encoding="utf-8"))

    def test_matrix_covers_each_actual_choice_and_viewport(self):
        checks = policy.scenarios()
        for group, count in (("identity",6), ("paint",48), ("stripe",18), ("rim",18),
                             ("camera",24), ("layout",6), ("integration",18)):
            self.assertEqual(sum(v["group"] == group for v in checks.values()), count)
        self.assertEqual(len(checks),138)
        for car in range(6):
            for rim in range(3):
                self.assertIn(f"car_{car}_rim_{rim}", checks)

    def test_init_and_evaluate_unverified_unperformed_matrix(self):
        template = self.init()
        self.assertEqual(template["identity"]["source_commit"], self.source)
        self.assertEqual(template["identity"]["runtime_log_sha256"], digest(self.log))
        self.assertTrue(all(c["result"] == "not_tested" for c in template["checks"].values()))
        summary = self.evaluate()
        self.assertEqual(summary["manual_observations"]["counts"]["not_tested"],138)
        self.assertIn("manual_observations_incomplete_or_failed",summary["blocking_observations"])
        self.assertFalse(summary["ready_for_release"])
        self.assertFalse(summary["gameplay_verified"])

    def test_all_self_reported_passes_never_authorize_release(self):
        template = self.init()
        for key in template["environment"]:
            template["environment"][key] = "Synthetic fixture"
        for check in template["checks"].values():
            check.update(result="pass", notes="Synthetic observation, not real playtesting.")
        self.write_checks(template)
        summary = self.evaluate()
        self.assertEqual(summary["blocking_observations"],[])
        self.assertTrue(summary["manual_observations"]["all_checks_self_reported_pass"])
        self.assertFalse(summary["source_authenticity_verified"])
        self.assertFalse(summary["gameplay_verified"])
        self.assertFalse(summary["ready_for_release"])

    def test_wrong_source_refuses_even_hash_consistent_manifest(self):
        result = self.invoke("garage-init",self.checklist,"d"*40)
        self.assertEqual(result.returncode,2)
        self.assertIn("exact reviewed SHA",result.stderr)
        self.assertFalse(self.checklist.exists())

    def test_log_changed_receipt_or_archive_blocks(self):
        self.log.write_bytes(b"tampered")
        self.assertEqual(self.invoke("garage-init",self.checklist).returncode,2)
        self.assertFalse(self.checklist.exists())

    def test_no_exit_seal_refuses(self):
        del self.receipt_data["candidate_integrity_at_exit_pass"]
        del self.receipt_data["candidate_integrity_at_exit_error"]
        self.save_receipt()
        self.assertEqual(self.invoke("garage-init",self.checklist).returncode,2)

    def test_crash_is_a_blocking_observation_not_release(self):
        self.receipt_data["process_exit_code"]=9
        self.receipt_data["startup_log_markers_pass"]=False
        self.save_receipt()
        self.init()
        summary=self.evaluate()
        self.assertIn("runtime_exit_not_zero",summary["blocking_observations"])
        self.assertIn("startup_markers_not_passing",summary["blocking_observations"])

    def test_cannot_change_scenario_definition_or_forge_status(self):
        original=self.init()
        for mode in ("missing","new","definition","short_note","release","other_sha"):
            with self.subTest(mode=mode):
                data=json.loads(json.dumps(original))
                key=next(iter(data["checks"]))
                if mode=="missing":
                    del data["checks"][key]
                elif mode=="new":
                    data["checks"]["invented"]={}
                elif mode=="definition":
                    data["checks"][key]["observe"]="Fake check"
                elif mode=="short_note":
                    data["checks"][key].update(result="pass",notes="ok")
                elif mode=="release":
                    data["ready_for_release"]=True
                else:
                    data["identity"]["source_commit"]="f"*40
                self.write_checks(data)
                result=self.invoke("garage-evaluate",self.summary)
                self.assertEqual(result.returncode,2,result.stderr)
                self.assertFalse(self.summary.exists())

    def test_cross_session_and_overwrite_refuse(self):
        self.init()
        self.assertEqual(self.invoke("garage-init",self.checklist).returncode,2)
        self.receipt_data["runtime_seconds"]=42
        self.save_receipt()
        self.assertEqual(self.invoke("garage-evaluate",self.summary).returncode,2)

    def test_no_output_inside_sealed_candidate(self):
        self.assertEqual(self.invoke("garage-init",self.candidate/"review.json").returncode,2)
        self.assertFalse((self.candidate/"review.json").exists())


if __name__=="__main__":
    unittest.main()
