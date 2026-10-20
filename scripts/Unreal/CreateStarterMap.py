"""Create the real starter map inside a licensed Unreal Engine 5.8 Editor.

This is an EDITOR-ONLY asset-authoring helper; Python is not part of the
LateBrake runtime. This script cannot create a .umap without Unreal Editor.
Do not call Python directly to pretend an Unreal map has been authored.
"""

import os
from pathlib import Path
import unreal

MAP_PACKAGE = "/Game/Maps/LatebrakeCircuit"
MAP_FILE = Path(unreal.Paths.project_content_dir()) / "Maps" / "LatebrakeCircuit.umap"

subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if subsystem is None:
    raise RuntimeError("Unreal LevelEditorSubsystem unavailable: use Unreal Editor 5.8.")

assets = unreal.EditorAssetLibrary
if assets.does_asset_exist(MAP_PACKAGE):
    # Idempotent: existing authored map is never destroyed or overwritten by
    # automatically creating a new blank world.
    if not subsystem.load_level(MAP_PACKAGE):
        raise RuntimeError("Existing LatebrakeCircuit could not be loaded.")
    unreal.log("LateBrake: existing project map loaded, not replaced.")
else:
    # new_level does the actual engine-authorized binary .umap save.
    if not subsystem.new_level(MAP_PACKAGE, False):
        raise RuntimeError("Unreal could not create/save LatebrakeCircuit.")
    unreal.log("LateBrake: new real .umap created by Unreal Editor.")

if not subsystem.save_current_level():
    raise RuntimeError("Unreal failed to save the project map.")

if not MAP_FILE.is_file() or MAP_FILE.stat().st_size < 1024:
    raise RuntimeError("Saved Unreal .umap missing or unexpectedly small: " + str(MAP_FILE))

unreal.log("LateBrake: authentic Unreal map verified at " + str(MAP_FILE))
unreal.log("LateBrake: the C++ game mode generates the track at runtime.")
