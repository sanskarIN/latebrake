"""Manual v0.5.97 preparation wiring; publication behavior is tested offline in Node."""
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = ROOT / '.github/workflows/prepare-v0597-source.yml'


class ExperimentalV0597PreparationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workflow = WORKFLOW.read_text()
        cls.plan = json.loads((ROOT / 'scripts/releases/v0.5.97.json').read_text())

    def test_preparation_is_manual_only_with_an_exact_shared_source_pin(self):
        source = self.plan['sourceSha']
        self.assertRegex(source, r'^[0-9a-f]{40}$')
        self.assertIn('  workflow_dispatch:', self.workflow)
        for forbidden in ('  push:', '  pull_request:', 'pull_request_target:', 'schedule:'):
            self.assertNotIn(forbidden, self.workflow)
        self.assertIn('RELEASE_COMMIT: ' + source, self.workflow)
        self.assertEqual(self.workflow.count('          ref: ' + source), 4)
        self.assertEqual(self.plan['tag'], 'v0.5.97')
        self.assertEqual(self.plan['previousTag'], 'v0.5.96')
        self.assertEqual(self.plan['previousSha'], '5bbea98bc085f1ab7f8a6dde0cf6b9f9d160bdd7')

    def test_write_scope_runs_only_reviewed_main_code_after_all_source_gates(self):
        self.assertEqual(self.workflow.count('      contents: write'), 1)
        self.assertIn('needs: [preflight, source-tests, editor-tests, packaging-policy, documentation]', self.workflow)
        write_job = self.workflow.split('  source-prerelease:', 1)[1]
        self.assertIn('ref: ${{ github.sha }}', write_job)
        self.assertIn("github.actor == 'sanskarIN'", write_job)
        self.assertIn('guard.publish({github, context, plan, notes})', write_job)
        for forbidden in ('ref: ' + self.plan['sourceSha'], 'cmake ', 'pwsh', 'upload-artifact', 'uploadReleaseAsset'):
            self.assertNotIn(forbidden, write_job)
        self.assertEqual(self.workflow.count('persist-credentials: false'), 6)

    def test_complete_source_matrix_named_targets_and_docs_are_required(self):
        for marker in ('os: [windows-latest, ubuntu-latest, macos-latest]',
                       'ctest --test-dir Tests/Standalone/build -C Release --output-on-failure',
                       'SprungContactBraking BrakeRoadTangent;', '--no-tests=error',
                       "python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v",
                       'python3 scripts/MeasureSourceScale.py --json',
                       'Test-SourceProvenancePolicy.ps1', 'Test-WindowsRuntimePlaytestPolicy.ps1',
                       'python scripts/verify_docs_site.py site'):
            self.assertIn(marker, self.workflow)
        core = (ROOT / '.github/workflows/core-tests.yml').read_text()
        self.assertIn('node --test Tests/Release/ExperimentalSourceReleaseTests.cjs', core)

    def test_notes_and_operator_guide_preserve_unpublished_and_nonplayable_status(self):
        notes = (ROOT / self.plan['notesPath']).read_text()
        guide = (ROOT / 'docs/V0597_SOURCE_RELEASE_PREPARATION.md').read_text()
        for marker in ('SOURCE-ONLY PRE-RELEASE', 'NO PLAYABLE WINDOWS EXECUTABLE',
                       'NOT a playable game build', 'not installers', 'unmerged development'):
            self.assertIn(marker, notes)
        self.assertIn(self.plan['sourceSha'], guide)
        self.assertIn('not published by merging', guide)
        self.assertIn('prepare-v0597-source.yml', guide)
        self.assertNotRegex(notes, r'(?i)Unreal (?:runtime|compilation) (?:verified|passed)')


if __name__ == '__main__':
    unittest.main()
