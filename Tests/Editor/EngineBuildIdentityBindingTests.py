"""Source-only UE Build.version identity wiring, NOT actual Unreal execution."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2] / "scripts"


class EngineBuildIdentityBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.wrapper = (ROOT / "Build-PlayableCandidate.ps1").read_text()
        cls.cook = (ROOT / "Build-PreRelease.ps1").read_text()
        cls.manifest = (ROOT / "Write-CandidateManifest.ps1").read_text()
        cls.check = (ROOT / "Test-CandidateManifest.ps1").read_text()
        cls.pair = (ROOT / "Test-CandidatePairIdentity.ps1").read_text()
        cls.snapshot = (ROOT / "Test-EngineBuildSnapshot.ps1").read_text()
        cls.ci = (ROOT.parent / ".github/workflows/core-tests.yml").read_text()

    def test_wrapper_pins_one_engine_snapshot_across_all_stages(self):
        self.assertIn("Get-FileHash -LiteralPath $engineVersionPath -Algorithm SHA256", self.wrapper)
        self.assertGreaterEqual(self.wrapper.count("Test-EngineBuildSnapshot.ps1"), 2)
        self.assertEqual(
            self.wrapper.count("-ExpectedEngineSha256 $engineBuildSha256"), 2,
            "both UAT and manifest writer must receive the same pinned engine digest",
        )
        self.assertLess(self.wrapper.index("$engineBuildSha256 = "),
                        self.wrapper.index("Build-Editor.ps1"))
        self.assertLess(self.wrapper.index("Build-Editor.ps1"),
                        self.wrapper.index("New-StarterMap.ps1"))
        self.assertLess(self.wrapper.index("New-StarterMap.ps1"),
                        self.wrapper.index("Build-PreRelease.ps1"))

    def test_direct_uat_checks_engine_pre_and_post(self):
        self.assertIn("[string]$ExpectedEngineSha256", self.cook)
        self.assertIn("$engineBeforeCook =", self.cook)
        self.assertGreaterEqual(self.cook.count("Test-EngineBuildSnapshot.ps1"), 2)
        before = self.cook.index("Test-EngineBuildSnapshot.ps1")
        uat = self.cook.index("& $uat @uatArguments")
        after = self.cook.rindex("Test-EngineBuildSnapshot.ps1")
        self.assertLess(before, uat)
        self.assertLess(uat, after)

    def test_manifest_records_verified_exact_build_identity(self):
        self.assertIn("schema_version = 2", self.manifest)
        self.assertIn("unreal_engine_build_sha256 = $engineBuildSha256", self.manifest)
        self.assertIn("[string]$ExpectedEngineSha256", self.manifest)
        self.assertIn("Test-EngineBuildSnapshot.ps1", self.manifest)
        self.assertIn("$manifest.schema_version -ne 2", self.check)
        self.assertIn("$manifest.unreal_engine_build_sha256 -cnotmatch", self.check)

    def test_pair_requires_same_exact_engine_and_map(self):
        self.assertIn("$info.schema_version -ne 2", self.pair)
        self.assertIn("$info.unreal_engine_build_sha256 -cnotmatch", self.pair)
        self.assertIn(
            "$windows.unreal_engine_build_sha256 -cne $android.unreal_engine_build_sha256",
            self.pair,
        )
        self.assertIn(
            "$windows.starter_map_sha256 -ine $android.starter_map_sha256",
            self.pair,
        )

    def test_synthetic_windows_policy_is_registered(self):
        self.assertIn("Test-EngineBuildSnapshotPolicy.ps1", self.ci)
        self.assertIn("Test-EngineBuildSnapshot.ps1", self.snapshot)
        self.assertIn("Build.version changed", self.snapshot)

    def test_engine_gate_cannot_be_replaced_with_version_string(self):
        broken = self.pair.replace(
            "$windows.unreal_engine_build_sha256 -cne $android.unreal_engine_build_sha256",
            "$windows.unreal_engine_version -cne $android.unreal_engine_version",
        )
        self.assertNotIn(
            "$windows.unreal_engine_build_sha256 -cne $android.unreal_engine_build_sha256",
            broken,
        )


if __name__ == "__main__":
    unittest.main()
