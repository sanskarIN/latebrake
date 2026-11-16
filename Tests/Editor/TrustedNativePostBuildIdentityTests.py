"""Static, offline checks of native PR validation identity provenance.

These tests cannot compile Unreal or certify a game. They ensure that a
long-running self-hosted build is followed by an independent, source-free
GitHub-hosted check of the CURRENT PR head, so an old artifact cannot be
mistaken for a valid current-PR result.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = (ROOT / ".github/workflows/validate-draft-unreal-pr.yml").read_text(
    encoding="utf-8"
)
GUIDE = (ROOT / "docs/UNREAL_DRAFT_PR_VALIDATION.md").read_text(
    encoding="utf-8"
)


class TrustedNativePostBuildIdentityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.native, cls.hosted = WORKFLOW.split(
            "\n  postbuild-source-identity:", 1
        )

    def test_explicit_owner_main_manual_only(self):
        self.assertIn("\non:\n  workflow_dispatch:", WORKFLOW)
        self.assertNotIn("\n  pull_request:", WORKFLOW)
        self.assertNotIn("\n  pull_request_target:", WORKFLOW)
        self.assertNotIn("\n  push:", WORKFLOW)
        self.assertIn("github.actor == 'sanskarIN'", self.native)
        self.assertIn("github.ref == 'refs/heads/main'", self.native)

    def test_owner_only_native_worker_outputs_the_authenticated_sha(self):
        self.assertIn("runs-on: [self-hosted, Windows, X64, unreal-5-8]",
                      self.native)
        self.assertIn(
            "verified_sha: ${{ steps.authorize.outputs.verified_sha }}",
            self.native)
        self.assertIn("PR head authenticated", self.native)
        self.assertIn("Reauthenticate PR SHA after checkout", self.native)

    def test_hosted_post_build_check_cannot_run_pr_code(self):
        self.assertIn("needs: [trusted-draft-candidate]", self.hosted)
        self.assertIn("runs-on: ubuntu-latest", self.hosted)
        self.assertIn("contents: read", self.hosted)
        self.assertIn("pull-requests: read", self.hosted)
        self.assertNotIn("contents: write", self.hosted)
        self.assertNotIn("runs-on: [self-hosted", self.hosted)
        self.assertNotIn("actions/checkout@", self.hosted)
        self.assertNotIn("download-artifact@", self.hosted)
        self.assertNotIn("LATEBRAKE_UNREAL_ROOT", self.hosted)
        self.assertNotIn("Build-PlayableCandidate", self.hosted)

    def test_post_build_live_identity_fails_closed(self):
        for required in (
            "github.rest.pulls.get", "pr.state !== 'open'",
            "pr.base?.ref !== 'main'", "pr.head?.repo?.full_name",
            "pr.head?.user?.login !== 'sanskarIN'",
            "pr.head?.sha !== built",
            "process.env.NATIVE_VERIFIED_SHA",
            "expected !== built",
            "context.eventName !== 'workflow_dispatch'",
            "context.ref !== 'refs/heads/main'",
            "stale and cannot certify the current PR",
        ):
            with self.subTest(required=required):
                self.assertIn(required, self.hosted)

    def test_artifact_stays_unverified_and_full_sha_pinned(self):
        for required in (
            "UNVERIFIED-UE58-Win64-PR",
            "${{ steps.authorize.outputs.verified_sha }}",
            "${{ github.run_id }}-${{ github.run_attempt }}",
            "retention-days: 3",
            "if-no-files-found: error",
        ):
            with self.subTest(required=required):
                self.assertIn(required, self.native)
        self.assertIn("full exact built 40-character SHA", GUIDE)
        self.assertIn("separate GitHub-hosted post-build", GUIDE)
        self.assertIn("not the self-hosted machine", GUIDE)
        self.assertIn("No installed gameplay", WORKFLOW)


if __name__ == "__main__":
    unittest.main()
