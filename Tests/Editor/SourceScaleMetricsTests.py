"""Source-scale audit must exclude tests, docs, generated and untracked files."""
import json
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "scripts/MeasureSourceScale.py"
MODULE = runpy.run_path(str(SCRIPT), run_name="source_scale_test")
metrics = MODULE["metrics"]
category = MODULE["category"]

class SourceScaleMetricsTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory(prefix="latebrake-loc-test-")
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        subprocess.run(["git", "init", "-q", str(self.root)], check=True)
        self.write("Source/MyCar.cpp", "int main() {}\n\n// explanation\n")
        self.write("Source/Engine.h", "#pragma once\r\nint speed;\r\n")
        self.write("Tests/Standalone/RaceTests.cpp", "test();\n")
        self.write("Experiments/Win32Preview/Demo.cpp", "preview();\n")
        self.write("scripts/check.py", "print('code')\n")
        self.write(".github/workflows/ci.yml", "name: ci\n")
        self.write("docs/overview.md", "This is not game source.\n")
        self.write("Content/fake_asset.cpp", "never counted\n")
        subprocess.run(["git", "-C", str(self.root), "add", "."], check=True)

    def write(self, name, contents):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(contents.encode("utf-8"))

    def test_source_count_stays_separate_from_tests_docs_and_preview(self):
        result = metrics(self.root)
        groups = result["categories"]
        self.assertEqual(groups["game_source"],
                         {"files": 2, "physical_lines": 5,
                          "nonblank_lines": 4})
        self.assertEqual(groups["test_code"]["physical_lines"], 1)
        self.assertEqual(groups["experimental_preview"]["physical_lines"], 1)
        self.assertEqual(groups["automation_code"]["physical_lines"], 1)
        self.assertEqual(groups["ci_workflows"]["physical_lines"], 1)
        self.assertEqual(result["remaining_game_source_lines"], 199995)
        self.assertFalse(result["goal_reached"])

    def test_untracked_and_deleted_files_do_not_count(self):
        self.write("Source/untracked.cpp", "a\nb\nc\n")
        # A deleted-in-worktree but still staged/indexed path must not count.
        (self.root / "Source/MyCar.cpp").unlink()
        result = metrics(self.root)
        self.assertEqual(result["categories"]["game_source"]["physical_lines"], 2)
        self.assertEqual(result["categories"]["game_source"]["files"], 1)

    def test_invalid_paths_are_rejected(self):
        for invalid in ("/Source/a.cpp", "../Source/a.cpp",
                        "Source/../a.cpp", "Source\\escape.cpp"):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                category(invalid)
        self.assertIsNone(category("docs/awesome.cpp"))
        self.assertIsNone(category("Source/image.png"))

    def test_repeatable_machine_readable_result(self):
        a = metrics(self.root)
        b = metrics(self.root)
        self.assertEqual(a, b)
        self.assertEqual(a["schema_version"], 1)
        self.assertEqual(json.loads(json.dumps(a)), a)
        self.assertEqual(a["skipped_tracked_symlinks"], 0)

if __name__ == "__main__":
    unittest.main()
