"""Source-only guards for actual Editor-only manual compilation workflow.

Passing hosted CI is not an Unreal Engine compilation, cook or playable game.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
def read(p):
    return (ROOT / p).read_text(encoding="utf-8")

class UnrealEditorCompileGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.wf = read(".github/workflows/compile-unreal-editor.yml")
        cls.compile = read("scripts/Compile-UnrealEditor.ps1")
        cls.ci = read(".github/workflows/core-tests.yml")
        cls.fixture = read("scripts/Test-EditorCompileGatePolicy.ps1")

    def test_workflow_requires_manual_exact_main_and_trusted_runner(self):
        for token in ("  workflow_dispatch:", "contents: read",
                      "github.ref == 'refs/heads/main'",
                      "github.event_name == 'workflow_dispatch'",
                      "github.repository == 'sanskarIN/latebrake'",
                      "runs-on: [self-hosted, Windows, X64, unreal-5-8]",
                      "persist-credentials: false",
                      "Compile-UnrealEditor.ps1", "cancel-in-progress: false"):
            self.assertIn(token, self.wf)
        self.assertEqual(self.wf.count("github.sha"), 2)
        self.assertNotRegex(self.wf, r"(?m)^  (?:pull_request|push):")
        for forbidden in ("actions/upload-artifact", "RunUAT.bat",
                          "BuildCookRun", "gh release create", "contents: write"):
            self.assertNotIn(forbidden, self.wf)

    def test_compile_script_guards_both_source_and_engine_snapshot(self):
        for token in ("Test-UnrealWorkstationPrerequisites.ps1",
                      "Build-Editor.ps1", "rev-parse --verify HEAD",
                      "-ExpectedCommit $sourceCommit",
                      "-ExpectedSha256 $engineHash",
                      "NOT VERIFIED"):
            self.assertIn(token, self.compile)
        self.assertEqual(self.compile.count("Test-SourceProvenance.ps1"), 2)
        self.assertEqual(self.compile.count("Test-EngineBuildSnapshot.ps1"), 2)
        self.assertLess(self.compile.index("Test-UnrealWorkstationPrerequisites.ps1"),
                        self.compile.index("Build-Editor.ps1"))
        for forbidden in ("Build-PreRelease.ps1", "RunUAT.bat",
                          "Start-Process", "Set-Content", "gh release"):
            self.assertNotIn(forbidden, self.compile)

    def test_negative_fixture_in_windows_ci(self):
        self.assertIn("Test-EditorCompileGatePolicy.ps1", self.ci)
        self.assertIn("run: ./scripts/Test-EditorCompileGatePolicy.ps1", self.ci)
        self.assertIn("Missing licensed UE was wrongly accepted", self.fixture)
        self.assertIn("NO UE executed", self.fixture)
