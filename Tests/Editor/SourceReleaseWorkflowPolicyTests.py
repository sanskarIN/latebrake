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


def verify_live_ci_source_release(version: str, source: str, notes: str) -> None:
    """Validate authorized post-merge source-only snapshot release models.

    The historical v0.5.32-v0.5.46 workflows pin a known older SHA. This
    workflow tests its *own* merged main SHA and releases that exact SHA,
    so it must not claim a non-existent v0.5.59 predecessor release.
    """
    previous_by_version = {
        "v0.5.60": "v0.5.46",
        "v0.5.65": "v0.5.60",  # Intermediate source milestones were not tagged.
        "v0.5.70": "v0.5.65",  # Includes safe HUD and completed-lap review.
        "v0.5.71": "v0.5.70",  # Real finished-run insights and sampled telemetry.
        "v0.5.72": "v0.5.71",  # Four native views and press-only Android CAM.
        "v0.5.73": "v0.5.72",  # Run-local arcade drift combos.
        "v0.5.74": "v0.5.73",  # Rescaled human controller deadzones.
        "v0.5.75": "v0.5.74",  # Exact post-green frame timing gate.
        "v0.5.76": "v0.5.75",  # Signed forward launch telemetry.
    }
    if version not in previous_by_version:
        raise ValueError("unsupported dynamic source publisher")
    previous_tag = previous_by_version[version]

    def require(value: str, name: str) -> None:
        if value not in source:
            raise ValueError(f"{version}: missing {name}")

    jobs = ("source-tests", "editor-tests", "packaging-policy",
            "documentation", "source-prerelease")
    for job in jobs:
        if len(re.findall(r"^  " + re.escape(job) + r":\s*$",
                          source, re.M)) != 1:
            raise ValueError(f"{version}: missing or duplicate job {job}")

    for value, name in (
        ("  push:\n    branches: [main]", "main push trigger"),
        (f"- '.github/workflows/publish-{version}-source.yml'",
         "self-trigger only on main"),
        ("  workflow_dispatch:", "manual retry capability"),
        ("os: [ubuntu-latest, windows-latest, macos-latest]",
         "all three real standalone build targets"),
        ("cmake --build Tests/Standalone/build", "real production compile"),
        ("ctest --test-dir Tests/Standalone/build", "complete test suite"),
        ("python3 -m unittest discover", "Unreal binding verification"),
        ("Test-PackagingPolicy.ps1", "synthetic archive policy"),
        ("python scripts/verify_docs_site.py site", "documentation integrity"),
        ("needs: [source-tests, editor-tests, packaging-policy, documentation]",
         "all four mandatory publication gates"),
        ("uses: actions/github-script@v7", "authenticated GitHub release"),
        (f"const tag = '{version}';", "correct immutable version"),
        ("const sha = context.sha;", "CI-qualified run SHA"),
        ("ref: ${{ github.sha }}", "exact source in all jobs"),
        ("if (main !== sha)", "reject stale or superseded main checkout"),
        ("existingRef.sha !== sha", "reject retagging"),
        (f"tag: '{previous_tag}'", "previous published source snapshot"),
        ("prerelease: true", "pre-release flag"),
        ("make_latest: 'false'", "no stable latest promotion"),
        ("published.assets.length !== 0", "no executable attachments"),
        (f"docs/releases/{version}.md", "matching release notes"),
    ):
        require(value, name)
    if source.count("ref: ${{ github.sha }}") != 5:
        raise ValueError(f"{version}: each qualification/release job must pin same SHA")

    for warning in ("SOURCE-ONLY", "NOT a playable game build",
                    "No compiled Win64 EXE", "Android APK/AAB",
                    "not installers", "zero executable binaries"):
        if warning not in notes:
            raise ValueError(f"{version}: release notes missing {warning}")


def verify_modern_source_release(version: str, source: str, notes: str) -> None:
    """Refuse obvious duplicate jobs, broken CTest quoting or drifting tags."""
    if version in {"v0.5.60", "v0.5.65", "v0.5.70", "v0.5.71", "v0.5.72", "v0.5.73", "v0.5.74", "v0.5.75", "v0.5.76"}:
        verify_live_ci_source_release(version, source, notes)
        return
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

    def test_v0560_dynamic_source_release_rejects_missing_gates(self):
        source = (WORKFLOWS / "publish-v0.5.60-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.60.md").read_text(
            encoding="utf-8"
        )
        verify_modern_source_release("v0.5.60", source, notes)
        mutations = (
            source.replace("const sha = context.sha;",
                           "const sha = '0' * 40;", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe assets */", 1),
            source.replace(
                "needs: [source-tests, editor-tests, packaging-policy, documentation]",
                "needs: [source-tests]", 1),
            source.replace("prerelease: true", "prerelease: false", 1),
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
        )
        for invalid in mutations:
            with self.subTest(fragment=invalid[-100:]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release("v0.5.60", invalid, notes)

    def test_v0565_dynamic_source_release_rejects_missing_gates(self):
        source = (WORKFLOWS / "publish-v0.5.65-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.65.md").read_text(
            encoding="utf-8"
        )
        verify_modern_source_release("v0.5.65", source, notes)
        mutations = (
            source.replace("const sha = context.sha;",
                           "const sha = '0' * 40;", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe assets */", 1),
            source.replace("needs: [source-tests, editor-tests, packaging-policy, documentation]",
                           "needs: [source-tests]", 1),
            source.replace("prerelease: true", "prerelease: false", 1),
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
            source.replace("tag: 'v0.5.60'",
                           "tag: 'v0.5.64'", 1),
        )
        for invalid in mutations:
            with self.subTest(fragment=invalid[-100:]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release("v0.5.65", invalid, notes)

    def test_v0570_dynamic_source_release_rejects_missing_gates(self):
        source = (WORKFLOWS / "publish-v0.5.70-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.70.md").read_text(
            encoding="utf-8"
        )
        verify_modern_source_release("v0.5.70", source, notes)
        mutations = (
            source.replace("const sha = context.sha;",
                           "const sha = '0' * 40;", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe assets */", 1),
            source.replace(
                "needs: [source-tests, editor-tests, packaging-policy, documentation]",
                "needs: [source-tests]", 1),
            source.replace("prerelease: true", "prerelease: false", 1),
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
            source.replace("tag: 'v0.5.65'",
                           "tag: 'v0.5.69'", 1),
        )
        for invalid in mutations:
            with self.subTest(fragment=invalid[-100:]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release("v0.5.70", invalid, notes)

    def test_v0571_dynamic_source_release_rejects_missing_gates(self):
        source = (WORKFLOWS / "publish-v0.5.71-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.71.md").read_text(
            encoding="utf-8"
        )
        verify_modern_source_release("v0.5.71", source, notes)
        mutations = (
            source.replace("const sha = context.sha;",
                           "const sha = '0' * 40;", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe assets */", 1),
            source.replace(
                "needs: [source-tests, editor-tests, packaging-policy, documentation]",
                "needs: [source-tests]", 1),
            source.replace("prerelease: true", "prerelease: false", 1),
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
            source.replace("tag: 'v0.5.70'",
                           "tag: 'v0.5.69'", 1),
        )
        for invalid in mutations:
            with self.subTest(fragment=invalid[-100:]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release("v0.5.71", invalid, notes)

    def test_v0572_dynamic_source_release_rejects_missing_gates(self):
        source = (WORKFLOWS / "publish-v0.5.72-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.72.md").read_text(
            encoding="utf-8"
        )
        verify_modern_source_release("v0.5.72", source, notes)
        mutations = (
            source.replace("const sha = context.sha;",
                           "const sha = '0' * 40;", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe assets */", 1),
            source.replace(
                "needs: [source-tests, editor-tests, packaging-policy, documentation]",
                "needs: [source-tests]", 1),
            source.replace("prerelease: true", "prerelease: false", 1),
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
            source.replace("tag: 'v0.5.71'",
                           "tag: 'v0.5.69'", 1),
        )
        for invalid in mutations:
            with self.subTest(fragment=invalid[-100:]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release("v0.5.72", invalid, notes)

    def test_v0573_dynamic_source_release_rejects_missing_gates(self):
        source = (WORKFLOWS / "publish-v0.5.73-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.73.md").read_text(
            encoding="utf-8"
        )
        verify_modern_source_release("v0.5.73", source, notes)
        mutations = (
            source.replace("const sha = context.sha;",
                           "const sha = '0' * 40;", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe assets */", 1),
            source.replace(
                "needs: [source-tests, editor-tests, packaging-policy, documentation]",
                "needs: [source-tests]", 1),
            source.replace("prerelease: true", "prerelease: false", 1),
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
            source.replace("tag: 'v0.5.72'",
                           "tag: 'v0.5.69'", 1),
        )
        for invalid in mutations:
            with self.subTest(fragment=invalid[-100:]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release("v0.5.73", invalid, notes)

    def test_v0574_dynamic_source_release_rejects_missing_gates(self):
        source = (WORKFLOWS / "publish-v0.5.74-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.74.md").read_text(
            encoding="utf-8"
        )
        verify_modern_source_release("v0.5.74", source, notes)
        mutations = (
            source.replace("const sha = context.sha;",
                           "const sha = '0' * 40;", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe assets */", 1),
            source.replace(
                "needs: [source-tests, editor-tests, packaging-policy, documentation]",
                "needs: [source-tests]", 1),
            source.replace("prerelease: true", "prerelease: false", 1),
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
            source.replace("tag: 'v0.5.73'",
                           "tag: 'v0.5.69'", 1),
        )
        for invalid in mutations:
            with self.subTest(fragment=invalid[-100:]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release("v0.5.74", invalid, notes)

    def test_v0575_dynamic_source_release_rejects_missing_gates(self):
        source = (WORKFLOWS / "publish-v0.5.75-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.75.md").read_text(
            encoding="utf-8"
        )
        verify_modern_source_release("v0.5.75", source, notes)
        mutations = (
            source.replace("const sha = context.sha;",
                           "const sha = '0' * 40;", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe assets */", 1),
            source.replace(
                "needs: [source-tests, editor-tests, packaging-policy, documentation]",
                "needs: [source-tests]", 1),
            source.replace("prerelease: true", "prerelease: false", 1),
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
            source.replace("tag: 'v0.5.74'",
                           "tag: 'v0.5.69'", 1),
        )
        for invalid in mutations:
            with self.subTest(fragment=invalid[-100:]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release("v0.5.75", invalid, notes)

    def test_v0576_dynamic_source_release_rejects_missing_gates(self):
        source = (WORKFLOWS / "publish-v0.5.76-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.76.md").read_text(
            encoding="utf-8"
        )
        verify_modern_source_release("v0.5.76", source, notes)
        mutations = (
            source.replace("const sha = context.sha;",
                           "const sha = '0' * 40;", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe assets */", 1),
            source.replace(
                "needs: [source-tests, editor-tests, packaging-policy, documentation]",
                "needs: [source-tests]", 1),
            source.replace("prerelease: true", "prerelease: false", 1),
            source.replace("ref: ${{ github.sha }}",
                           "ref: main", 1),
            source.replace("tag: 'v0.5.75'",
                           "tag: 'v0.5.69'", 1),
        )
        for invalid in mutations:
            with self.subTest(fragment=invalid[-100:]):
                with self.assertRaises(ValueError):
                    verify_modern_source_release("v0.5.76", invalid, notes)

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
