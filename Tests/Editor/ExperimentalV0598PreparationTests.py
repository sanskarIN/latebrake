"""Prepared v0.5.98 wiring; these tests do not contact GitHub or execute UE."""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ExperimentalV0598PreparationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.plan = json.loads((ROOT/'scripts/releases/v0.5.98.json').read_text())
        cls.workflow = (ROOT/'.github/workflows/prepare-v0598-source.yml').read_text()

    def test_manual_source_pin_and_sequential_immutable_predecessor(self):
        pin = self.plan['sourceSha']
        self.assertEqual(pin, 'bc0d5606d4c415f7603cc97f5ba6c90f59fa9c37')
        self.assertEqual(self.plan['tag'], 'v0.5.98')
        self.assertEqual(self.plan['previousTag'], 'v0.5.97')
        self.assertEqual(self.plan['previousSha'], '65ef6f2f192081b64f8256867403f40882cca462')
        self.assertIn('  workflow_dispatch:', self.workflow)
        for forbidden in ('  push:', '  pull_request:', 'pull_request_target:', 'schedule:'):
            self.assertNotIn(forbidden, self.workflow)
        self.assertIn('RELEASE_COMMIT: '+pin, self.workflow)
        self.assertEqual(self.workflow.count('          ref: '+pin), 4)

    def test_all_source_platform_and_named_regression_gates_are_required(self):
        for marker in ('os: [windows-latest, ubuntu-latest, macos-latest]',
                       'ctest --test-dir Tests/Standalone/build -C Release --output-on-failure',
                       '--no-tests=error', 'WheelBrakeDynamics FourCornerWheelBrakes',
                       'IndependentWheelVisual ServiceBrakeCapability',
                       'RivalBrakeCapacityPace RivalWheelBrakeApproach;',
                       "python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v",
                       'python3 scripts/MeasureSourceScale.py --json',
                       'Test-WindowsRuntimePlaytestPolicy.ps1',
                       'python scripts/verify_docs_site.py site'):
            self.assertIn(marker, self.workflow)

    def test_candidate_content_never_executes_in_the_write_scoped_job(self):
        self.assertEqual(self.workflow.count('      contents: write'), 1)
        self.assertIn('needs: [preflight, source-tests, editor-tests, packaging-policy, documentation]', self.workflow)
        write = self.workflow.split('  source-prerelease:', 1)[1]
        self.assertIn('ref: ${{ github.sha }}', write)
        self.assertIn("github.actor == 'sanskarIN'", write)
        self.assertIn('guard.publish({github, context, plan, notes})', write)
        for forbidden in ('ref: '+self.plan['sourceSha'], 'cmake ', 'pwsh',
                          'upload-artifact', 'uploadReleaseAsset'):
            self.assertNotIn(forbidden, write)

    def test_notes_and_guide_do_not_claim_publication_or_unreal_execution(self):
        notes = (ROOT/self.plan['notesPath']).read_text()
        guide = (ROOT/'docs/V0598_SOURCE_RELEASE_PREPARATION.md').read_text()
        for marker in ('SOURCE-ONLY PRE-RELEASE', 'NO PLAYABLE WINDOWS EXECUTABLE',
                       'NOT a playable game build', 'No compiled Win64 EXE',
                       'Android APK/AAB', 'not installers', 'zero executable binaries',
                       'zero attached game assets', 'unmerged development', 'road-tangent'):
            self.assertIn(marker, notes)
        self.assertIn(self.plan['sourceSha'], guide)
        self.assertIn('Prepared, not published', guide)
        self.assertIn('prepare-v0598-source.yml', guide)
        self.assertIn('100,000-line target', guide)


if __name__ == '__main__':
    unittest.main()
