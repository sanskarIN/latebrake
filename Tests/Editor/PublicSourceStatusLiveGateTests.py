"""Source contract for a LIVE release-status gate. Not Unreal QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class PublicSourceStatusLiveGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / ".github/workflows" /
                      "verify-public-source-status.yml").read_text(
                          encoding="utf-8")

    def test_read_only_real_release_identity_gate(self):
        for marker in (
            "on:",
            "  pull_request:",
            "  push:",
            "permissions:\n  contents: read",
            "persist-credentials: false",
            "uses: actions/github-script@v7",
            "getReleaseByTag({",
            "getRef({",
            "release.draft",
            "!release.prerelease",
            "!release.published_at",
            "release.assets.length !== 0",
            "ref.type !== 'commit'",
            "release.target_commitish !== ref.sha",
            "release.body !== notes",
            "fs.readFileSync('README.md'",
            "docs/releases/",
            "zero executable binaries",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, self.source)
        for forbidden in (
            "createRelease({", "updateRelease({", "deleteRelease({",
            "createRef({", "updateRef({", "deleteRef({",
            "uploadReleaseAsset({", "contents: write",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, self.source)

    def test_watch_all_five_public_status_documents(self):
        for path in (
            "'README.md'",
            "'docs/STATUS.md'",
            "'docs/index.md'",
            "'docs/development-status.md'",
            "'docs/release-history.md'",
            "'docs/releases/**'",
        ):
            self.assertIn(path, self.source)


if __name__ == "__main__":
    unittest.main()
