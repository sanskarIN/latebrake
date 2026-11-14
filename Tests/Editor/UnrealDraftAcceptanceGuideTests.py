"""Fail-closed source/document checks for manual UE5.8 drafting QA.

These tests never execute Unreal, dispatch workflow jobs or certify a
playable candidate. They protect only the *instructions* to the owner.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealDraftAcceptanceGuideTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workflow = read(".github/workflows/validate-draft-unreal-pr.yml")
        cls.guide = read("docs/UNREAL_DRAFT_PR_VALIDATION.md")
        cls.runner = read("docs/UNREAL_RUNNER_QUEUE_QA.md")
        cls.readiness = read("docs/UNREAL_NATIVE_MERGE_READINESS.md")
        cls.acceptance = read("docs/UNREAL_V05100_DRAFTING_ACCEPTANCE.md")
        cls.mkdocs = read("mkdocs.yml")

    def test_owner_only_exact_pr_identity_still_required(self):
        for expected in (
            "workflow_dispatch:",
            "expected_head_sha:",
            "github.actor == 'sanskarIN'",
            "github.ref == 'refs/heads/main'",
            "runs-on: [self-hosted, Windows, X64, unreal-5-8]",
            "github.token",
            "persist-credentials: false",
        ):
            self.assertIn(expected, self.workflow)

    def test_v05100_operator_has_no_static_sha_or_false_native_claim(self):
        for guide in (self.guide, self.acceptance):
            for required in (
                "PR #377", "Editor", "Win64", "main",
                "UNVERIFIED",
            ):
                self.assertIn(required, guide)
        self.assertIn("expected_head_sha", self.guide)
        self.assertIn("current", self.guide.lower())
        self.assertIn("source-only", self.acceptance.lower())
        self.assertIn("Not tested / to be recorded", self.acceptance)
        self.assertIn("explicit approval", self.acceptance)
        self.assertNotIn("PR #377 has passed real hardware QA", self.guide)

    def test_real_obstacle_and_vertical_QA_are_required(self):
        for phrase in (
            "solid Visibility-blocking barrier",
            "0.18 s",
            "road crest and descent",
            "actor identity",
            "30, 60 and",
            "120 FPS",
            "40%",
            "source HEAD",
            "UE Build.version",
        ):
            self.assertIn(phrase, self.acceptance)

    def test_no_deleted_native_issue_used_as_a_release_gate(self):
        self.assertNotIn("playable milestone #148", self.acceptance)
        self.assertIn("PR #377", self.readiness)
        self.assertIn("PR #377", self.runner)
        self.assertIn(
            "UNREAL_V05100_DRAFTING_ACCEPTANCE.md", self.mkdocs
        )


if __name__ == "__main__":
    unittest.main()
