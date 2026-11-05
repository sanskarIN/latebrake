"""One-time source-only v0.5.91 draft-PR publisher safety contract.

This checks policy wiring, not a GitHub publication or UE5.8 runtime build.
It runs on the publishing PR before the workflow may reach protected main.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = ROOT / ".github/workflows/publish-suspension-v0591-experimental.yml"
NOTES = ROOT / "docs/releases/v0.5.91.md"
SOURCE_SHA = "8edba49c0a6294ece2797c708d80b6dd52288d40"


class ExperimentalV0591SourceReleasePolicyTests(unittest.TestCase):
    def test_exact_candidate_and_all_four_independent_gates(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        for required in (
            "branches: [main]", "workflow_dispatch:",
            "group: latebrake-v0591-experimental-source-only",
            "RELEASE_COMMIT: " + SOURCE_SHA,
            "pull_number:295", "pr.state !== 'open'", "!pr.draft",
            "pr.head.sha !== candidate", "pr.head.sha !== sha",
            "pr.head.user?.login !== 'sanskarIN'",
            "pr.head.repo?.full_name !== 'sanskarIN/latebrake'",
            "pr.head.ref !== 'feat/chaos-four-corner-suspension'",
            "main !== context.sha",
            "needs: [preflight, source-tests, editor-tests, packaging-policy, documentation]",
            "os: [windows-latest, ubuntu-latest, macos-latest]",
            "cmake --build Tests/Standalone/build --config Release",
            "ctest --test-dir Tests/Standalone/build -C Release --output-on-failure",
            "ChassisWheelGeometry SuspensionQaCadence;",
            "python3 -m unittest discover -s Tests/Editor",
            "Test-PackagingPolicy.ps1", "Test-SourceProvenancePolicy.ps1",
            "python scripts/verify_docs_site.py site",
            "published.tag_name !== tag",
            "published.target_commitish !== sha",
            "published.assets.length !== 0",
        ):
            with self.subTest(marker=required):
                self.assertIn(required, text)
        self.assertEqual(text.count("          ref: " + SOURCE_SHA), 4)
        self.assertEqual(text.count("      contents: write"), 1)
        self.assertIn("preflight:", text)
        for forbidden in (
            "updateRef({", "deleteRef({", "deleteRelease({",
            "updateRelease({", "uploadReleaseAsset({",
            "upload-artifact@", "make_latest:'true'",
            "push --force"
        ):
            self.assertNotIn(forbidden, text)

    def test_notes_identity_and_source_only_claims(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        notes = NOTES.read_text(encoding="utf-8")
        for keyword in (
            "SOURCE-ONLY", "NOT a playable game build",
            "No compiled Win64 EXE", "Android APK/AAB",
            "zero executable binaries", "not installers",
            "unmerged development", "v0.5.90", "four-corner",
        ):
            self.assertIn(keyword, notes)
        for keyword in (
            "sourceFile.encoding !== 'base64'",
            "Buffer.from(sourceFile.content,'base64').toString('utf8') !== notes",
            "ref.type !== 'commit' || ref.sha !== sha",
            "github.rest.git.createRef({",
            "github.rest.repos.createRelease({",
            "prerelease:true", "make_latest:'false'",
            "previous.assets.length !== 0",
            "never rewrite",
        ):
            self.assertIn(keyword.lower() if keyword == "never rewrite" else keyword,
                          workflow.lower() if keyword == "never rewrite" else workflow)


if __name__ == "__main__":
    unittest.main()
