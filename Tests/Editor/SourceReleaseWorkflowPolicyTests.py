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
        "v0.5.77": "v0.5.76",  # Chassis G estimates and private Win64 QA.
        "v0.5.78": "v0.5.77",  # Wheel visuals and rival lane-aware AI.
        "v0.5.79": "v0.5.78",  # Run-local best-lap ghost and rear signals.
        "v0.5.80": "v0.5.79",  # Stable chassis, rival following and rear camera.
        "v0.5.81": "v0.5.80",  # Human speed camera, brake input and estimate.
        "v0.5.82": "v0.5.81",  # Race-local tires, nitro, and verified drafting.
        "v0.5.83": "v0.5.82",  # Grip-aware autonomous corner pace.
        "v0.5.84": "v0.5.83",  # Measured paved/off-pavement chassis traction.
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
    if version == "v0.5.84":
        for new_gate in (
            "ArcadeSurfaceTraction|RivalGripPace|ArcadeTireCondition",
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeSurfaceTraction$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^RivalGripPace$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeTireCondition$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeNitroBoost$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^SlipstreamDraftBoost$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeBoostArbitration$" --no-tests=error',
            "Test-CandidatePairIdentityPolicy.ps1",
            "'OFF-PAVEMENT TRACTION'",
        ):
            require(new_gate, "v0.5.84 surface traction and racing provenance gates")

    if version == "v0.5.83":
        for new_gate in (
            "RivalGripPace|ArcadeTireCondition",
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^RivalGripPace$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeTireCondition$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeNitroBoost$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^SlipstreamDraftBoost$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeBoostArbitration$" --no-tests=error',
            "Test-CandidatePairIdentityPolicy.ps1",
            "'ARCADE TIRE', 'NITRO BOOST', 'SLIPSTREAM', 'RIVAL GRIP'",
        ):
            require(new_gate, "v0.5.83 AI grip and existing racecraft policy gates")

    if version == "v0.5.82":
        for new_gate in (
            "ArcadeTireCondition|ArcadeNitroBoost|SlipstreamDraftBoost|ArcadeBoostArbitration",
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeTireCondition$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeNitroBoost$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^SlipstreamDraftBoost$" --no-tests=error',
            'run: ctest --test-dir Tests/Standalone/build -C Release -R "^ArcadeBoostArbitration$" --no-tests=error',
            "Test-CandidatePairIdentityPolicy.ps1",
            "'ARCADE TIRE', 'NITRO BOOST', 'SLIPSTREAM'",
        ):
            require(new_gate, "v0.5.82 racing source and fail-closed target gates")

    if version == "v0.5.81":
        for new_gate in (
            "SpeedReactiveCameraFov|HandbrakeDrivePolicy|ObservedBrakeDistance",
            "FrameStableDamping",
            "HeldLookBackCamera",
            "RivalFollowingBrakeHold",
            "Test-WindowsRuntimePlaytestPolicy.ps1",
            "Test-CandidateManifestPolicy.ps1",
        ):
            require(new_gate, "v0.5.81 camera/brake telemetry source gates")

    if version == "v0.5.80":
        for new_gate in (
            "ChassisSlipAngleMeter|HeldLookBackCamera",
            "RivalFollowingBrakeHold",
            "FrameStableDamping",
            "Test-WindowsPlaytestWrapperPolicy.ps1",
            "Test-WindowsRuntimePlaytestPolicy.ps1",
            "Test-WindowsLogDirectoryPolicy.ps1",
        ):
            require(new_gate, "v0.5.80 camera/AI/physics source gates")

    if version == "v0.5.79":
        for new_gate in (
            "BestLapGhost|VehicleSignalLights",
            "Test-WindowsPlaytestWrapperPolicy.ps1",
            "Test-WindowsRuntimePlaytestPolicy.ps1",
            "Test-WindowsLogDirectoryPolicy.ps1",
        ):
            require(new_gate, "v0.5.79 ghost/signals and Win64 source checks")

    if version == "v0.5.78":
        for new_gate in (
            "WheelVisualMotion|RivalTrafficAwareness",
            "Test-WindowsPlaytestWrapperPolicy.ps1",
            "Test-WindowsRuntimePlaytestPolicy.ps1",
            "Test-WindowsLogDirectoryPolicy.ps1",
        ):
            require(new_gate, "v0.5.78 gameplay/Win64 source qualification")

    if version == "v0.5.77":
        for new_gate in (
            "VehicleGForceMeter|LaunchSpeedMeter",
            "Test-WindowsPlaytestWrapperPolicy.ps1",
            "Test-WindowsRuntimePlaytestPolicy.ps1",
            "Test-WindowsLogDirectoryPolicy.ps1",
        ):
            require(new_gate, "v0.5.77 feature and Win64 synthetic acceptance")

    if source.count("ref: ${{ github.sha }}") != 5:
        raise ValueError(f"{version}: each qualification/release job must pin same SHA")

    for warning in ("SOURCE-ONLY", "NOT a playable game build",
                    "No compiled Win64 EXE", "Android APK/AAB",
                    "not installers", "zero executable binaries"):
        if warning not in notes:
            raise ValueError(f"{version}: release notes missing {warning}")


def verify_modern_source_release(version: str, source: str, notes: str) -> None:
    """Refuse obvious duplicate jobs, broken CTest quoting or drifting tags."""
    if version in {"v0.5.60", "v0.5.65", "v0.5.70", "v0.5.71", "v0.5.72", "v0.5.73", "v0.5.74", "v0.5.75", "v0.5.76", "v0.5.77", "v0.5.78", "v0.5.79", "v0.5.80", "v0.5.81", "v0.5.82", "v0.5.83", "v0.5.84"}:
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

    def test_v0577_g_force_source_publisher_retains_all_qualification_gates(self):
        source = (WORKFLOWS / "publish-v0.5.77-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.77.md").read_text(
            encoding="utf-8"
        )
        verify_live_ci_source_release("v0.5.77", source, notes)
        self.assertIn("VehicleGForceMeter|LaunchSpeedMeter", source)
        self.assertIn("zero executable binaries", notes)
        for broken in (
            source.replace("tag: 'v0.5.76'", "tag: 'v0.5.75'", 1),
            source.replace("Test-WindowsLogDirectoryPolicy.ps1",
                           "Test-Placeholder.ps1", 1),
            source.replace("VehicleGForceMeter|LaunchSpeedMeter",
                           "LaunchSpeedMeter", 1),
            source.replace("ref: ${{ github.sha }}", "ref: main", 1),
            source.replace("published.assets.length !== 0",
                           "false /* no asset check */", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("needs: [source-tests, editor-tests, packaging-policy, documentation]",
                           "needs: [source-tests]", 1),
        ):
            with self.subTest(marker=broken[-95:]):
                with self.assertRaises(ValueError):
                    verify_live_ci_source_release("v0.5.77", broken, notes)

    def test_v0578_source_release_checks_actual_wheels_and_rival(self):
        source = (WORKFLOWS / "publish-v0.5.78-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.78.md").read_text(
            encoding="utf-8"
        )
        verify_live_ci_source_release("v0.5.78", source, notes)
        for mutation in (
            source.replace("WheelVisualMotion|RivalTrafficAwareness",
                           "VehicleGForceMeter", 1),
            source.replace("tag: 'v0.5.77'", "tag: 'v0.5.76'", 1),
            source.replace("published.assets.length !== 0", "false", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("Test-WindowsLogDirectoryPolicy.ps1",
                           "Test-DummyPolicy.ps1", 1),
        ):
            with self.subTest(mutation=mutation[-80:]):
                with self.assertRaises(ValueError):
                    verify_live_ci_source_release("v0.5.78", mutation, notes)

    def test_v0584_pavement_publisher_has_strict_surface_and_legacy_gates(self):
        version = "v0.5.84"
        source = (WORKFLOWS / "publish-v0.5.84-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.84.md").read_text(
            encoding="utf-8"
        )
        verify_live_ci_source_release(version, source, notes)
        for mutant in (
            source.replace("ArcadeSurfaceTraction|RivalGripPace|ArcadeTireCondition",
                           "RivalGripPace|ArcadeTireCondition", 1),
            source.replace('^ArcadeSurfaceTraction$" --no-tests=error',
                           '^ArcadeSurfaceTraction$" --output-on-failure', 1),
            source.replace('^RivalGripPace$" --no-tests=error',
                           '^RivalGripPace$" --output-on-failure', 1),
            source.replace("tag: 'v0.5.83'", "tag: 'v0.5.82'", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0", "false /* unsafe */", 1),
            source.replace("needs: [source-tests, editor-tests, packaging-policy, documentation]",
                           "needs: [source-tests]", 1),
            source.replace("'OFF-PAVEMENT TRACTION'", "'UNRELATED'", 1),
        ):
            self.assertNotEqual(mutant, source)
            with self.subTest(fragment=mutant[:85]):
                with self.assertRaises(ValueError):
                    verify_live_ci_source_release(version, mutant, notes)

    def test_v0583_source_publisher_requires_grip_corners_and_legacy_racing(self):
        version = "v0.5.83"
        source = (WORKFLOWS / "publish-v0.5.83-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.83.md").read_text(
            encoding="utf-8"
        )
        verify_live_ci_source_release(version, source, notes)
        for mutation in (
            source.replace("RivalGripPace|ArcadeTireCondition",
                           "ArcadeTireCondition", 1),
            source.replace('^RivalGripPace$" --no-tests=error',
                           '^RivalGripPace$" --output-on-failure', 1),
            source.replace('^ArcadeTireCondition$" --no-tests=error',
                           '^ArcadeTireCondition$" --output-on-failure', 1),
            source.replace("tag: 'v0.5.82'", "tag: 'v0.5.81'", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0",
                           "false /* unsafe */", 1),
            source.replace("needs: [source-tests, editor-tests, packaging-policy, documentation]",
                           "needs: [source-tests]", 1),
            source.replace("'RIVAL GRIP'", "'UNRELATED'", 1),
        ):
            self.assertNotEqual(mutation, source)
            with self.subTest(fragment=mutation[:80]):
                with self.assertRaises(ValueError):
                    verify_live_ci_source_release(version, mutation, notes)

    def test_v0582_source_publisher_requires_all_qualified_racing_systems(self):
        version = "v0.5.82"
        source = (WORKFLOWS / "publish-v0.5.82-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.82.md").read_text(
            encoding="utf-8"
        )
        verify_live_ci_source_release(version, source, notes)
        for mutant in (
            source.replace("ArcadeTireCondition|ArcadeNitroBoost|SlipstreamDraftBoost|ArcadeBoostArbitration",
                           "ArcadeTireCondition|ArcadeNitroBoost|SlipstreamDraftBoost", 1),
            source.replace('^ArcadeBoostArbitration$" --no-tests=error',
                           '^ArcadeBoostArbitration$" --output-on-failure', 1),
            source.replace('^SlipstreamDraftBoost$" --no-tests=error',
                           '^SlipstreamDraftBoost$" --output-on-failure', 1),
            source.replace("tag: 'v0.5.81'", "tag: 'v0.5.80'", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0", "false /* unsafe */", 1),
            source.replace("needs: [source-tests, editor-tests, packaging-policy, documentation]",
                           "needs: [source-tests]", 1),
        ):
            self.assertNotEqual(mutant, source)
            with self.subTest(fragment=mutant[:80]):
                with self.assertRaises(ValueError):
                    verify_live_ci_source_release(version, mutant, notes)

    def test_v0581_source_release_requires_measured_brake_and_camera(self):
        source = (WORKFLOWS / "publish-v0.5.81-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.81.md").read_text(
            encoding="utf-8"
        )
        verify_live_ci_source_release("v0.5.81", source, notes)
        for mutation in (
            source.replace("HandbrakeDrivePolicy|ObservedBrakeDistance",
                           "HandbrakeDrivePolicy", 1),
            source.replace("tag: 'v0.5.80'", "tag: 'v0.5.79'", 1),
            source.replace("FrameStableDamping", "VehicleSignalLights", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0", "false", 1),
        ):
            with self.subTest(mutation=mutation[-95:]):
                with self.assertRaises(ValueError):
                    verify_live_ci_source_release("v0.5.81", mutation, notes)

    def test_v0580_source_release_requires_camera_ai_and_frame_safety(self):
        source = (WORKFLOWS / "publish-v0.5.80-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.80.md").read_text(
            encoding="utf-8"
        )
        verify_live_ci_source_release("v0.5.80", source, notes)
        for mutation in (
            source.replace("ChassisSlipAngleMeter|HeldLookBackCamera",
                           "ChassisSlipAngleMeter", 1),
            source.replace("tag: 'v0.5.79'", "tag: 'v0.5.78'", 1),
            source.replace("FrameStableDamping", "VehicleSignalLights", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("published.assets.length !== 0", "false", 1),
        ):
            with self.subTest(mutation=mutation[-95:]):
                with self.assertRaises(ValueError):
                    verify_live_ci_source_release("v0.5.80", mutation, notes)

    def test_v0579_source_release_requires_real_ghost_and_light_policies(self):
        source = (WORKFLOWS / "publish-v0.5.79-source.yml").read_text(
            encoding="utf-8"
        )
        notes = (RELEASE_NOTES / "v0.5.79.md").read_text(
            encoding="utf-8"
        )
        verify_live_ci_source_release("v0.5.79", source, notes)
        for mutation in (
            source.replace("BestLapGhost|VehicleSignalLights",
                           "VehicleSignalLights", 1),
            source.replace("tag: 'v0.5.78'", "tag: 'v0.5.77'", 1),
            source.replace("published.assets.length !== 0", "false", 1),
            source.replace("if (main !== sha)", "if (false)", 1),
            source.replace("needs: [source-tests, editor-tests, packaging-policy, documentation]",
                           "needs: [source-tests, editor-tests]", 1),
        ):
            with self.subTest(mutation=mutation[-90:]):
                with self.assertRaises(ValueError):
                    verify_live_ci_source_release("v0.5.79", mutation, notes)

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
