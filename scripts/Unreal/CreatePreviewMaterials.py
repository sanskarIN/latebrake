"""Author real editor-compiled vertex-color PBR materials for LateBrake.

Run ONLY using installed Unreal 5.8 Editor's Python plugin. This script does
not synthesize .uasset binaries, does not touch race physics, and preserves all
existing material assets without modifying them. Native procedural visual
bridges use these assets when available; the stock engine material fallback
does NOT prove that custom geometry has displayed its intended colors.
"""
import hashlib
import json
from pathlib import Path

import unreal

ENGINE_PREFIX = "5.8."
MATERIAL_FOLDER = "/Game/Latebrake/Materials"
SAVED_FOLDER = Path(unreal.Paths.project_saved_dir()) / "LatebrakeAutomation"
RESULT_PATH = SAVED_FOLDER / "preview-materials-result.json"
CONTENT = Path(unreal.Paths.project_content_dir()) / "Latebrake" / "Materials"
# Roughness, metallic, emissive intensity: physically-inspired presentation
# only, not game tire grip / Chaos road friction / vehicle engine data.
PROFILES = {
    "M_LatebrakeRoadPbr": (0.92, 0.00, 0.0),
    "M_LatebrakeSteelPbr": (0.43, 0.82, 0.0),
    "M_LatebrakeCarPaintPbr": (0.29, 0.45, 0.0),
    "M_LatebrakeGlassPbr": (0.13, 0.00, 0.0),
    "M_LatebrakeRubberPbr": (0.88, 0.00, 0.0),
    "M_LatebrakeEmissionPbr": (0.35, 0.05, 2.2),
}


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def expression(mat, cls, x, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        mat, cls, x, y
    )
    if node is None:
        raise RuntimeError("Material graph node creation failed: " + str(cls))
    return node


def constant(mat, value, x, y, property_kind):
    node = expression(mat, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", float(value))
    if not unreal.MaterialEditingLibrary.connect_material_property(
        node, "", property_kind
    ):
        raise RuntimeError("Unable to connect material scalar to output.")
    return node


def author_material(material, roughness, metallic, emissive):
    vertex = expression(
        material, unreal.MaterialExpressionVertexColor, -620, -150
    )
    if not unreal.MaterialEditingLibrary.connect_material_property(
        vertex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    ):
        raise RuntimeError("Material vertex color not connected to Base Color.")
    constant(
        material, roughness, -340, 60,
        unreal.MaterialProperty.MP_ROUGHNESS
    )
    constant(
        material, metallic, -340, 260,
        unreal.MaterialProperty.MP_METALLIC
    )
    if emissive > 0.0:
        strength = expression(
            material, unreal.MaterialExpressionConstant, -590, 460
        )
        strength.set_editor_property("r", float(emissive))
        multiply = expression(
            material, unreal.MaterialExpressionMultiply, -310, 430
        )
        if not unreal.MaterialEditingLibrary.connect_material_expressions(
            vertex, "RGB", multiply, "A"
        ):
            raise RuntimeError("Cannot connect color to emissive intensity.")
        if not unreal.MaterialEditingLibrary.connect_material_expressions(
            strength, "", multiply, "B"
        ):
            raise RuntimeError("Cannot connect intensity to emissive color.")
        if not unreal.MaterialEditingLibrary.connect_material_property(
            multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        ):
            raise RuntimeError("Cannot connect emissive property.")
    errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if errors:
        raise RuntimeError(
            "Unreal material compilation errors: " + "; ".join(map(str, errors))
        )


def create_or_preserve(name, profile, subsystem, tools):
    package = MATERIAL_FOLDER + "/" + name
    destination = CONTENT / (name + ".uasset")
    if unreal.EditorAssetLibrary.does_asset_exist(package):
        material = subsystem.load_asset(package)
        if not isinstance(material, unreal.Material):
            raise RuntimeError("Unexpected non-Material asset: " + package)
        # User-authored existing materials are never overwritten or
        # recompiled by this script. They require independent visual QA.
        operation = "existing_unmodified_requires_visual_review"
    else:
        material = tools.create_asset(
            name, MATERIAL_FOLDER,
            unreal.Material, unreal.MaterialFactoryNew()
        )
        if not isinstance(material, unreal.Material):
            raise RuntimeError("Unreal did not create a Material: " + package)
        author_material(material, *profile)
        if not subsystem.save_loaded_asset(material, True):
            raise RuntimeError("Unreal could not save Material: " + package)
        operation = "created_and_compiled_in_unreal_editor"
    if not destination.is_file() or destination.stat().st_size < 256:
        raise RuntimeError("No real serialized .uasset found at " + str(destination))
    return {
        "asset": package,
        "operation": operation,
        "bytes": destination.stat().st_size,
        "sha256": sha256_file(destination),
    }


def main():
    version = str(unreal.SystemLibrary.get_engine_version())
    if not version.startswith(ENGINE_PREFIX):
        raise RuntimeError(
            "Actual installed Unreal Editor 5.8 required; found " + version
        )
    subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    if subsystem is None:
        raise RuntimeError("EditorAssetSubsystem missing; Editor Python required.")
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    if asset_tools is None:
        raise RuntimeError("Unreal AssetTools missing.")
    # Never write a receipt from an earlier or partially failed pass.
    RESULT_PATH.unlink(missing_ok=True)
    receipt = []
    for name, profile in PROFILES.items():
        receipt.append(create_or_preserve(name, profile, subsystem, asset_tools))
    SAVED_FOLDER.mkdir(parents=True, exist_ok=True)
    RESULT_PATH.write_text(
        json.dumps({
            "schema_version": 1,
            "engine_version": "5.8",
            "status": "editor_material_assets_inspected",
            "native_gameplay_validated": False,
            "installed_cooked_game_validated": False,
            "all_newly_compiled": all(
                item["operation"] == "created_and_compiled_in_unreal_editor"
                for item in receipt
            ),
            "materials": receipt,
        }, sort_keys=True, indent=2),
        encoding="utf-8"
    )
    unreal.log(
        "LateBrake: preview material assets written/inspected by real UE "
        + version + " in " + str(CONTENT)
    )


main()
