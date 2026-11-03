"""Source-only contracts for the optional Win32 installer.

Real Windows setup self-tests run on Windows CI separately. These checks
do not install anything or certify a playable Unreal game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SETUP = (ROOT / "Experiments/Win32Preview/Setup.cpp").read_text(encoding="utf-8")
WORKFLOW = (ROOT / ".github/workflows/experimental-win32-preview.yml").read_text(
    encoding="utf-8"
)


class Win32InstallerStagingBindingTests(unittest.TestCase):
    def test_uninstall_flags_are_exact_arguments_only(self):
        self.assertIn("hasExactArgument(cmd,\"--final-remove\")", SETUP)
        self.assertIn("hasExactArgument(cmd,\"--uninstall\")", SETUP)
        self.assertIn("hasExactArgument(cmd,\"--self-test\")", SETUP)
        self.assertIn("else if(hasNoArgument(cmd))install();", SETUP)
        self.assertNotIn('if(has(cmd,"--final-remove"))', SETUP)
        self.assertNotIn('if(has(cmd,"--uninstall"))', SETUP)
        self.assertIn("installerSelfTest()", SETUP)
        self.assertIn('!hasExactArgument(quoted,"--uninstall")', SETUP)
        self.assertIn('!hasExactArgument(quoted,"--final-remove")', SETUP)
        self.assertIn('!hasExactArgument("LateBrakeSetupV5.exe --uninstall-now"', SETUP)
        self.assertIn("Unknown setup argument.", SETUP)

    def test_installer_stages_both_files_and_never_truncates_installed_game(self):
        self.assertIn("CreateFileA(name,0x40000000u,0,0,1,0x80,0)", SETUP)
        self.assertIn("FlushFileBuffers(h)", SETUP)
        self.assertIn("GetFileAttributesA(folder)", SETUP)
        self.assertIn("folderFlags&0x400u", SETUP)
        self.assertIn('join(gameStage,570,folder,"LateBrakePreviewV5.exe.new")', SETUP)
        self.assertIn('join(setupStage,570,folder,"LateBrakeSetupV5.exe.new")', SETUP)
        self.assertIn("writeData(gameStage,embeddedGame,", SETUP)
        self.assertIn("CopyFileA(self,setupStage,1)", SETUP)
        self.assertIn("MoveFileExA(setupStage,setup,0x1u|0x8u)", SETUP)
        self.assertIn("MoveFileExA(gameStage,game,0x1u|0x8u)", SETUP)
        self.assertNotIn("writeData(game,embeddedGame,", SETUP)
        self.assertLess(SETUP.index("writeData(gameStage,embeddedGame,"),
                        SETUP.index("CopyFileA(self,setupStage,1)"))
        self.assertLess(SETUP.index("CopyFileA(self,setupStage,1)"),
                        SETUP.index("MoveFileExA(setupStage,setup,0x1u|0x8u)"))
        self.assertLess(SETUP.index("MoveFileExA(setupStage,setup,0x1u|0x8u)"),
                        SETUP.index("MoveFileExA(gameStage,game,0x1u|0x8u)"))
        self.assertLess(SETUP.index("MoveFileExA(gameStage,game,0x1u|0x8u)"),
                        SETUP.index("registration(folder,setup);"))

    def test_installer_version_matches_windows_v5_preview(self):
        self.assertIn('regString(k,"DisplayVersion","5.0.0-preview")', SETUP)
        self.assertNotIn('regString(k,"DisplayVersion","4.0.0-preview")', SETUP)

    def test_actual_windows_ci_runs_selftest_without_user_profile_writes(self):
        self.assertIn("Run synthetic C++ navigation/progression smoke test", WORKFLOW)
        self.assertIn("Test setup argument parsing without installing anything", WORKFLOW)
        self.assertIn("-ArgumentList '--self-test' -Wait -PassThru", WORKFLOW)
        self.assertIn("setup-parser-test", WORKFLOW)
        self.assertIn("A setup self-test wrote to player storage.", WORKFLOW)
        self.assertIn("retention-days: 14", WORKFLOW)
        self.assertIn("UNVERIFIED-LateBrake-V5-2D-Win64-", WORKFLOW)


if __name__ == "__main__":
    unittest.main()
