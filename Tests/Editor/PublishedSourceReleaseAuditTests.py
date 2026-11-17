"""Static integration tests for the read-only published-source audit.

The GitHub workflow delegates identity policy to a separately tested
CommonJS module. Both files must keep the exact immutable source gates;
these assertions do not substitute for GitHub API or native Unreal QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
AUDIT = (ROOT / ".github/workflows/audit-published-source-release.yml").read_text(
    encoding="utf-8"
)
POLICY = (ROOT / "scripts/AuditPublishedSourceRelease.cjs").read_text(
    encoding="utf-8"
)


def validate_audit_policy(workflow: str, policy: str = POLICY) -> None:
    workflow_required = (
        "release:\n    types: [published]",
        "workflow_dispatch:",
        "permissions:\n  contents: read",
        "jobs:\n  audit-source-release:",
        "uses: actions/checkout@v4",
        "persist-credentials: false",
        "ref: main",
        "uses: actions/github-script@v7",
        "require('./scripts/AuditPublishedSourceRelease.cjs')",
        "context.payload.release?.tag_name",
        "process.env.INPUT_TAG",
        "getReleaseByTag({",
        "github.rest.git.getRef({",
        "github.rest.repos.getContent({",
        "ref: reference.object.sha",
        "noteFile.encoding !== 'base64'",
        "policy.verifyPublishedSourceRelease({",
        "context.payload.release.id !== release.id",
    )
    policy_required = (
        "const OFFICIAL_SOURCE_TAG = /^v0\\.5\\.\\d+$/;",
        "const LEGACY_TEST_TAG = 'v0.5.100-test.1';",
        "const LEGACY_TEST_SHA = 'a04db1ccc15ee2e3be6771d7e4a5159dcadb2384';",
        "ref?.type === 'commit'",
        "release.target_commitish === ref.sha",
        "release.body === sourceNotes",
        "release.draft === false",
        "release.prerelease === true",
        "release.assets.length === 0",
        "ref.sha === LEGACY_TEST_SHA",
        "release.target_commitish === 'main'",
        "release.name === LEGACY_TEST_NAME",
        "release.body.replace(/\\r\\n?/g, '\\n').trimEnd() === LEGACY_TEST_BODY",
        "NOT a playable game build",
        "No compiled Win64 EXE",
        "Android APK/AAB",
        "zero executable binaries",
    )
    for label, source, required in (
        ("workflow", workflow, workflow_required),
        ("policy", policy, policy_required),
    ):
        for marker in required:
            if marker not in source:
                raise ValueError(f"Source audit {label} missing: {marker}")
    for forbidden in (
        "contents: write",
        "createRelease({",
        "updateRelease({",
        "deleteRelease({",
        "createRef({",
        "updateRef({",
        "deleteRef({",
        "uploadReleaseAsset({",
        "workflow_dispatch({",
    ):
        if forbidden in workflow or forbidden in policy:
            raise ValueError(f"Release audit must be read-only: {forbidden}")


class PublishedSourceReleaseAuditTests(unittest.TestCase):
    def test_metadata_only_audit_has_all_read_only_proofs(self):
        validate_audit_policy(AUDIT)

    def test_missing_workflow_identity_guard_fails(self):
        for marker in (
            "ref: reference.object.sha",
            "noteFile.encoding !== 'base64'",
            "policy.verifyPublishedSourceRelease({",
            "context.payload.release.id !== release.id",
            "persist-credentials: false",
        ):
            with self.subTest(marker=marker):
                mutated = AUDIT.replace(marker, "/* safety check disabled */", 1)
                self.assertNotEqual(mutated, AUDIT)
                with self.assertRaises(ValueError):
                    validate_audit_policy(mutated)

    def test_missing_official_or_test_release_identity_guard_fails(self):
        for marker in (
            "release.target_commitish === ref.sha",
            "release.body === sourceNotes",
            "release.assets.length === 0",
            "release.draft === false",
            "release.prerelease === true",
            "ref.sha === LEGACY_TEST_SHA",
            "release.target_commitish === 'main'",
            "release.body.replace(/\\r\\n?/g, '\\n').trimEnd() === LEGACY_TEST_BODY",
        ):
            with self.subTest(marker=marker):
                mutated = POLICY.replace(marker, "/* safety check disabled */", 1)
                self.assertNotEqual(mutated, POLICY)
                with self.assertRaises(ValueError):
                    validate_audit_policy(AUDIT, mutated)

    def test_write_capability_cannot_be_added(self):
        with self.assertRaises(ValueError):
            validate_audit_policy(AUDIT.replace("contents: read", "contents: write"))
        with self.assertRaises(ValueError):
            validate_audit_policy(AUDIT + "\n# createRelease({\n")
        with self.assertRaises(ValueError):
            validate_audit_policy(AUDIT, POLICY + "\n// updateRef({\n")


if __name__ == "__main__":
    unittest.main()
