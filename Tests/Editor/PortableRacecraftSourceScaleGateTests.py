"""Verify a real +20,000 authored C++ source-line increase on this tree.

The frozen protected-main baseline (2026-11-21) is 87,792 qualifying
nonblank/noncomment tracked C++ production lines. Never substitute PR diff
counts, generated output, tests, docs or noncode comments for this metric.
"""
import json
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
FROZEN_MAIN = 87_792
MIN_ADDITIONAL = 20_000
REQUIRED = FROZEN_MAIN + MIN_ADDITIONAL

class PortableRacecraftScaleGate(unittest.TestCase):
    def test_unique_production_source_at_least_107792(self):
        result = subprocess.run(
            [sys.executable, str(ROOT / "scripts" / "MeasureSourceScale.py"),
             "--root", str(ROOT), "--json"],
            cwd=ROOT, capture_output=True, text=True,
            encoding="utf-8", check=True,
        )
        audit = json.loads(result.stdout)
        self.assertEqual(audit["goal_metric"],
                         "game_source.nonblank_noncomment_lines")
        actual = audit["categories"]["game_source"][
            "nonblank_noncomment_lines"]
        self.assertGreaterEqual(
            actual, REQUIRED,
            f"Audited production C++ = {actual:,}; threshold "
            f"{REQUIRED:,}; missing {max(0, REQUIRED-actual):,}. "
            "No credit for tests/docs/comments/duplicate source.")
        self.assertEqual(audit["schema_version"], 2)

if __name__ == "__main__":
    unittest.main()
