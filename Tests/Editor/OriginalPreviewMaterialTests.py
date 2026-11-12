"""Source-only contracts; true .uasset creation requires installed UE5.8."""
import ast
import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[2]
GENERATE=ROOT/"scripts/Unreal/CreatePreviewMaterials.py"
SOURCE=ROOT/"Source/Latebrake/Private/Visual/OriginalPreviewMaterials.cpp"

class OriginalPreviewMaterialTests(unittest.TestCase):
    def test_editor_script_only_creates_real_unreal_assets(self):
        source=GENERATE.read_text(encoding="utf-8")
        ast.parse(source,filename=str(GENERATE))
        self.assertIn('import unreal',source)
        self.assertIn('ENGINE_PREFIX = "5.8."',source)
        self.assertIn('tools.create_asset(',source)
        self.assertIn('unreal.MaterialFactoryNew()',source)
        self.assertIn('unreal.MaterialExpressionVertexColor',source)
        self.assertIn('unreal.MaterialProperty.MP_BASE_COLOR',source)
        self.assertIn('MaterialEditingLibrary.recompile_material(',source)
        self.assertIn('subsystem.save_loaded_asset(material, True)',source)
        self.assertIn('sha256_file(destination)',source)
        self.assertIn('existing_unmodified_requires_visual_review',source)
        self.assertIn('native_gameplay_validated": False',source)
        self.assertIn('installed_cooked_game_validated": False',source)
    def test_native_material_lookup_is_opt_in_preview_only(self):
        code=SOURCE.read_text(encoding="utf-8")
        self.assertIn('FindOriginalPreviewMaterial(',code)
        self.assertIn('LoadObject<UMaterialInterface>',code)
        for name in ("Road","Steel","CarPaint","Glass","Rubber","Emission"):
            self.assertIn('M_Latebrake'+name+'Pbr.',code)
        self.assertNotIn("SetSimulatePhysics",code)
        self.assertNotIn("SetCollisionEnabled",code)
        self.assertNotIn("SetPhysicsLinearVelocity",code)

if __name__=="__main__":
    unittest.main()
