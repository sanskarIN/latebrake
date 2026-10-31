"""Source-only contracts for read-only UE 5.8 workstation prerequisite checks.

Never treats synthetic placeholder files as evidence of real Unreal compiler,
UHT, cooker, packaged game or installed gameplay.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealWorkstationPrerequisiteBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.probe = read("scripts/Test-UnrealWorkstationPrerequisites.ps1")
        cls.fixture = read("scripts/Test-UnrealWorkstationPrerequisitesPolicy.ps1")
        cls.pipeline = read("scripts/Build-PlayableCandidate.ps1")
        cls.win64 = read("scripts/Build-WindowsPlaytest.ps1")
        cls.ci = read(".github/workflows/core-tests.yml")
        cls.docs = read("docs/UE58_WORKSTATION_QA.md")

    def test_real_engine_paths_and_project_metadata_are_checked(self):
        for name in (
            "Engine/Build/Build.version", "Engine/Build/BatchFiles/Build.bat",
            "Engine/Build/BatchFiles/RunUAT.bat",
            "Engine/Binaries/Win64/UnrealEditor.exe",
            "Latebrake.uproject", "EngineAssociation", "MajorVersion",
            "MinorVersion", "Get-FileHash", "rev-parse --verify HEAD",
            "Get-Command git",
        ):
            with self.subTest(token=name):
                self.assertIn(name, self.probe)

    def test_builders_preflight_before_any_package_work(self):
        marker = "Test-UnrealWorkstationPrerequisites.ps1"
        self.assertLess(self.pipeline.index(marker),
                        self.pipeline.index("Build-Editor.ps1"))
        self.assertLess(self.win64.index(marker),
                        self.win64.index("& (Join-Path $PSScriptRoot 'Build-PlayableCandidate.ps1')"))
        self.assertIn("-Platform $Platform", self.pipeline)
        self.assertIn("-Platform Win64", self.win64)

    def test_synthetic_gates_are_enforced_on_windows_ci(self):
        self.assertIn("Test-UnrealWorkstationPrerequisites.ps1", self.ci)
        self.assertIn("Test-UnrealWorkstationPrerequisitesPolicy.ps1", self.ci)
        self.assertIn("run: ./scripts/Test-UnrealWorkstationPrerequisitesPolicy.ps1", self.ci)
        for token in (
            "Assert-Rejected 'missing Unreal directory'",
            "Assert-Rejected 'wrong UE minor version'",
            "Assert-Rejected 'wrong UE major version'",
            "Assert-Rejected 'malformed engine metadata'",
            "SYNTHETIC PLACEHOLDER - NOT UNREAL",
        ):
            self.assertIn(token, self.fixture)

    def test_preflight_cannot_manufacture_evidence(self):
        for forbidden in (
            "New-Item", "Set-Content", "Out-File", "Start-Process",
            "Build-Editor.ps1", "Build-PreRelease.ps1",
            "Build-PlayableCandidate.ps1", "gh release", "upload-artifact",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, self.probe)
        for marker in ("NOT VERIFIED", "MSVC/SDK", "installed gameplay"):
            self.assertIn(marker, self.probe)
        self.assertIn("read-only", self.docs.lower())
        self.assertIn("source-only", self.docs.lower() + "\n" + self.fixture.lower())


if __name__ == "__main__":
    unittest.main()
