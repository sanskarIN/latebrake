"""Mocked Unreal Editor material creation tests; NEVER real engine or asset evidence."""
import importlib.util
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[2] / "scripts/Unreal/AuthorRacingLineMaterial.py"


class FakeMaterial:
    def __init__(self):
        self.parameters = set()
        self.usages = set()


class FakeVectorParameter:
    def __init__(self, material):
        self.material = material
        self.properties = {}

    def set_editor_property(self, name, value):
        self.properties[name] = value
        if name == "parameter_name":
            self.material.parameters.add(value)


class FakeUnrealEditor:
    """A tiny explicit API stub. Written bytes are NOT a valid Unreal asset."""

    def __init__(self, directory):
        self.asset_path = Path(directory) / "Materials/M_RacingLineGuide.uasset"
        self.receipt_path = Path(directory) / "Saved/LatebrakeAutomation/racing-line-material-result.json"
        self.existing = None
        self.version = "5.8.0-TEST-STUB"
        self.editor_available = True
        self.factory_returns_none = False
        self.node_returns_none = False
        self.connect_success = True
        self.compile_errors = []
        self.save_success = True
        self.created_count = 0
        self.saved_count = 0
        self.created_material = None
        self.created_node = None
        self.last_connected_property = None
        self.last_usage = None
        self.logs = []

    def module(self):
        fake = types.ModuleType("unreal")
        fake.Material = FakeMaterial
        fake.MaterialUsage = types.SimpleNamespace(
            MATUSAGE_INSTANCED_STATIC_MESHES="ISM"
        )
        fake.MaterialProperty = types.SimpleNamespace(MP_BASE_COLOR="base_color")
        fake.MaterialExpressionVectorParameter = FakeVectorParameter
        fake.MaterialFactoryNew = lambda: object()
        fake.LinearColor = lambda *parts: tuple(parts)
        fake.Paths = types.SimpleNamespace(
            project_content_dir=lambda: str(self.asset_path.parent.parent),
            project_saved_dir=lambda: str(self.receipt_path.parent.parent),
        )
        fake.SystemLibrary = types.SimpleNamespace(
            get_engine_version=lambda: self.version
        )
        fake.EditorAssetSubsystem = type("EditorAssetSubsystem", (), {})
        fake.get_editor_subsystem = (
            lambda cls: self if self.editor_available else None
        )
        fake.EditorAssetLibrary = types.SimpleNamespace(
            make_directory=lambda name: None
        )
        fake.AssetToolsHelpers = types.SimpleNamespace(
            get_asset_tools=lambda: self
        )
        fake.MaterialEditingLibrary = types.SimpleNamespace(
            create_material_expression=self.create_material_expression,
            connect_material_property=self.connect_material_property,
            set_base_material_usage=self.set_base_material_usage,
            recompile_material=lambda material: list(self.compile_errors),
            get_vector_parameter_names=lambda material: list(material.parameters),
            has_material_usage=lambda material, usage: usage in material.usages,
        )
        fake.log = self.logs.append
        return fake

    def does_asset_exist(self, path):
        return self.existing is not None

    def load_asset(self, path):
        return self.existing

    def create_asset(self, name, directory, material_type, factory):
        self.created_count += 1
        if self.factory_returns_none:
            return None
        self.created_material = material_type()
        return self.created_material

    def create_material_expression(self, material, expression_type, x, y):
        if self.node_returns_none:
            return None
        self.created_node = FakeVectorParameter(material)
        return self.created_node

    def connect_material_property(self, node, output, target):
        self.last_connected_property = target
        return self.connect_success

    def set_base_material_usage(self, material, usage, enabled):
        self.last_usage = usage
        if enabled:
            material.usages.add(usage)

    def save_loaded_asset(self, material, only_if_dirty):
        self.saved_count += 1
        if not self.save_success:
            return False
        self.asset_path.parent.mkdir(parents=True, exist_ok=True)
        self.asset_path.write_bytes(b"SYNTHETIC_TEST_ONLY" * 100)
        return True


def execute_with_stub(editor):
    with patch.dict(sys.modules, {"unreal": editor.module()}):
        spec = importlib.util.spec_from_file_location(
            "latebrake_material_stub_test", SCRIPT
        )
        script = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(script)


class UnrealRacingLineMaterialAuthoringTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="latebrake-material-stub-")
        self.addCleanup(self.temp.cleanup)
        self.editor = FakeUnrealEditor(self.temp.name)

    def test_new_graph_parameter_usage_compile_and_save(self):
        execute_with_stub(self.editor)
        self.assertEqual((self.editor.created_count, self.editor.saved_count), (1, 1))
        self.assertEqual(self.editor.last_usage, "ISM")
        self.assertEqual(self.editor.last_connected_property, "base_color")
        self.assertEqual(self.editor.created_node.properties["parameter_name"], "Color")
        self.assertEqual(self.editor.created_node.properties["default_value"],
                         (1.0, 1.0, 1.0, 1.0))
        self.assertTrue(self.editor.asset_path.is_file())
        import json
        receipt = json.loads(self.editor.receipt_path.read_text(encoding="utf-8"))
        self.assertEqual(receipt["schema_version"], 1)
        self.assertEqual(receipt["status"], "saved_by_unreal_editor")
        self.assertEqual(receipt["asset"], "/Game/Materials/M_RacingLineGuide")
        self.assertEqual(receipt["operation"], "created")
        self.assertEqual(receipt["color_parameter"], "Color")
        self.assertIs(receipt["instanced_mesh_usage"], True)
        self.assertEqual(receipt["bytes"], self.editor.asset_path.stat().st_size)
        self.assertEqual(len(receipt["sha256"]), 64)
        self.assertTrue(any("status=created" in msg for msg in self.editor.logs))
        self.assertTrue(any("asset_sha256=" in msg for msg in self.editor.logs))

    def test_existing_valid_material_is_preserved_untouched(self):
        material = FakeMaterial()
        material.parameters.add("Color")
        material.usages.add("ISM")
        self.editor.existing = material
        self.editor.asset_path.parent.mkdir(parents=True)
        original = b"SYNTHETIC_EXISTING_ASSET" * 100
        self.editor.asset_path.write_bytes(original)
        execute_with_stub(self.editor)
        self.assertEqual(self.editor.created_count, 0)
        self.assertEqual(self.editor.saved_count, 0)
        self.assertEqual(self.editor.asset_path.read_bytes(), original)
        import json
        receipt = json.loads(self.editor.receipt_path.read_text(encoding="utf-8"))
        self.assertEqual(receipt["operation"], "validated_existing")
        self.assertTrue(any("status=validated_existing" in msg
                            for msg in self.editor.logs))

    def test_bad_engine_version_and_missing_editor_fail_closed(self):
        self.editor.version = "5.7.9-TEST-STUB"
        with self.assertRaisesRegex(RuntimeError, "requires genuine UE 5.8"):
            execute_with_stub(self.editor)
        self.editor.version = "5.8.0-TEST-STUB"
        self.editor.editor_available = False
        with self.assertRaisesRegex(RuntimeError, "EditorAssetSubsystem is unavailable"):
            execute_with_stub(self.editor)
        self.assertEqual(self.editor.created_count, 0)

    def test_incompatible_existing_assets_are_rejected_without_rewriting(self):
        for material in (object(), FakeMaterial()):
            with self.subTest(material_type=type(material)):
                self.editor.existing = material
                with self.assertRaises(RuntimeError):
                    execute_with_stub(self.editor)
                self.assertEqual(self.editor.saved_count, 0)
        missing_usage = FakeMaterial()
        missing_usage.parameters.add("Color")
        self.editor.existing = missing_usage
        with self.assertRaisesRegex(RuntimeError, "Instanced Static Meshes"):
            execute_with_stub(self.editor)
        self.assertEqual(self.editor.saved_count, 0)

    def test_creation_graph_compile_and_save_errors_never_report_success(self):
        scenarios = (
            ("factory_returns_none", True, "could not create"),
            ("node_returns_none", True, "could not add"),
            ("connect_success", False, "Cannot connect"),
            ("compile_errors", ["synthetic compiler error"], "compile errors"),
            ("save_success", False, "could not save"),
        )
        for name, value, reason in scenarios:
            with self.subTest(scenario=name):
                editor = FakeUnrealEditor(self.temp.name)
                setattr(editor, name, value)
                with self.assertRaisesRegex(RuntimeError, reason):
                    execute_with_stub(editor)
                self.assertFalse(any("status=created" in msg for msg in editor.logs))
                self.assertFalse(editor.receipt_path.exists())

    def test_missing_or_tiny_existing_file_is_not_authoring_success(self):
        material = FakeMaterial()
        material.parameters.add("Color")
        material.usages.add("ISM")
        self.editor.existing = material
        with self.assertRaisesRegex(RuntimeError, "did not save a plausible"):
            execute_with_stub(self.editor)
        self.editor.asset_path.parent.mkdir(parents=True)
        self.editor.asset_path.write_bytes(b"TOO_SMALL")
        with self.assertRaisesRegex(RuntimeError, "did not save a plausible"):
            execute_with_stub(self.editor)
        self.assertEqual(self.editor.saved_count, 0)
        self.assertFalse(self.editor.receipt_path.exists())


if __name__ == "__main__":
    unittest.main()
