"""Read-only contracts for diagnosing queued owner-operated UE 5.8 jobs.

These source checks never access the current runner registry, call a
licensed Unreal toolchain, start a workflow, or report a playable game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealRunnerQueueBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.queue = read("docs/UNREAL_RUNNER_QUEUE_QA.md")
        cls.manual = read("docs/UNREAL_DRAFT_PR_VALIDATION.md")
        cls.workflow = read(".github/workflows/validate-draft-unreal-pr.yml")
        cls.nav = read("mkdocs.yml")
        cls.preflight = read("scripts/Test-UnrealWorkstationPrerequisites.ps1")

    def test_manual_job_does_not_run_on_public_hosted_actions(self):
        self.assertIn(
            "runs-on: [self-hosted, Windows, X64, unreal-5-8]",
            self.workflow,
        )
        for label in ("self-hosted", "Windows", "X64", "unreal-5-8"):
            self.assertIn(label, self.queue)
        self.assertIn("windows-latest", self.queue)
        self.assertIn("Do **not** replace", self.queue)
        self.assertNotIn("\\`", self.queue)
        self.assertIn("Online", self.queue)
        self.assertIn("Idle", self.queue)

    def test_credentials_and_manual_owner_review_remain_private(self):
        for marker in (
            "registration token", "service account", "GitHub",
            "Settings", "Actions", "Runners", "Labels",
            "LATEBRAKE_UNREAL_ROOT", "PowerShell **7**",
            "issue", "validation=Editor", "draft PR #341",
        ):
            self.assertIn(marker.lower(), self.queue.lower())
        self.assertIn("github.actor == 'sanskarIN'", self.workflow)
        self.assertIn("github.ref == 'refs/heads/main'", self.workflow)
        self.assertIn("pull-requests: read", self.workflow)
        self.assertIn("persist-credentials: false", self.workflow)
        self.assertIn("Run workflow", self.manual)

    def test_runner_queue_is_not_misrepresented_as_engine_evidence(self):
        for marker in (
            "queued", "in_progress", "completed",
            "source-only", "not", "UHT", "cooked", "installed",
            "UNVERIFIED", "40-character", "#148",
        ):
            self.assertIn(marker.lower(), self.queue.lower())
        self.assertIn("NOT VERIFIED", self.preflight)
        self.assertIn("Build.version", self.preflight)
        self.assertIn("UNREAL_RUNNER_QUEUE_QA.md", self.nav)
        self.assertIn("UNREAL_RUNNER_QUEUE_QA.md", self.manual)

    def test_diagnostics_cannot_create_builds_or_workflows(self):
        for forbidden in (
            "gh workflow run", "gh release create", "workflow_dispatch:\n",
            "actions/upload-artifact", "Set-Content",
        ):
            self.assertNotIn(forbidden, self.queue)
        self.assertIn("workflow_dispatch:", self.workflow)


if __name__ == "__main__":
    unittest.main()
