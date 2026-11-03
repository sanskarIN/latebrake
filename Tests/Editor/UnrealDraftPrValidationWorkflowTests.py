"""Source-only verification of the HUMAN-DISPATCHED licensed UE5.8 PR gate.

These tests inspect YAML/PowerShell wiring without launching UE, visiting
GitHub APIs, using credentials or creating a playable package. An actual
authorized Windows self-hosted runner is still required to prove compilation.
"""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class UnrealDraftPrValidationWorkflowTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.yml = (ROOT / ".github/workflows/validate-draft-unreal-pr.yml").read_text(
            encoding="utf-8"
        )
        cls.docs = (ROOT / "docs/UNREAL_DRAFT_PR_VALIDATION.md").read_text(
            encoding="utf-8"
        )

    def test_only_manual_main_workflow_dispatch_may_reach_licensed_runner(self):
        source = self.yml
        self.assertIn("on:\n  workflow_dispatch:", source)
        before_jobs = source.split("jobs:", 1)[0]
        self.assertNotIn("\n  pull_request:", before_jobs)
        self.assertNotIn("\n  push:", before_jobs)
        self.assertNotIn("\n  schedule:", before_jobs)
        self.assertIn("github.repository == 'sanskarIN/latebrake'", source)
        self.assertIn("github.actor == 'sanskarIN'", source)
        self.assertIn("github.ref == 'refs/heads/main'", source)
        self.assertIn("github.event_name == 'workflow_dispatch'", source)
        self.assertIn("runs-on: [self-hosted, Windows, X64, unreal-5-8]", source)
        self.assertIn("timeout-minutes: 180", source)
        self.assertIn("cancel-in-progress: false", source)

    def test_untrusted_forks_and_stale_head_sha_are_rejected_before_checkout(self):
        source = self.yml
        for required in (
            "type: number", "expected_head_sha:", "type: string",
            "^[1-9][0-9]{0,5}$", "^[0-9a-fA-F]{40}$",
            "Invoke-RestMethod", "-Method Get", "-Headers $headers",
            "$pr.state -cne 'open'", "$pr.base.ref -cne 'main'",
            "$pr.head.repo.full_name -cne $env:REPO",
            "$pr.head.user.login -cne 'sanskarIN'",
            "$pr.head.sha -cne $env:EXPECTED_SHA.ToLowerInvariant()",
            "verified_sha=$($pr.head.sha)",
            "ref: ${{ steps.authorize.outputs.verified_sha }}",
            "The reviewed SHA is stale", "PR changed or became ineligible",
            "Checked out source does not match approved PR commit",
        ):
            with self.subTest(required=required):
                self.assertIn(required, source)
        self.assertLess(
            source.index("Authenticate open owner PR and exact reviewed SHA"),
            source.index("Checkout exactly the authenticated SAME-repository commit"),
        )
        self.assertLess(
            source.index("Checkout exactly the authenticated SAME-repository commit"),
            source.index("Confirm source stayed current and licensed Unreal 5.8"),
        )
        self.assertLess(
            source.index("Reauthenticate PR SHA after checkout without running PR code"),
            source.index("Verify genuine UE host and prepare a fresh candidate destination"),
        )
        self.assertLess(
            source.index("Verify genuine UE host and prepare a fresh candidate destination"),
            source.index("Compile genuine UE 5.8 Editor and UHT"),
        )
        tokened = source.split("Reauthenticate PR SHA after checkout without running PR code", 1)[1].split(
            "- name: Verify genuine UE host", 1
        )[0]
        source_scripts = source.split("- name: Verify genuine UE host", 1)[1].split(
            "- name: Upload UNVERIFIED", 1
        )[0]
        self.assertIn("GH_TOKEN:", tokened)
        self.assertNotIn("./scripts/", tokened)
        self.assertNotIn("GH_TOKEN:", source_scripts)
        self.assertNotIn("API_URL:", source_scripts)
        self.assertNotIn("github.token", source_scripts)
        self.assertGreaterEqual(source.count("persist-credentials: false"), 2)

    def test_genuine_builds_never_fake_unreal_or_publish_release(self):
        source = self.yml
        for required in (
            "permissions:\n  contents: read\n  pull-requests: read",
            "Test-UnrealWorkstationPrerequisites.ps1",
            "Test-SourceProvenance.ps1",
            "Compile-UnrealEditor.ps1",
            "Build-PlayableCandidate.ps1",
            "-Platform Win64",
            "Unreal 5.8", "LATEBRAKE_UNREAL_ROOT",
            "actions/upload-artifact@v4",
            "retention-days: 3",
            "UNVERIFIED-UE58-Win64-PR",
            "if: inputs.validation == 'Editor'",
            "if: inputs.validation == 'Win64'",
            "if: inputs.validation == 'Win64' && success()",
            "if: always() && inputs.validation == 'Win64'",
        ):
            with self.subTest(required=required):
                self.assertIn(required, source)
        for forbidden in (
            "softprops/action-gh-release", "gh release create",
            "git push ", "pull_request_target:", "submodules: recursive",
        ):
            self.assertNotIn(forbidden, source)
        self.assertIn("Only the GitHub owner may dispatch", self.docs)
        self.assertIn("not a release", self.docs.lower())
        self.assertIn("PR #295", self.docs)
        core_ci = (ROOT / ".github/workflows/core-tests.yml").read_text(
            encoding="utf-8"
        )
        self.assertIn(
            "Test-UnrealCandidateWorkflowInlineSyntax.ps1 -WorkflowPath .github/workflows/validate-draft-unreal-pr.yml",
            core_ci,
        )
        self.assertIn(
            "UNREAL_DRAFT_PR_VALIDATION.md",
            (ROOT / "mkdocs.yml").read_text(encoding="utf-8"),
        )

    def test_cleanup_cannot_escape_runner_temporary_directory(self):
        source = self.yml
        for required in (
            "Refusing to reuse stale candidate output",
            "$env:RUNNER_TEMP", "Path]::GetFullPath",
            "OrdinalIgnoreCase", "[IO.FileAttributes]::ReparsePoint",
            "Refusing to delete a candidate containing a junction/symlink",
            "Refusing to delete a candidate root that is a junction/symlink",
            "Get-ChildItem -LiteralPath $directory -Force",
            "Remove-Item -LiteralPath $target -Recurse -Force",
        ):
            self.assertIn(required, source)
        self.assertLess(
            source.index("if (-not $target.StartsWith($root"),
            source.index("Remove-Item -LiteralPath $target -Recurse -Force"),
        )


if __name__ == "__main__":
    unittest.main()
