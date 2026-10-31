"""Source-only guarantees for the private Windows Unreal build launcher.

These tests do not compile UE/UHT or produce a playable EXE.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WRAPPER = ROOT / "scripts/Build-WindowsPlaytest.ps1"
CMD = ROOT / "Build-Windows-Playtest.cmd"
DOCS = ROOT / "docs/WINDOWS_PERSONAL_PLAYTEST.md"
CI = ROOT / ".github/workflows/core-tests.yml"


class WindowsPlaytestSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.script = WRAPPER.read_text(encoding="utf-8")
        cls.launcher = CMD.read_text(encoding="utf-8")
        cls.docs = DOCS.read_text(encoding="utf-8")
        cls.ci = CI.read_text(encoding="utf-8")

    def test_game_is_built_by_real_engine_pipeline(self):
        for required in (
            "UnrealEditor.exe",
            "Engine/Build/BatchFiles/Build.bat",
            "Engine/Build/BatchFiles/RunUAT.bat",
            "Engine/Build/Build.version",
            "Test-SourceProvenance.ps1",
            "Build-PlayableCandidate.ps1",
            "-Platform Win64",
            "Test-CandidateManifest.ps1",
            "-Platform Win64",
            "Latebrake-candidate-manifest.json",
            "Latebrake.exe",
            "Get-FileHash",
            "UNVERIFIED",
        ):
            with self.subTest(required=required):
                self.assertIn(required, self.script)
        self.assertIn("$gameExecutables.Count -ne 1", self.script)
        self.assertIn("if ($Launch)", self.script)
        self.assertIn("Run-WindowsPlaytest.ps1", self.script)

    def test_guarded_runtime_session_has_accurate_evidence(self):
        runtime = (ROOT / "scripts/Run-WindowsPlaytest.ps1").read_text(encoding="utf-8")
        policy = (ROOT / "scripts/Test-WindowsRuntimePlaytestPolicy.ps1").read_text(encoding="utf-8")
        for marker in (
            "Test-CandidateManifest.ps1",
            "Test-UATPackageStructure.ps1",
            "-Platform Win64",
            "Latebrake-candidate-manifest.json",
            "Where-Object { $_.Name -ieq 'Latebrake.exe' }",
            "$executables.Count -ne 1",
            "Get-FileHash",
            "Start-Process -FilePath $game.FullName",
            "-PassThru -Wait",
            "-ABSLOG=",
            "unreal_log_present",
            "process_exit_code",
            "Verify-RuntimeStartupLog.ps1",
            "startup_log_markers_pass",
            "startup_log_error",
            "gameplay_verified = $false",
            "UNVERIFIED_PERSONAL_RUNTIME_PROBE",
            "Assert-SafeWindowsPlaytestLogDirectory.ps1",
            "ConvertTo-Json",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, runtime)
        self.assertIn("rejectedFake", policy)
        self.assertIn("rejectedMalformed", policy)
        # Post-run seal must be attempted AFTER the real process exits,
        # BEFORE writing the private receipt, and may not be silently ignored.
        launch = runtime.index("Start-Process -FilePath $game.FullName")
        postrun = runtime.index("Test-PostRunWin64CandidateIntegrity.ps1")
        receipt = runtime.index("$report = [ordered]@{")
        self.assertLess(launch, postrun)
        self.assertLess(postrun, receipt)
        self.assertIn("candidate_integrity_at_exit_pass", runtime)
        self.assertIn("candidate_integrity_at_exit_error", runtime)
        self.assertIn("if (-not $archiveIntegrityAtExitPass)", runtime)
        self.assertLess(receipt, runtime.index("if (-not $archiveIntegrityAtExitPass)"))
        helper = (ROOT / "scripts/Test-PostRunWin64CandidateIntegrity.ps1").read_text(encoding="utf-8")
        for required in ("Test-CandidateManifest.ps1",
                         "ExpectedManifestSha256", "ExpectedExecutableSha256",
                         "GetRelativePath", "Assert-OriginalIdentity",
                         "Candidate manifest changed after game launch",
                         "Packaged Latebrake.exe changed after game launch"):
            self.assertIn(required, helper)
        self.assertGreaterEqual(helper.count("Assert-OriginalIdentity"), 3)
        postrun_policy = (ROOT / "scripts/Test-PostRunWin64CandidateIntegrityPolicy.ps1").read_text(encoding="utf-8")
        for required in ("same-length executable mutation", "new runtime file inside sealed package",
                         "rewritten self-consistent manifest", "missing executable after test run",
                         "post-run linked candidate directory", "EXE outside selected candidate folder"):
            self.assertIn(required, postrun_policy)
        self.assertIn("Test-PostRunWin64CandidateIntegrityPolicy.ps1", self.ci)
        self.assertIn("Test-PostRunWin64CandidateIntegrity.ps1", self.ci)
        self.assertIn("Test-WindowsRuntimePlaytestPolicy.ps1", self.ci)
        self.assertIn("Run-WindowsPlaytest.ps1", self.ci)
        guard = (ROOT / "scripts/Assert-SafeWindowsPlaytestLogDirectory.ps1").read_text(encoding="utf-8")
        fixture = (ROOT / "scripts/Test-WindowsLogDirectoryPolicy.ps1").read_text(encoding="utf-8")
        for name in ("GetRelativePath", "IsPathRooted", "Assert-NoReparseAncestor",
                     "ReparsePoint", "GetParent", "New-Item", "LogDirectory must be outside"):
            self.assertIn(name, guard)
        self.assertIn("New-Item -ItemType Junction", fixture)
        self.assertIn("rejectedLinkedParent", fixture)
        self.assertIn("rejectedLinkedCandidate", fixture)
        self.assertIn("Test-WindowsLogDirectoryPolicy.ps1", self.ci)
        self.assertIn("Assert-SafeWindowsPlaytestLogDirectory.ps1", self.ci)

    def test_double_click_existing_candidate_launcher(self):
        cmd = (ROOT / "Play-Windows-Candidate.cmd").read_text(encoding="utf-8")
        self.assertIn("Run-WindowsPlaytest.ps1", cmd)
        self.assertIn('"%~1"', cmd)
        self.assertIn("where pwsh.exe", cmd)
        self.assertNotIn("gh release", cmd)

    def test_no_fake_executable_or_automatic_release(self):
        for forbidden in (
            "cmake --build", "gh release create", "upload-artifact",
            "WriteAllBytes", "Set-Content -Path $game",
            "Copy-Item -LiteralPath $project",
        ):
            self.assertNotIn(forbidden, self.script)
        self.assertIn("if (Test-Path -LiteralPath $candidate)", self.script)
        self.assertIn("out/Win64Playtests", self.script)
        self.assertIn("SourceProvenance", self.script)

    def test_double_click_launcher_uses_real_wrapper(self):
        self.assertIn("where pwsh.exe", self.launcher)
        self.assertIn('scripts\\Build-WindowsPlaytest.ps1', self.launcher)
        self.assertIn('%*', self.launcher)
        self.assertNotIn('Test/Standalone', self.launcher)

    def test_documentation_distinguishes_candidate_from_playable_release(self):
        for text in (
            "PowerShell 7", "Unreal Engine 5.8",
            "Build-Windows-Playtest.cmd",
            "Build-WindowsPlaytest.ps1",
            "UNVERIFIED",
            "out/Win64Playtests",
            "cooked",
            "#148",
        ):
            self.assertIn(text, self.docs)

    def test_ci_runs_synthetic_negative_preflight(self):
        self.assertIn("Build-WindowsPlaytest.ps1", self.ci)
        self.assertIn("Test-WindowsPlaytestWrapperPolicy.ps1", self.ci)


if __name__ == "__main__":
    unittest.main()
