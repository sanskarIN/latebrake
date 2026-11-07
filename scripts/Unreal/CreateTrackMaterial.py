"""Author an ORIGINAL parameterized PBR track material in real Unreal Editor 5.8.

Requires the licensed UnrealEditor.exe Python + Editor Scripting Utilities.
This is not runnable by plain CPython and does not manufacture a fake uasset.
Existing original material assets are never overwritten.
"""

import hashlib
import json
from pathlib import Path

import unreal

PACKAGE_DIR = "/Game/Latebrake/Materials"
ASSET_NAME = "M_LatebrakeTrackPBR"
ASSET_PATH = f"{PACKAGE_DIR}/{ASSET_NAME}"
MATERIAL_FILE = (
    Path(unreal.Paths.project_content_dir()) /
    "Latebrake" / "Materials" / f"{ASSET_NAME}.uasset"
)
RESULT_FILE = (
    Path(unreal.Paths.project_saved_dir()) /
    "LatebrakeAutomation" / "track-pbr-result.json"
)


def hash_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for part in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(part)
    return digest.hexdigest()


def create_material():
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        loaded = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
        if not isinstance(loaded, unreal.Material):
            raise RuntimeError("Existing track material has the wrong asset type.")
        return loaded, "reused"

    if not unreal.EditorAssetLibrary.does_directory_exist(PACKAGE_DIR):
        if not unreal.EditorAssetLibrary.make_directory(PACKAGE_DIR):
            raise RuntimeError("Could not create project-owned Materials directory.")

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = asset_tools.create_asset(
        ASSET_NAME, PACKAGE_DIR, unreal.Material, unreal.MaterialFactoryNew()
    )
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Unreal AssetTools did not create a Material.")

    # These exact parameters are consumed by CreateAndSetMaterialInstanceDynamic
    # in the actual C++ road and track-detail rendering paths.
    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -420, -220
    )
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -420, 40
    )
    specular = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -420, 240
    )
    if color is None or roughness is None or specular is None:
        raise RuntimeError("Material node creation failed in Unreal Editor.")

    color.set_editor_property("parameter_name", unreal.Name("Color"))
    color.set_editor_property(
        "default_value", unreal.LinearColor(0.17, 0.18, 0.19, 1.0)
    )
    roughness.set_editor_property("parameter_name", unreal.Name("Roughness"))
    roughness.set_editor_property("default_value", 0.88)
    specular.set_editor_property("parameter_name", unreal.Name("Specular"))
    specular.set_editor_property("default_value", 0.28)

    mapping = (
        (color, unreal.MaterialProperty.MP_BASE_COLOR),
        (roughness, unreal.MaterialProperty.MP_ROUGHNESS),
        (specular, unreal.MaterialProperty.MP_SPECULAR),
    )
    for node, material_property in mapping:
        if not unreal.MaterialEditingLibrary.connect_material_property(
            node, "", material_property
        ):
            raise RuntimeError(
                f"Failed to connect original material property {material_property}"
            )
    errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if errors:
        raise RuntimeError(f"Track material shader compile failed: {errors}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
        raise RuntimeError("Unreal Editor failed to save track PBR material.")
    return material, "created"


def main():
    version = str(unreal.SystemLibrary.get_engine_version())
    if not version.startswith("5.8."):
        raise RuntimeError(f"Expected licensed Unreal Editor 5.8, found {version}")
    # Never accept stale output when an Editor Python exception is swallowed.
    if RESULT_FILE.exists():
        RESULT_FILE.unlink()

    material, status = create_material()
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Unreal returned an invalid track material object.")
    if not MATERIAL_FILE.is_file() or MATERIAL_FILE.stat().st_size < 1024:
        raise RuntimeError(
            "No sufficiently large genuine serialized Unreal material asset: "
            + str(MATERIAL_FILE)
        )
    RESULT_FILE.parent.mkdir(parents=True, exist_ok=True)
    RESULT_FILE.write_text(
        json.dumps({
            "schema_version": 1,
            "status": "saved_by_unreal_editor",
            "engine_version": "5.8",
            "asset": ASSET_PATH,
            "operation": status,
            "bytes": MATERIAL_FILE.stat().st_size,
            "sha256": hash_file(MATERIAL_FILE),
        }, sort_keys=True),
        encoding="utf-8",
    )
    unreal.log(
        f"LateBrake: Unreal Editor {status} original track material at {ASSET_PATH}"
    )


main()
