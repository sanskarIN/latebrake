"""Author LateBrake's real parameterized racing-line material in UE 5.8.

Editor-only. Run via UnrealEditor.exe -ExecutePythonScript, not system Python.
Produces one genuine .uasset that MUST be reviewed and committed through
Git LFS before any final reproducible Win64 candidate can be cooked.
This script never handwrites, simulates or redistributes an Unreal asset.
"""

from pathlib import Path
import hashlib

import unreal

ASSET_DIRECTORY = "/Game/Materials"
ASSET_NAME = "M_RacingLineGuide"
ASSET_PATH = f"{ASSET_DIRECTORY}/{ASSET_NAME}"
ASSET_FILE = (
    Path(unreal.Paths.project_content_dir())
    / "Materials"
    / f"{ASSET_NAME}.uasset"
)
COLOR_PARAMETER = "Color"
INSTANCED_USAGE = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES


def ensure_supported_editor() -> None:
    engine = str(unreal.SystemLibrary.get_engine_version())
    if not engine.startswith("5.8."):
        raise RuntimeError(f"Racing-line material requires genuine UE 5.8; got {engine}")


def inspect_material(material) -> None:
    """Reject similarly named but incompatible existing authored assets."""
    if not isinstance(material, unreal.Material):
        raise RuntimeError(f"{ASSET_PATH} exists but is not a material asset.")
    names = {
        str(name) for name in
        unreal.MaterialEditingLibrary.get_vector_parameter_names(material)
    }
    if COLOR_PARAMETER not in names:
        raise RuntimeError(
            f"{ASSET_PATH} has no vector '{COLOR_PARAMETER}' parameter."
        )
    if not unreal.MaterialEditingLibrary.has_material_usage(
        material, INSTANCED_USAGE
    ):
        raise RuntimeError(
            f"{ASSET_PATH} must support Instanced Static Meshes."
        )


def create_or_validate_material(asset_subsystem):
    if asset_subsystem.does_asset_exist(ASSET_PATH):
        # Never rewrite the graph of an existing artist-authored material.
        existing = asset_subsystem.load_asset(ASSET_PATH)
        if existing is None:
            raise RuntimeError(f"Unable to load existing {ASSET_PATH}.")
        inspect_material(existing)
        return existing, "validated_existing"

    unreal.EditorAssetLibrary.make_directory(ASSET_DIRECTORY)
    factory = unreal.MaterialFactoryNew()
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, ASSET_DIRECTORY, unreal.Material, factory
    )
    if material is None:
        raise RuntimeError("Unreal could not create the racing-line material.")
    param = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -400, 0
    )
    if param is None:
        raise RuntimeError("Unreal could not add the Color parameter node.")
    param.set_editor_property("parameter_name", COLOR_PARAMETER)
    param.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    if not unreal.MaterialEditingLibrary.connect_material_property(
        param, "", unreal.MaterialProperty.MP_BASE_COLOR
    ):
        raise RuntimeError("Cannot connect Color to material Base Color.")
    unreal.MaterialEditingLibrary.set_base_material_usage(
        material, INSTANCED_USAGE, True
    )
    compiler_errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if compiler_errors:
        raise RuntimeError(
            "Unreal material compile errors: " + "; ".join(map(str, compiler_errors))
        )
    inspect_material(material)
    if not asset_subsystem.save_loaded_asset(material, False):
        raise RuntimeError("Unreal could not save the genuine racing-line .uasset.")
    return material, "created"


def main() -> None:
    ensure_supported_editor()
    subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    if subsystem is None:
        raise RuntimeError("EditorAssetSubsystem is unavailable; run Unreal Editor.")
    _, result = create_or_validate_material(subsystem)
    if not ASSET_FILE.is_file() or ASSET_FILE.stat().st_size < 1024:
        raise RuntimeError("Unreal did not save a plausible real .uasset file.")
    checksum = hashlib.sha256(ASSET_FILE.read_bytes()).hexdigest()
    unreal.log(f"LATEBRAKE_GUIDE_MATERIAL status={result} asset={ASSET_PATH}")
    unreal.log(f"LATEBRAKE_GUIDE_MATERIAL asset_sha256={checksum}")
    unreal.log(
        "Commit the genuine material asset via Git LFS before building a "
        "source-pinned candidate; this log is NOT a rendering or Win64 QA pass."
    )


# Required for Unreal's -ExecutePythonScript command-line execution.
main()
