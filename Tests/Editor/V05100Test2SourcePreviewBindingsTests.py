"""Source-only test.2 release gate and exact-tag publication boundary.

These are static checks, not a live GitHub publication, UE Editor build,
installed Windows game or substitute for source-CI verification.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW_PATH = ROOT / ".github/workflows/audit-v05100-test2-source-readiness.yml"
PREVIEW_PATH = ROOT / "scripts/SourcePreviewV05100Test2Readiness.cjs"
AUDIT_PATH = ROOT / "scripts/AuditPublishedSourceRelease.cjs"
NOTES_PATH = ROOT / "docs/releases/v0.5.100-test.2.md"


def verify_safe_preview(workflow: str, guard: str, audit: str, notes: str) -> None:
    required_workflow = (
        "workflow_dispatch:", "candidate_sha:", "permissions:",
        "  contents: read", "  checks: read", "  pull-requests: read",
        "github.actor == 'sanskarIN'",
        "github.ref == 'refs/heads/main'",
        "persist-credentials: false",
        "ref: ${{ github.sha }}",
        "guard.inspectReadiness({",
        "V05100Test2ReadinessTests.cjs",
    )
    required_guard = (
        "const REQUIRED_SOURCE_PR = 395;",
        "const UNMERGED_GAMEPLAY_PRS = Object.freeze([377, 391, 394]);",
        "context.eventName === 'workflow_dispatch'",
        "context.payload?.inputs?.candidate_sha === candidateSha",
        "branch.protected === true",
        "blob.encoding === 'base64'",
        "readUtf8File(note) === notes",
        "sourceGate.requirePassingChecks(checkRuns, candidateSha)",
        "releaseAudit.verifyPublishedSourceRelease({",
        "new descendant of immutable v0.5.100-test.1",
        "nextTag === null) === (nextRelease === null",
        "latest.commit?.sha === candidateSha",
        "nativeUnrealQualified: false",
        "playableWin64Qualified: false",
        "writesPerformed: 0",
    )
    required_audit = (
        "const REVIEWED_TEST_TAG = 'v0.5.100-test.2';",
        "return {kind: 'reviewed-source-test', notesPath: REVIEWED_TEST_NOTES_PATH};",
        "classification.kind === 'reviewed-source-test'",
        "release.target_commitish === ref.sha",
        "release.body === sourceNotes",
        "release.assets.length === 0",
        "ref.sha === LEGACY_TEST_SHA",
    )
    required_notes = (
        "SOURCE-ONLY", "NOT a playable game build",
        "No compiled Win64 EXE", "Android APK/AAB",
        "not installers", "zero executable binaries",
        "zero attached game assets", "PR #395", "PR #377",
        "PR #391", "PR #394", "87,792", "+16,225",
        "not integrated into real Unreal gameplay",
    )
    for name, value, markers in (
        ("workflow", workflow, required_workflow),
        ("readiness", guard, required_guard),
        ("published-audit", audit, required_audit),
        ("tagged-notes", notes, required_notes),
    ):
        for marker in markers:
            if marker not in value:
                raise ValueError(f"{name} missing source safety gate: {marker}")

    for forbidden in (
        "contents: write", "createRelease({", "createRef({",
        "updateRef({", "uploadReleaseAsset({",
        "workflow_dispatch({", "deleteRelease({",
    ):
        if forbidden in workflow or forbidden in guard:
            raise ValueError(f"Source preview readiness must stay read-only: {forbidden}")
    if "publish" in WORKFLOW_PATH.name:
        raise ValueError("Readiness must remain separate from the publisher")


class V05100Test2SourcePreviewBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workflow = WORKFLOW_PATH.read_text(encoding="utf-8")
        cls.guard = PREVIEW_PATH.read_text(encoding="utf-8")
        cls.audit = AUDIT_PATH.read_text(encoding="utf-8")
        cls.notes = NOTES_PATH.read_text(encoding="utf-8")

    def test_exact_tag_and_source_only_warning_contract(self):
        verify_safe_preview(
            self.workflow, self.guard, self.audit, self.notes
        )

    def test_missing_write_protection_is_detected(self):
        with self.assertRaises(ValueError):
            verify_safe_preview(
                self.workflow.replace("contents: read", "contents: write"),
                self.guard, self.audit, self.notes,
            )

    def test_unknown_tag_would_not_be_eligible(self):
        with self.assertRaises(ValueError):
            verify_safe_preview(
                self.workflow, self.guard,
                self.audit.replace("v0.5.100-test.2", "v0.5.100-test.3"),
                self.notes,
            )

    def test_no_relabeling_unmerged_gameplay_as_shipped(self):
        with self.assertRaises(ValueError):
            verify_safe_preview(
                self.workflow, self.guard, self.audit,
                self.notes.replace("PR #394", "Merged PR #394"),
            )


if __name__ == "__main__":
    unittest.main()
