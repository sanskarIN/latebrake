"""Guard the preparatory v0.5.98 audit from becoming a publisher.

These are static source-only expectations, NOT a live GitHub release
eligibility proof, licensed Unreal build or Windows installer test.
"""
from pathlib import Path
import json
import unittest

ROOT = Path(__file__).resolve().parents[2]


class V0598ReleaseReadinessBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workflow = (ROOT / ".github/workflows/audit-ready-v0598-source.yml"
                        ).read_text(encoding="utf-8")
        cls.publisher = (ROOT / ".github/workflows/prepare-v0598-source.yml"
                         ).read_text(encoding="utf-8")
        cls.plan = json.loads(
            (ROOT / "scripts/releases/v0.5.98.json").read_text(encoding="utf-8")
        )
        cls.notes = (ROOT / "docs/releases/v0.5.98.md").read_text(encoding="utf-8")
        cls.docs = (ROOT / "docs/V0598_SOURCE_READINESS_QA.md").read_text(
            encoding="utf-8"
        )
        cls.nav = (ROOT / "mkdocs.yml").read_text(encoding="utf-8")

    def test_manual_read_only_not_source_release_workflow(self):
        w = self.workflow
        self.assertIn("workflow_dispatch:", w)
        self.assertIn("github.actor == 'sanskarIN'", w)
        self.assertIn("github.ref == 'refs/heads/main'", w)
        self.assertIn("permissions:\n  contents: read", w)
        self.assertIn("      pull-requests: read", w)
        self.assertIn("persist-credentials: false", w)
        self.assertIn("ref: ${{ github.sha }}", w)
        self.assertIn("guard.inspectState({", w)
        self.assertEqual(w.count("guard.inspectState("), 1)
        self.assertIn("node --test Tests/Release/ExperimentalSourceReleaseTests.cjs", w)
        self.assertIn("Source pin qualified; no v0.5.98 tag or release exists", w)
        self.assertIn("NOT checked", w)
        for disallowed in (
            "contents: write", "pull_request_target:", "pull_request:",
            "push:", "schedule:", "workflow_run:", "createRelease(",
            "createRef(", "guard.publish(", "upload-artifact@",
            "Compile-UnrealEditor.ps1", "Build-PlayableCandidate.ps1",
        ):
            self.assertNotIn(disallowed, w)

    def test_prepared_manifest_and_source_tag_cannot_drift(self):
        self.assertEqual(self.plan["tag"], "v0.5.98")
        self.assertEqual(self.plan["previousTag"], "v0.5.97")
        self.assertEqual(self.plan["prNumber"], 295)
        self.assertEqual(self.plan["branch"], "feat/chaos-four-corner-suspension")
        self.assertEqual(self.plan["sourceSha"],
                         "bc0d5606d4c415f7603cc97f5ba6c90f59fa9c37")
        self.assertEqual(self.plan["previousSha"],
                         "65ef6f2f192081b64f8256867403f40882cca462")
        self.assertEqual(self.plan["notesPath"], "docs/releases/v0.5.98.md")
        self.assertIn("RELEASE_COMMIT: " + self.plan["sourceSha"],
                      self.workflow)
        self.assertIn("RELEASE_COMMIT: " + self.plan["sourceSha"],
                      self.publisher)
        self.assertIn("guard.publish(", self.publisher)
        self.assertIn("ZERO", self.docs.upper())
        self.assertIn("SOURCE-ONLY PRE-RELEASE", self.notes)
        self.assertIn("NO PLAYABLE WINDOWS EXECUTABLE", self.notes)
        self.assertIn("V0598_SOURCE_READINESS_QA.md", self.nav)

    def test_preflight_only_authenticates_and_leaves_source_tests_for_publisher(self):
        self.assertIn("PR #295", self.workflow)
        self.assertIn("protected main", self.workflow)
        self.assertIn("zero-asset release", self.workflow)
        self.assertIn("notes = fs.readFileSync(plan.notesPath, 'utf8')",
                      self.workflow)
        self.assertIn("plan.sourceSha !== process.env.RELEASE_COMMIT", self.workflow)
        self.assertIn("await core.summary.write();", self.workflow)
        self.assertNotIn("cmake --build", self.workflow)
        self.assertNotIn("ctest --test-dir", self.workflow)
        self.assertIn("source-tests:", self.publisher)
        self.assertIn("editor-tests:", self.publisher)
        self.assertIn("documentation:", self.publisher)
        self.assertIn("packaging-policy:", self.publisher)


if __name__ == "__main__":
    unittest.main()
