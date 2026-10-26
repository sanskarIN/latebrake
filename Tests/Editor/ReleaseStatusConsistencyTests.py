"""Keep site, README and release-history labels aligned.

A source-only status test, not a GitHub release API check, game build,
Unreal Editor/UHT verification or proof of installed gameplay.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def extract_latest_release_versions(documents: dict[str, str]) -> dict[str, str]:
    """Read independently maintained *latest published* labels, not history."""
    patterns = {
        "README.md": r"Latest published:\s*\[v(\d+\.\d+\.\d+)\]",
        "docs/STATUS.md": r"Latest published milestone:\s*\[v(\d+\.\d+\.\d+)",
        "docs/development-status.md": (
            r"Latest published GitHub pre-release\s*\|\s*\[v(\d+\.\d+\.\d+)"
        ),
        "docs/release-history.md": (
            r"latest published GitHub tag is \*\*v(\d+\.\d+\.\d+)\*\*"
        ),
        "docs/index.md": (
            r"latest GitHub source tag is <a href=\"[^\"]+/releases/tag/"
            r"v(\d+\.\d+\.\d+)\">"
        ),
    }
    versions = {}
    for path, pattern in patterns.items():
        match = re.search(pattern, documents[path], flags=re.IGNORECASE)
        if not match:
            raise ValueError(f"{path}: missing explicit latest published version")
        versions[path] = match.group(1)
    return versions


def require_consistent_release_status(documents: dict[str, str]) -> str:
    versions = extract_latest_release_versions(documents)
    if len(set(versions.values())) != 1:
        raise ValueError(f"latest published source tag differs: {versions}")
    version = next(iter(versions.values()))
    history = documents["docs/release-history.md"]
    if not re.search(
        rf"(?m)^\| v{re.escape(version)} \| "
        rf"\[Release notes\]\(releases/v{re.escape(version)}\.md\) \|$",
        history,
    ):
        raise ValueError(f"missing release-history notes row for v{version}")
    return version


class ReleaseStatusConsistencyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.documents = {
            path: (ROOT / path).read_text(encoding="utf-8")
            for path in (
                "README.md", "docs/STATUS.md",
                "docs/development-status.md",
                "docs/release-history.md", "docs/index.md",
            )
        }

    def test_every_public_latest_source_release_label_matches(self):
        version = require_consistent_release_status(self.documents)
        self.assertRegex(version, r"^0\.\d+\.\d+$")
        notes = ROOT / "docs" / "releases" / f"v{version}.md"
        self.assertTrue(notes.is_file(), f"missing notes for published v{version}")

    def test_current_release_notes_disclose_source_only_limitation(self):
        version = require_consistent_release_status(self.documents)
        text = (ROOT / "docs" / "releases" / f"v{version}.md").read_text(
            encoding="utf-8"
        )
        for marker in (
            "SOURCE-ONLY",
            "NOT a playable game build",
            "No compiled Win64 EXE",
            "Android APK/AAB",
            "not installers",
            "zero executable binaries",
        ):
            self.assertIn(marker, text)

    def test_stale_status_is_rejected(self):
        mismatch = dict(self.documents)
        mismatch["docs/index.md"] = mismatch["docs/index.md"].replace(
            "latest GitHub source tag is <a href=",
            "latest GitHub source tag is <a href=",
        ).replace(
            "/releases/tag/v0.5.76", "/releases/tag/v0.5.75", 1
        )
        with self.assertRaises(ValueError):
            require_consistent_release_status(mismatch)

    def test_missing_site_status_is_rejected(self):
        mismatch = dict(self.documents)
        mismatch["docs/index.md"] = mismatch["docs/index.md"].replace(
            "latest GitHub source tag is", "archived GitHub source tag was", 1
        )
        with self.assertRaises(ValueError):
            require_consistent_release_status(mismatch)


if __name__ == "__main__":
    unittest.main()
