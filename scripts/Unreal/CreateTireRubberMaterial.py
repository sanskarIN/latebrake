"""Author the original optional skid-strip material in licensed UE5.8 Editor.

This script never writes synthetic .uasset bytes or certifies a cook/game.
It preserves an existing asset; inspect that asset in the real Editor.
"""
import unreal

PACKAGE = '/Game/Materials'
NAME = 'M_LatebrakeTireRubber'
PATH = PACKAGE + '/' + NAME


def create_material(api):
    version = str(api.SystemLibrary.get_engine_version())
    if not version.startswith('5.8.'):
        raise RuntimeError('Tire-rubber authoring requires licensed Unreal 5.8 Editor')
    if api.EditorAssetLibrary.does_asset_exist(PATH):
        existing = api.EditorAssetLibrary.load_asset(PATH)
        if not isinstance(existing, api.MaterialInterface):
            raise RuntimeError('Existing skid material path has an unexpected asset type')
        api.log('LateBrake: preserving existing optional rubber material at '+PATH)
        return existing
    material = api.AssetToolsHelpers.get_asset_tools().create_asset(
        NAME, PACKAGE, api.Material, api.MaterialFactoryNew())
    if material is None:
        raise RuntimeError('Unreal did not create the optional tire-rubber material')
    graph = api.MaterialEditingLibrary
    graph.set_base_material_usage(material, api.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, True)
    color = graph.create_material_expression(material, api.MaterialExpressionConstant3Vector, -400, -120)
    roughness = graph.create_material_expression(material, api.MaterialExpressionConstant, -400, 80)
    specular = graph.create_material_expression(material, api.MaterialExpressionConstant, -400, 240)
    if any(node is None for node in (color, roughness, specular)):
        raise RuntimeError('Unreal failed to create rubber material graph nodes')
    color.set_editor_property('constant', api.LinearColor(0.012, 0.011, 0.010, 1.0))
    roughness.set_editor_property('r', 0.93)
    specular.set_editor_property('r', 0.05)
    for node, prop in ((color, api.MaterialProperty.MP_BASE_COLOR),
                       (roughness, api.MaterialProperty.MP_ROUGHNESS),
                       (specular, api.MaterialProperty.MP_SPECULAR)):
        if not graph.connect_material_property(node, '', prop):
            raise RuntimeError('Unreal failed to connect a rubber material property')
    errors = graph.recompile_material(material)
    if errors:
        raise RuntimeError('Rubber material compilation failed: '+'; '.join(str(e) for e in errors))
    if not api.EditorAssetLibrary.save_loaded_asset(material, False):
        raise RuntimeError('Unreal failed to save the rubber material asset')
    api.log('LateBrake: original optional tire-rubber material authored by Unreal at '+PATH)
    return material


if __name__ == '__main__':
    create_material(unreal)
