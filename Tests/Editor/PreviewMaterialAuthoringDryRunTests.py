"""Adversarial dry-run of the Unreal Editor PBR asset authoring control flow.

All objects here are fake. No simulation is permitted to claim binary
Unreal compatibility; only a licensed UE5.8 run can prove a real .uasset.
"""
import json
import runpy
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/Unreal/CreatePreviewMaterials.py"

class FakeMaterial:
    pass

class FakeExpression:
    def __init__(self):
        self.properties={}
    def set_editor_property(self,name,value):
        self.properties[name]=value

class MaterialAuthoringDryRunTests(unittest.TestCase):
    def simulate(self,version="5.8.3",existing=(),compile_errors=(),
                 save_ok=True):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            saved=root/"Saved"
            content=root/"Content"
            folder=content/"Latebrake"/"Materials"
            folder.mkdir(parents=True)
            fake=types.ModuleType("unreal")
            fake.Paths=types.SimpleNamespace(
                project_saved_dir=lambda: str(saved),
                project_content_dir=lambda: str(content))
            fake.SystemLibrary=types.SimpleNamespace(
                get_engine_version=lambda:version)
            fake.Material=FakeMaterial
            fake.MaterialFactoryNew=lambda: object()
            fake.MaterialExpressionVertexColor=type(
                "MaterialExpressionVertexColor",(),{})
            fake.MaterialExpressionConstant=type(
                "MaterialExpressionConstant",(),{})
            fake.MaterialExpressionMultiply=type(
                "MaterialExpressionMultiply",(),{})
            fake.MaterialProperty=types.SimpleNamespace(
                MP_BASE_COLOR=1,MP_ROUGHNESS=2,
                MP_METALLIC=3,MP_EMISSIVE_COLOR=4)
            calls={"create":[],"connect":[],"recompile":[],"save":[]}
            class Tools:
                def create_asset(self,name,directory,cls,factory):
                    calls["create"].append((name,directory))
                    (folder/(name+".uasset")).write_bytes(
                        ("NEW-FAKE-"+name).encode()*64)
                    return FakeMaterial()
            class AssetSubsystem:
                def load_asset(self,package):
                    return FakeMaterial()
                def save_loaded_asset(self,material,only_if_dirty):
                    calls["save"].append(only_if_dirty)
                    return save_ok
            fake.EditorAssetSubsystem=type("EditorAssetSubsystem",(),{})
            fake.get_editor_subsystem=lambda cls:AssetSubsystem()
            fake.AssetToolsHelpers=types.SimpleNamespace(
                get_asset_tools=lambda:Tools())
            fake.EditorAssetLibrary=types.SimpleNamespace(
                does_asset_exist=lambda package:
                    package.rsplit("/",1)[-1] in existing)
            fake.MaterialEditingLibrary=types.SimpleNamespace(
                create_material_expression=lambda mat,cls,x,y:
                    FakeExpression(),
                connect_material_property=lambda expr,output,property_:
                    calls["connect"].append((output,property_)) or True,
                connect_material_expressions=lambda *args:True,
                recompile_material=lambda material:
                    calls["recompile"].append(material) or list(compile_errors))
            fake.log=lambda message:None
            for name in existing:
                (folder/(name+".uasset")).write_bytes(
                    ("KEEP-FAKE-"+name).encode()*64)
            caught=None
            with patch.dict(sys.modules,{"unreal":fake}):
                try:
                    runpy.run_path(str(SCRIPT),run_name="__main__")
                except RuntimeError as error:
                    caught=str(error)
            receipt_file=saved/"LatebrakeAutomation"/"preview-materials-result.json"
            receipt=json.loads(receipt_file.read_text(encoding="utf-8")) if (
                receipt_file.is_file()) else None
            kept={
                name:(folder/(name+".uasset")).read_bytes()
                for name in existing}
            return receipt,calls,caught,kept

    def test_all_six_are_created_and_compiled_when_new(self):
        receipt,calls,error,_=self.simulate()
        self.assertIsNone(error)
        self.assertEqual(len(calls["create"]),6)
        self.assertEqual(len(calls["recompile"]),6)
        self.assertEqual(len(calls["save"]),6)
        self.assertIsNotNone(receipt)
        self.assertTrue(receipt["all_newly_compiled"])
        self.assertFalse(receipt["native_gameplay_validated"])
        self.assertFalse(receipt["installed_cooked_game_validated"])
        self.assertEqual(len(receipt["materials"]),6)
        self.assertTrue(all(
            asset["operation"]=="created_and_compiled_in_unreal_editor"
            for asset in receipt["materials"]))
        self.assertTrue(all(
            asset["sha256"] and asset["bytes"]>=256
            for asset in receipt["materials"]))
        self.assertEqual(
            sum(1 for item in calls["connect"] if item[1]==1),6)

    def test_existing_material_never_edited_or_overwritten(self):
        name="M_LatebrakeRoadPbr"
        receipt,calls,error,existing=self.simulate(existing=(name,))
        self.assertIsNone(error)
        self.assertEqual(len(calls["create"]),5)
        self.assertEqual(len(calls["recompile"]),5)
        self.assertEqual(len(calls["save"]),5)
        self.assertFalse(receipt["all_newly_compiled"])
        self.assertEqual(
            receipt["materials"][0]["operation"],
            "existing_unmodified_requires_visual_review")
        self.assertTrue(existing[name].startswith(b"KEEP-FAKE-"))

    def test_wrong_engine_version_rejects_without_asset_writes(self):
        receipt,calls,error,_=self.simulate(version="5.7.7")
        self.assertIsNone(receipt)
        self.assertEqual(calls["create"],[])
        self.assertIn("Unreal Editor 5.8 required",error)

    def test_material_compile_failure_yields_no_success_receipt(self):
        receipt,calls,error,_=self.simulate(
            compile_errors=("shader compilation failure",))
        self.assertIsNone(receipt)
        self.assertEqual(len(calls["create"]),1)
        self.assertIn("compilation errors",error)

    def test_editor_save_failure_never_writes_receipt(self):
        receipt,calls,error,_=self.simulate(save_ok=False)
        self.assertIsNone(receipt)
        self.assertEqual(len(calls["create"]),1)
        self.assertIn("could not save Material",error)

if __name__=="__main__":
    unittest.main()
