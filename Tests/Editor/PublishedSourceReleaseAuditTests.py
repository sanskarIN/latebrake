"""Read-only published-release audit policy; not real game QA.

These are intentionally source inspection tests only. The GitHub API
workflow independently executes against actual publication metadata.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
AUDIT = (ROOT / ".github/workflows/audit-published-source-release.yml").read_text(
    encoding="utf-8"
)


def validate_audit_policy(source: str) -> None:
    for required in (
        "release:\n    types: [published]",
        "workflow_dispatch:",
        "permissions:\n  contents: read",
        "jobs:\n  audit-source-release:",
        "uses: actions/github-script@v7",
        "context.payload.release?.tag_name",
        "process.env.INPUT_TAG",
        "getReleaseByTag({",
        "release.draft",
        "!release.prerelease",
        "!release.published_at",
        "release.assets.length !== 0",
        "github.rest.git.getRef({",
        "ref.type !== 'commit'",
        "release.target_commitish !== ref.sha",
        "github.rest.repos.getContent({",
        "ref: ref.sha",
        "'docs/releases/' + tag + '.md'",
        "noteFile.encoding !== 'base64'",
        "release.body !== notes",
        "context.payload.release.id !== release.id",
        "SOURCE-ONLY",
        "NOT a playable game build",
        "zero executable binaries",
        "No compiled Win64 EXE",
        "Android APK/AAB",
    ):
        if required not in source:
            raise ValueError(f"Source audit policy missing: {required}")
    for forbidden in (
        "contents: write",
        "createRelease({",
        "updateRelease({",
        "deleteRelease({",
        "createRef({",
        "updateRef({",
        "deleteRef({",
        "uploadReleaseAsset({",
    ):
        if forbidden in source:
            raise ValueError(f"Release audit must be read-only: {forbidden}")


class PublishedSourceReleaseAuditTests(unittest.TestCase):
    def test_metadata_only_audit_has_all_read_only_proofs(self):
        validate_audit_policy(AUDIT)

    def test_missing_immutable_identity_guard_fails(self):
        for marker in (
            "release.target_commitish !== ref.sha",
            "release.body !== notes",
            "release.assets.length !== 0",
            "ref: ref.sha",
            "release.draft",
            "!release.prerelease",
            "context.payload.release.id !== release.id",
        ):
            with self.subTest(marker=marker):
                mutated = AUDIT.replace(marker, "/* safety check disabled */", 1)
                self.assertNotEqual(mutated, AUDIT)
                with self.assertRaises(ValueError):
                    validate_audit_policy(mutated)

    def test_write_capability_cannot_be_added(self):
        with self.assertRaises(ValueError):
            validate_audit_policy(AUDIT.replace("contents: read", "contents: write"))
        with self.assertRaises(ValueError):
            validate_audit_policy(AUDIT + "\n# createRelease({\n")


if __name__ == "__main__":
    unittest.main()
