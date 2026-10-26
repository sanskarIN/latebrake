"""Fail-closed source contracts for the *manual*, licensed UE 5.8 runner.

No source-level test here compiles Unreal/UHT, cooks a real original map,
produces packages or certifies installed gameplay QA.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
WF = ROOT / ".github/workflows/build-unreal-candidate.yml"
DOC = ROOT / "docs/UE58_WORKSTATION_QA.md"
BUILD_SCRIPT = ROOT / "scripts/Build-PlayableCandidate.ps1"
INLINE_SCRIPT = ROOT / "scripts/Test-UnrealCandidateWorkflowInlineSyntax.ps1"
INLINE_FIXTURES = ROOT / "scripts/Test-UnrealCandidateWorkflowInlineSyntaxPolicy.ps1"
CORE_CI = ROOT / ".github/workflows/core-tests.yml"


def validate_manual_workstation_workflow(source: str) -> None:
    required = (
        "on:\n  workflow_dispatch:",
        "github.repository == 'sanskarIN/latebrake'",
        "github.ref == 'refs/heads/main'",
        "github.event_name == 'workflow_dispatch'",
        "runs-on: [self-hosted, Windows, X64, unreal-5-8]",
        "ref: ${{ github.sha }}",
        "persist-credentials: false",
        "lfs: true",
        "contents: read",
        "LATEBRAKE_UNREAL_ROOT",
        "Engine/Build/Build.version",
        "$version.MajorVersion -ne 5",
        "$version.MinorVersion -ne 8",
        "EXPECTED_SOURCE_SHA: ${{ github.sha }}",
        "$actualSha -cne $env:EXPECTED_SOURCE_SHA",
        "GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT",
        "candidate_output=$candidate",
        "Build-PlayableCandidate.ps1",
        "CANDIDATE_PLATFORM: ${{ inputs.platform }}",
        "CANDIDATE_OUTPUT: ${{ steps.preflight.outputs.candidate_output }}",
        "-Platform $env:CANDIDATE_PLATFORM",
        "-OutputDirectory $env:CANDIDATE_OUTPUT",
        "Test-Path -LiteralPath $candidate",
        "actions/upload-artifact@v4",
        "UNVERIFIED",
        "retention-days: 3",
        "compression-level: 0",
        "Remove-Item -LiteralPath $target -Recurse -Force",
        "if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)",
        # A failure before Write-CandidateManifest still leaves mutable
        # staging contents; never delete through nested junctions.
        "[System.Collections.Generic.Stack[string]]::new()",
        "Get-ChildItem -LiteralPath $directory -Force -ErrorAction Stop",
        "if (($child.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)",
        "$directories.Push($child.FullName)",
        "if: always()",
        "cancel-in-progress: false",
    )
    for marker in required:
        if marker not in source:
            raise ValueError(f"Missing manual engine work gate: {marker}")
    for expression in (r"^  pull_request:", r"^  push:"):
        if re.search(expression, source, re.MULTILINE):
            raise ValueError("Licensed self-hosted runner cannot be triggered by PR or push")
    for forbidden in (
        "continue-on-error: true", "github.rest.repos.createRelease",
        "gh release create", "make_latest:", "release-action",
        "-OutputDirectory '${{ steps.preflight.outputs.candidate_output }}'",
        "-Platform '${{ inputs.platform }}'",
    ):
        if forbidden in source:
            raise ValueError("Unverified UE candidate cannot publish a public release")
    if source.count("runs-on: [self-hosted, Windows, X64, unreal-5-8]") != 1:
        raise ValueError("Ambiguous authorized runner")
    if source.count("ref: ${{ github.sha }}") != 1:
        raise ValueError("Candidate must use exact dispatch SHA")


class UnrealWorkstationWorkflowPolicyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = WF.read_text(encoding="utf-8")
        cls.doc = DOC.read_text(encoding="utf-8")
        cls.script = BUILD_SCRIPT.read_text(encoding="utf-8")

    def test_manual_exact_sha_unverified_candidate(self):
        validate_manual_workstation_workflow(self.source)
        for marker in (
            "Editor/UHT", "actual", "map", "UAT", "SHA",
            "installed", "unverified", "#148",
        ):
            self.assertIn(marker.lower(), self.doc.lower())
        for marker in (
            "Test-SourceProvenance.ps1", "Build-Editor.ps1",
            "New-StarterMap.ps1", "Build-PreRelease.ps1",
            "Write-CandidateManifest.ps1", "Test-CandidateManifest.ps1",
        ):
            self.assertIn(marker, self.script)

    def test_inline_power_shell_blocks_checked_without_ue(self):
        validator = INLINE_SCRIPT.read_text(encoding="utf-8")
        fixture = INLINE_FIXTURES.read_text(encoding="utf-8")
        ci = CORE_CI.read_text(encoding="utf-8")
        for token in (
            "ParseInput(", "contentIndent", "Expected at least 3",
            "Malformed workflow PowerShell block",
        ):
            self.assertIn(token, validator)
        self.assertIn("did not fail closed", fixture)
        self.assertIn("Test-UnrealCandidateWorkflowInlineSyntaxPolicy.ps1", ci)
        self.assertIn("shell: pwsh", ci)

    def test_mutated_security_and_source_gates_fail_closed(self):
        source = self.source
        mutants = (
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
            source.replace("persist-credentials: false",
                           "persist-credentials: true", 1),
            source.replace("github.ref == 'refs/heads/main'",
                           "true", 1),
            source.replace("runs-on: [self-hosted, Windows, X64, unreal-5-8]",
                           "runs-on: windows-latest", 1),
            source.replace("EXPECTED_SOURCE_SHA: ${{ github.sha }}",
                           "EXPECTED_SOURCE_SHA: latest", 1),
            source.replace("$version.MinorVersion -ne 8",
                           "$version.MinorVersion -ne 0", 1),
            source.replace("UNVERIFIED", "READY_TO_PUBLISH"),
            source.replace("Build-PlayableCandidate.ps1",
                           "Build-PreRelease.ps1", 1),
            source.replace("-OutputDirectory $env:CANDIDATE_OUTPUT",
                           "-OutputDirectory './stale-output'", 1),
            source.replace("CANDIDATE_PLATFORM: ${{ inputs.platform }}",
                           "CANDIDATE_PLATFORM: Win64", 1),
            source.replace("retention-days: 3",
                           "retention-days: 90", 1),
            source.replace("[IO.FileAttributes]::ReparsePoint",
                           "[IO.FileAttributes]::Normal", 1),
            source.replace(
                "Get-ChildItem -LiteralPath $directory -Force -ErrorAction Stop",
                "Get-ChildItem -LiteralPath $directory -Force -ErrorAction SilentlyContinue",
                1,
            ),
            source.replace(
                "if (($child.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)",
                "if ($false)", 1,
            ),
            source.replace("$directories.Push($child.FullName)", "# skip nested directories", 1),
            source + "\n  pull_request:\n",
        )
        cleanup_root_check = source.index(
            "if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint)"
        )
        child_check = source.index(
            "if (($child.Attributes -band [IO.FileAttributes]::ReparsePoint)"
        )
        cleanup_delete = source.index("Remove-Item -LiteralPath $target -Recurse -Force")
        self.assertLess(cleanup_root_check, child_check)
        self.assertLess(child_check, cleanup_delete)

        for broken in mutants:
            with self.subTest(fragment=broken[-90:]):
                with self.assertRaises(ValueError):
                    validate_manual_workstation_workflow(broken)


if __name__ == "__main__":
    unittest.main()
