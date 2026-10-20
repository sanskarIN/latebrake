"""Create/validate the genuine LateBrake map using licensed Unreal Editor 5.8.

Editor Python only. This file is executed by UnrealEditor.exe; no Python
runtime ships in the actual C++20 racing game.

Do not check levels with EditorAssetLibrary.does_asset_exist: the Unreal 5.8
API explicitly documents that method for assets *other than levels*.
"""

import json
from pathlib import Path

import unreal

PACKAGE = "/Game/Maps/LatebrakeCircuit"
MAP_FILE = Path(unreal.Paths.project_content_dir()) / "Maps" / "LatebrakeCircuit.umap"
RESULT_FILE = Path(unreal.Paths.project_saved_dir()) / "LatebrakeAutomation" / "starter-map-result.json"


def create_or_load_level(subsystem, map_file: Path = MAP_FILE) -> str:
    """Preserve an existing level; only Unreal Editor may serialize new maps."""
    if map_file.is_file():
        if not subsystem.load_level(PACKAGE):
            raise RuntimeError("Existing LatebrakeCircuit could not be loaded.")
        return "loaded"

    # This API creates and serializes a real engine .umap. This is NOT a
    # handwritten or synthetic map file.
    if not subsystem.new_level(PACKAGE, False):
        raise RuntimeError("Unreal failed to create/save LatebrakeCircuit.")
    return "created"


def main() -> None:
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if subsystem is None:
        raise RuntimeError(
            "Unreal LevelEditorSubsystem unavailable: use installed UE 5.8 Editor."
        )
    action = create_or_load_level(subsystem)
    if not subsystem.save_current_level():
        raise RuntimeError("Unreal failed to save the current project level.")

    if not MAP_FILE.is_file() or MAP_FILE.stat().st_size < 1024:
        raise RuntimeError(
            "Serialized Unreal map is missing or unexpectedly small: "
            + str(MAP_FILE)
        )
    # The Windows wrapper deletes any previous receipt before launching the
    # Editor. A receipt is written ONLY after successful load/create + save.
    # This prevents an Editor exit code of zero after a swallowed Python error
    # from being misinterpreted as successful map authoring.
    RESULT_FILE.parent.mkdir(parents=True, exist_ok=True)
    RESULT_FILE.write_text(
        json.dumps({
            "status": "saved_by_unreal_editor",
            "map": PACKAGE,
            "operation": action,
            "bytes": MAP_FILE.stat().st_size,
        }, sort_keys=True),
        encoding="utf-8",
    )
    unreal.log(f"LateBrake: real Unreal level {action} and saved at {MAP_FILE}")
    unreal.log("LateBrake: the C++ game mode builds its prototype course at runtime.")


# Unreal -ExecutePythonScript executes this file as a top-level script.
# The unconditional call preserves that command-line execution contract.
main()
