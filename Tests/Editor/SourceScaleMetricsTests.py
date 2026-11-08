"""Source-scale audit must exclude tests, docs, generated and untracked files."""
import json
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[2] / "scripts/MeasureSourceScale.py"
MODULE = runpy.run_path(str(SCRIPT), run_name="source_scale_test")
metrics = MODULE["metrics"]
category = MODULE["category"]
count_code_lines = MODULE["count_code_lines"]

class SourceScaleFixture(unittest.TestCase):
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

class SourceScaleMetricsTests(SourceScaleFixture):
    def test_source_count_stays_separate_from_tests_docs_and_preview(self):
        result = metrics(self.root)
        groups = result["categories"]
        self.assertEqual(groups["game_source"],
                         {"files": 2, "physical_lines": 5,
                          "nonblank_lines": 4, "nonblank_noncomment_lines": 3})
        self.assertEqual(groups["test_code"]["physical_lines"], 1)
        self.assertEqual(groups["experimental_preview"]["physical_lines"], 1)
        self.assertEqual(groups["automation_code"]["physical_lines"], 1)
        self.assertEqual(groups["ci_workflows"]["physical_lines"], 1)
        self.assertEqual(result["remaining_game_source_lines"], 199997)
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
        self.assertEqual(a["schema_version"], 2)
        self.assertEqual(json.loads(json.dumps(a)), a)
        self.assertEqual(a["skipped_tracked_symlinks"], 0)


class ProductionNoncommentMilestoneTests(SourceScaleFixture):
    def stage(self):
        subprocess.run(['git', '-C', str(self.root), 'add', '.'], check=True)

    def test_comments_never_satisfy_production_milestone(self):
        self.write('Source/padding.cpp', '// padding\n' * 200_000)
        self.stage()
        result = metrics(self.root)
        self.assertGreater(result['categories']['game_source']['physical_lines'], 200_000)
        self.assertEqual(result['categories']['game_source']['nonblank_noncomment_lines'], 3)
        self.assertFalse(result['goal_reached'])
        self.assertEqual(result['remaining_game_source_lines'], 199_997)
        self.assertEqual(result['milestone'], 'v0.6.0 pre-release')

    def test_build_rules_generated_vendor_and_exact_duplicates_excluded(self):
        self.write('Source/Latebrake.Target.cs', 'using UnrealBuildTool;\n// not gameplay\n')
        for name in ('Source/vendor/fake.cpp', 'Source/ThirdParty/fake.cpp',
                     'Source/Intermediate/fake.cpp', 'Source/Tests/fake.cpp',
                     'Source/Car.generated.h', 'Source/Car.gen.cpp'):
            self.write(name, 'padding\n' * 10)
        self.write('Source/copy.cpp', (self.root / 'Source/MyCar.cpp').read_text())
        self.stage()
        result = metrics(self.root)
        self.assertEqual(result['categories']['game_source']['nonblank_noncomment_lines'], 3)
        self.assertEqual(result['categories']['unreal_build_rules']['nonblank_noncomment_lines'], 1)
        self.assertEqual(result['excluded_exact_duplicate_game_source_files'], 1)

    def test_lexical_comments_do_not_erase_literal_content(self):
        cases = [
            ('// comment\n/* block\n comment */\n', 0),
            ('int x; /* comment\n still comment */ int y;\n', 2),
            ('const char* url = "https://site/*path*/"; // trailing\n// comment\n', 1),
            ("auto x = '\\''; // comment\n/* ignored */\n", 1),
            ("auto n = 1'000; // comment\n/* ignored */\n", 1),
            ('auto r = u8R"tag(//literal\n/*literal*/\n\n)tag";\n//ignored\n', 3),
            ('auto s = "escaped\\\"//literal";\n// ignored\n', 1),
            ('\ufeff// BOM is not code\nint x;\n', 1),
            ('/* one *//* two */ int x; // end', 1),
            ('int x;\r\n// windows\r\nint y;', 2),
        ]
        for text, expected in cases:
            with self.subTest(text=text):
                self.assertEqual(count_code_lines(text), expected)

    def test_cpp_spliced_comments_keep_physical_line_mapping(self):
        self.assertEqual(count_code_lines('// comment\\\nstill comment\nint x;'), 1)
        self.assertEqual(count_code_lines('/\\\n/ comment\nint x;'), 1)
        self.assertEqual(count_code_lines('in\\\nt x; // comment\n'), 2)
        self.assertEqual(count_code_lines('/* comment *\\\n/ int x;'), 1)

    def test_raw_literals_revert_cpp_line_splicing(self):
        self.assertEqual(count_code_lines('auto r = R"(\n\\\n)";'), 3)
        self.assertEqual(count_code_lines('auto r = R"t(\n)t\\\n"\n// literal\n)t";'), 5)

    def test_quantity_boundary_uses_noncomment_metric(self):
        with patch.dict(metrics.__globals__, {"GOAL_LINES": 3}):
            self.assertTrue(metrics(self.root)['goal_reached'])
            self.assertEqual(metrics(self.root)['remaining_game_source_lines'], 0)
        with patch.dict(metrics.__globals__, {"GOAL_LINES": 4}):
            self.assertFalse(metrics(self.root)['goal_reached'])

    def test_symlinked_directory_is_not_traversed(self):
        try:
            (self.root / 'Source/linked').symlink_to(
                self.root / 'Tests/Standalone', target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest('symlinks unavailable')
        with patch.dict(metrics.__globals__, {
                'tracked_paths': lambda root: ['Source/linked/RaceTests.cpp']}):
            result = metrics(self.root)
        self.assertEqual(result['skipped_tracked_symlinks'], 1)
        self.assertEqual(result['categories']['game_source']['nonblank_noncomment_lines'], 0)

    def test_csharp_verbatim_and_raw_strings_preserve_comment_text(self):
        self.assertEqual(count_code_lines('var s = @"//string\n""/*string*/";\n// comment\n', csharp=True), 2)
        self.assertEqual(count_code_lines('var s = """\n//string\n/*string*/\n""";\n// comment\n', csharp=True), 4)

    def test_milestone_cli_is_opt_in_and_machine_readable_on_failure(self):
        regular = subprocess.run([sys.executable, str(SCRIPT), '--root', str(self.root), '--json'],
                                 capture_output=True, text=True)
        self.assertEqual(regular.returncode, 0)
        gate = subprocess.run([sys.executable, str(SCRIPT), '--root', str(self.root),
                               '--json', '--require-milestone'], capture_output=True, text=True)
        self.assertEqual(gate.returncode, 1)
        self.assertEqual(json.loads(gate.stdout), json.loads(regular.stdout))

    def test_tracked_symlink_parent_cannot_escape_source_root(self):
        try:
            (self.root / 'Source/link.cpp').symlink_to(self.root / 'Tests/Standalone/RaceTests.cpp')
        except (OSError, NotImplementedError):
            self.skipTest('symlinks unavailable')
        self.stage()
        result = metrics(self.root)
        self.assertEqual(result['skipped_tracked_symlinks'], 1)
        self.assertEqual(result['categories']['game_source']['nonblank_noncomment_lines'], 3)


if __name__ == "__main__":
    unittest.main()
