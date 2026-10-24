"""Structural regression guards for modern source-only release workflows.

These tests intentionally need only Python's standard library, so they run
inside the existing Editor-source CI job without licensed Unreal or PyYAML.
They are NOT a complete YAML parser. GitHub Action execution remains the
authoritative semantic validation.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WORKFLOWS = ROOT / ".github" / "workflows"
RELEASE_NOTES = ROOT / "docs" / "releases"

REQUIRED_JOBS = ("portable-tests", "powershell-policy", "editor-policy", "release")
_SHA = r"[0-9a-f]{40}"


def verify_modern_source_release(version: str, source: str, notes: str) -> None:
    """Refuse obvious duplicate jobs, broken CTest quoting or drifting tags."""
    found_version = re.fullmatch(r"v0\.5\.(\d+)", version)
    if not found_version or int(found_version.group(1)) < 32:
        raise ValueError("only immutable source workflow versions >= v0.5.32 supported")

    for job in REQUIRED_JOBS:
        if len(re.findall(r"^  " + re.escape(job) + r":\s*$", source, re.M)) != 1:
            raise ValueError(f"{version}: missing or duplicate job {job}")

    def require(needle: str, description: str) -> None:
        if needle not in source:
            raise ValueError(f"{version}: missing {description}")

    require("  push:\n    branches: [main]", "main-branch trigger")
    require(f"- '.github/workflows/publish-{version}-source.yml'",
            "self-triggered workflow path")
    require("  workflow_dispatch:", "manual retry entry point")
    require("os: [ubuntu-latest, windows-latest, macos-latest]",
            "all three portable targets")
    require("needs: [portable-tests, powershell-policy, editor-policy]",
            "all source gates")
    require("uses: actions/github-script@v7", "release action")
    require("github.rest.repos.compareCommits(", "ancestor validation")
    require("release.assets.length", "no executable attachments policy")
    require("prerelease: true", "explicit pre-release flag")
    require("make_latest: 'false'", "no stable/latest promotion")
    require(f"const tag = '{version}';", "pinned release identity")
    require(f"docs/releases/{version}.md", "correct release notes")

    # The prior release must have been a source-only published preview.
    previous = f"v0.5.{int(found_version.group(1)) - 1}"
    require(f"tag: '{previous}'", "predecessor release verification")

    # All three test jobs checkout the same immutable source candidate.
    refs = re.findall(r"^          ref: (" + _SHA + r")$", source, re.M)
    sha = re.search(r"^            const sha = '(" + _SHA + r")';$", source, re.M)
    if len(refs) != 3 or sha is None or any(ref != sha.group(1) for ref in refs):
        raise ValueError(f"{version}: release and test source revisions diverged")

    # This catches the historical templating bug which removed the closing
    # apostrophe and accidentally duplicated the rest of the workflow.
    matches = re.findall(
        r"^        run: ctest --test-dir Tests/Standalone/build -C Release "
        r"-R '\^([A-Za-z_][A-Za-z_0-9]*)\$' --output-on-failure$",
        source, re.M,
    )
    if len(matches) != 1:
        raise ValueError(f"{version}: malformed or duplicate isolated CTest regex")
    cmake = (ROOT / "Tests" / "Standalone" / "CMakeLists.txt").read_text(
        encoding="utf-8"
    )
    if f"add_test(NAME {matches[0]} " not in cmake:
        raise ValueError(f"{version}: focused CTest is not registered")

    for phrase in ("SOURCE-ONLY", "NOT a playable game build",
                   "No compiled Win64 EXE", "Android APK/AAB", "not installers"):
        if phrase not in notes:
            raise ValueError(f"{version}: release notes omit warning {phrase}")


class SourceReleaseWorkflowPolicyTests(unittest.TestCase):
    def test_every_modern_source_release_workflow_is_well_formed(self):
        paths = sorted(WORKFLOWS.glob("publish-v0.5.*-source.yml"))
        selected = []
        for path in paths:
            found = re.fullmatch(r"publish-(v0\.5\.(\d+))-source\.yml", path.name)
            if found and int(found.group(2)) >= 32:
                selected.append((found.group(1), path))
        self.assertGreaterEqual(len(selected), 2, "modern v0.5.32/33 workflows missing")
        for version, path in selected:
            with self.subTest(version=version):
                notes_file = RELEASE_NOTES / f"{version}.md"
                self.assertTrue(notes_file.is_file(), f"notes absent: {version}")
                verify_modern_source_release(
                    version, path.read_text(encoding="utf-8"),
                    notes_file.read_text(encoding="utf-8"),
                )

    def test_known_yaml_truncation_and_revision_drift_are_rejected(self):
        version = "v0.5.32"
        source = (WORKFLOWS / "publish-v0.5.32-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.32.md").read_text(encoding="utf-8")
        verify_modern_source_release(version, source, notes)
        sha = re.search(r"const sha = '([0-9a-f]{40})';", source)
        self.assertIsNotNone(sha)
        broken = (
            source.replace("-R '^CurrentLapPotential$'",
                           "-R '^CurrentLapPotential", 1),
            source.replace("\n  release:\n", "\n  release:\n  release:\n", 1),
            source.replace(sha.group(1), "a" * 40, 1),
            source.replace("const tag = 'v0.5.32';",
                           "const tag = 'v0.5.31';", 1),
            source.replace("docs/releases/v0.5.32.md",
                           "docs/releases/placeholder.md", 1),
        )
        for invalid in broken:
            with self.subTest(fragment=invalid[:100]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release(version, invalid, notes)

    def test_required_release_warnings_fail_closed(self):
        version = "v0.5.32"
        source = (WORKFLOWS / "publish-v0.5.32-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.32.md").read_text(encoding="utf-8")
        with self.assertRaises(ValueError):
            verify_modern_source_release(
                version, source, notes.replace("not installers", "source files")
            )


if __name__ == "__main__":
    unittest.main()
