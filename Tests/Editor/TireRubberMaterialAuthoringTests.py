"""Synthetic Editor API decisions; no actual material is authored here."""
from pathlib import Path
import runpy
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
with patch.dict(sys.modules, {'unreal': SimpleNamespace()}):
    create_material = runpy.run_path(str(ROOT/'scripts/Unreal/CreateTireRubberMaterial.py'))['create_material']


class Material:
    pass


class Node:
    def __init__(self):
        self.properties = {}

    def set_editor_property(self, name, value):
        self.properties[name] = value


def api_fixture(existing=None, version='5.8.0', errors=None):
    state = {'created': 0, 'saved': 0, 'nodes': [], 'connections': [], 'usage': []}

    def create(*args):
        state['created'] += 1
        return Material()

    def node(*args):
        value = Node(); state['nodes'].append(value)
        return value

    def connect(value, output, prop):
        state['connections'].append(prop)
        return True

    def save(*args):
        state['saved'] += 1
        return True

    api = SimpleNamespace(
        SystemLibrary=SimpleNamespace(get_engine_version=lambda: version),
        EditorAssetLibrary=SimpleNamespace(does_asset_exist=lambda p: existing is not None,
                                          load_asset=lambda p: existing, save_loaded_asset=save),
        AssetToolsHelpers=SimpleNamespace(get_asset_tools=lambda: SimpleNamespace(create_asset=create)),
        Material=Material, MaterialInterface=Material, MaterialFactoryNew=object,
        MaterialExpressionConstant3Vector=Node, MaterialExpressionConstant=Node,
        LinearColor=lambda *rgba: rgba,
        MaterialProperty=SimpleNamespace(MP_BASE_COLOR='base', MP_ROUGHNESS='rough', MP_SPECULAR='spec'),
        MaterialUsage=SimpleNamespace(MATUSAGE_INSTANCED_STATIC_MESHES='ism'),
        MaterialEditingLibrary=SimpleNamespace(create_material_expression=node,
            connect_material_property=connect, recompile_material=lambda material: errors or [],
            set_base_material_usage=lambda *args: state['usage'].append(args)),
        log=lambda message: None,
    )
    return api, state


class TireRubberMaterialAuthoringTests(unittest.TestCase):
    def test_original_material_is_authored_connected_and_saved_through_editor_api(self):
        api, state = api_fixture()
        result = create_material(api)
        self.assertIsInstance(result, Material)
        self.assertEqual((state['created'], state['saved']), (1, 1))
        self.assertEqual(state['connections'], ['base', 'rough', 'spec'])
        self.assertEqual(state['usage'][0][1:], ('ism', True))
        self.assertEqual(state['nodes'][1].properties['r'], 0.93)

    def test_existing_user_material_is_preserved_without_graph_or_file_mutation(self):
        original = Material(); api, state = api_fixture(existing=original)
        self.assertIs(create_material(api), original)
        self.assertEqual((state['created'], state['saved'], len(state['nodes'])), (0, 0, 0))

    def test_compiler_errors_cannot_be_reported_as_successfully_saved_assets(self):
        api, state = api_fixture(errors=['synthetic shader failure'])
        with self.assertRaisesRegex(RuntimeError, 'compilation failed'):
            create_material(api)
        self.assertEqual(state['saved'], 0)

    def test_wrong_engine_and_existing_wrong_asset_type_fail_before_creation(self):
        for existing, version in ((None, '5.7.4'), (object(), '5.8.0')):
            api, state = api_fixture(existing=existing, version=version)
            with self.assertRaises(RuntimeError):
                create_material(api)
            self.assertEqual((state['created'], state['saved']), (0, 0))

    def test_failed_editor_connection_does_not_save_a_partial_graph(self):
        api, state = api_fixture()
        api.MaterialEditingLibrary.connect_material_property = lambda *args: False
        with self.assertRaisesRegex(RuntimeError, 'connect'):
            create_material(api)
        self.assertEqual(state['saved'], 0)


if __name__ == '__main__':
    unittest.main()
