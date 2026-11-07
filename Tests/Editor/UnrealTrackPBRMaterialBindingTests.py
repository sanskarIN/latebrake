"""Source-only checks for project-owned material authoring contracts."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealTrackPBRMaterialBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.script = read("scripts/Unreal/CreateTrackMaterial.py")
        cls.wrapper = read("scripts/New-TrackMaterial.ps1")
        cls.game = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.engine = read("Config/DefaultEngine.ini")

    def test_genuine_editor_asset_and_no_fake_uasset(self):
        for expected in (
            "import unreal",
            "unreal.AssetToolsHelpers.get_asset_tools()",
            "unreal.MaterialFactoryNew()",
            "unreal.MaterialExpressionVectorParameter",
            "unreal.MaterialExpressionScalarParameter",
            "unreal.MaterialProperty.MP_BASE_COLOR",
            "unreal.MaterialProperty.MP_ROUGHNESS",
            "unreal.MaterialEditingLibrary.recompile_material(material)",
            "unreal.EditorAssetLibrary.save_loaded_asset(material, False)",
            "version.startswith(\"5.8.\")",
            "MATERIAL_FILE.is_file()",
            "hash_file(MATERIAL_FILE)",
            "RESULT_FILE.write_text(",
        ):
            self.assertIn(expected, self.script)
        self.assertNotIn("MATERIAL_FILE.write_bytes(", self.script)
        self.assertNotIn("shutil.", self.script)

    def test_runtime_cooks_same_canonical_material_path(self):
        path = "/Game/Latebrake/Materials/M_LatebrakeTrackPBR.M_LatebrakeTrackPBR"
        self.assertGreaterEqual(self.game.count(path), 3)
        self.assertIn("Instances->SetMaterial(0, ProjectTrackMaterial);", self.game)
        self.assertIn("Mesh->SetMaterial(0, ProjectTrackMaterial);", self.game)
        self.assertIn("Marks->SetMaterial(0, ProjectTrackMaterial);", self.game)
        self.assertIn(
            '+DirectoriesToAlwaysCook=(Path="/Game/Latebrake/Materials")',
            self.engine,
        )
        self.assertIn("CreateAndSetMaterialInstanceDynamic(0)", self.game)

    def test_realistic_preview_material_source_is_immutable(self):
        pipeline = read("scripts/Build-PlayableCandidate.ps1")
        uat = read("scripts/Build-PreRelease.ps1")
        manifest = read("scripts/Write-CandidateManifest.ps1")
        checker = read("scripts/Test-CandidateManifest.ps1")
        policy = read("scripts/Test-TrackMaterialSource.ps1")
        fixture = read("scripts/Test-TrackMaterialSourcePolicy.ps1")
        workflow = read(".github/workflows/core-tests.yml")
        for token in ("RequireAuthoredTrackMaterial", "trackMaterialSha256",
                      "ExpectedTrackMaterialSha256", "Test-TrackMaterialSource.ps1"):
            self.assertIn(token, pipeline)
        self.assertIn("ExpectedTrackMaterialSha256", uat)
        self.assertIn("Test-TrackMaterialSource.ps1", uat)
        self.assertIn("authored_track_material_sha256", manifest)
        self.assertIn("authored_track_material_sha256", checker)
        for token in ("ls-files --error-unmatch", "git -C $root status",
                      "Get-FileHash", "ExpectedSha256", "ReparsePoint"):
            self.assertIn(token, policy)
        self.assertIn("untracked material", fixture)
        self.assertIn("unhydrated Git LFS pointer", fixture)
        self.assertIn("Test-TrackMaterialSourcePolicy.ps1", workflow)

    def test_wrapper_rejects_stale_or_forged_material_receipt(self):
        for s in (
            'Remove-Item -LiteralPath $receipt -Force',
            '-ExecutePythonScript=',
            '$proof.sha256 -cne $hash',
            'Get-FileHash -LiteralPath $asset -Algorithm SHA256',
            "$projectVersion.EngineAssociation -cne '5.8'",
        ):
            self.assertIn(s, self.wrapper)


if __name__ == "__main__":
    unittest.main()
