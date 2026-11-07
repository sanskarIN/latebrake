"""Build a SEPARATE, genuine Unreal Editor 5.8 collision-road inspection map.

Input is exported by the actual standalone C++ RoundedCircuitRoadPlan; this
script is run only by UnrealEditor.exe and does not run in a normal Python
interpreter. It never changes the active LatebrakeCircuit or default map.
"""

import hashlib
import json
import math
from pathlib import Path

import unreal

LAB_PACKAGE = "/Game/Maps/LatebrakeRoundedPhysicsLab"
LAB_MAP = Path(unreal.Paths.project_content_dir()) / "Maps" / "LatebrakeRoundedPhysicsLab.umap"
ROOT_SAVED = Path(unreal.Paths.project_saved_dir()) / "LatebrakeAutomation"
GEOMETRY_FILE = ROOT_SAVED / "rounded-road-pieces.json"
RECEIPT = ROOT_SAVED / "rounded-physics-lab-result.json"
EXPECTED_PIECES = 68


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for part in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(part)
    return digest.hexdigest()


def read_verified_geometry(path):
    """Fail closed on corrupt or stale geometry; never accept invented roads."""
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or set(data) != {
        "schema_version", "units", "road_top_cm", "piece_count", "pieces"
    }:
        raise RuntimeError("Rounded lab JSON schema mismatch")
    if (type(data["schema_version"]) is not int or data["schema_version"] != 1 or
        data["units"] != "cm" or type(data["road_top_cm"]) not in (int, float) or
        data["road_top_cm"] != -1.0 or type(data["piece_count"]) is not int or
        data["piece_count"] != EXPECTED_PIECES):
        raise RuntimeError("Rounded lab geometry version/units/road top mismatch")
    pieces = data["pieces"]
    if not isinstance(pieces, list) or len(pieces) != EXPECTED_PIECES:
        raise RuntimeError("Rounded lab missing physical collision pieces")
    for i, part in enumerate(pieces):
        if (not isinstance(part, dict) or
            set(part) != {"index", "position", "rotation_yaw", "scale"} or
            type(part["index"]) is not int or part["index"] != i):
            raise RuntimeError(f"Bad or reordered rounded road piece {i}")
        point = part["position"]
        scale = part["scale"]
        yaw = part["rotation_yaw"]
        if (not isinstance(point, list) or not isinstance(scale, list) or
            len(point) != 3 or len(scale) != 3):
            raise RuntimeError(f"Bad rounded road transform {i}")
        numbers = [*point, yaw, *scale]
        if not all(type(v) in (int, float) and math.isfinite(v) for v in numbers):
            raise RuntimeError(f"Nonfinite/non-numeric rounded road transform {i}")
        if (not all(v > 0 for v in scale) or abs(yaw) > 360 or
            abs(point[2] + 50.0 * scale[2] + 1.0) > 0.000001):
            raise RuntimeError(f"Misaligned/invalid collision road slab {i}")
        if (abs(scale[1] - 9.0) > 0.000001 or scale[0] > 45 or
            not (-4000 <= point[0] <= 4000 and -3000 <= point[1] <= 3000)):
            raise RuntimeError(f"Out-of-bounds rounded road slab {i}")
    return data


def author_map(subsystem, actors, world_api, cube, pieces):
    """No caller may create the production starter circuit through this path."""
    if LAB_MAP.exists():
        raise RuntimeError("Dedicated rounded lab map already exists; refusing to overwrite")
    if not subsystem.new_level(LAB_PACKAGE, False):
        raise RuntimeError("Could not create a genuine new dedicated rounded lab level")
    world = world_api.get_editor_world()
    if world is None:
        raise RuntimeError("Dedicated rounded lab world unavailable after level creation")
    settings = world.get_world_settings()
    if settings is None:
        raise RuntimeError("Unreal world settings unavailable; cannot isolate game mode")
    settings.set_editor_property("default_game_mode", unreal.GameModeBase)
    if settings.get_editor_property("default_game_mode") != unreal.GameModeBase:
        raise RuntimeError("Cannot enforce non-racing GameModeBase in lab map")
    # A separate ground slab 37.5 cm below the paved top provides off-road
    # collision/context but never constitutes one of the 68 tested road pieces.
    ground = actors.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, -100.0),
        unreal.Rotator(0.0, 0.0, 0.0), False)
    if ground is None:
        raise RuntimeError("Unreal failed to spawn dedicated lab ground")
    ground_mesh = ground.get_editor_property("static_mesh_component")
    if ground_mesh is None:
        raise RuntimeError("Lab ground is missing a mesh component")
    ground_mesh.set_static_mesh(cube)
    ground_mesh.set_collision_profile_name("BlockAll")
    ground_mesh.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    ground.set_actor_scale3d(unreal.Vector(110.0, 85.0, 1.25))
    ground.set_actor_label("RoundedPhysicsLabGround")

    spawned = 0
    for piece in pieces:
        position = piece["position"]
        yaw = piece["rotation_yaw"]
        actor = actors.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(position[0], position[1], position[2]),
            unreal.Rotator(0.0, yaw, 0.0),
            False,
        )
        if actor is None:
            raise RuntimeError(f"Unreal failed to spawn rounded slab {piece['index']}")
        mesh = actor.get_editor_property("static_mesh_component")
        if mesh is None:
            raise RuntimeError("Unreal collision road actor missing a mesh component")
        mesh.set_static_mesh(cube)
        mesh.set_collision_profile_name("BlockAll")
        mesh.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
        actor.set_actor_scale3d(unreal.Vector(*piece["scale"]))
        actor.set_actor_label(f"RoundedPhysicsRoad_{piece['index']:02d}")
        spawned += 1
    if spawned != EXPECTED_PIECES:
        raise RuntimeError("Partial collision road creation must fail before saving")
    if not subsystem.save_current_level():
        raise RuntimeError("Unreal failed to save rounded physics lab")
    if not LAB_MAP.is_file() or LAB_MAP.stat().st_size < 1024:
        raise RuntimeError("Unreal did not serialize a genuine lab .umap")
    return spawned


def main():
    engine = str(unreal.SystemLibrary.get_engine_version())
    if not engine.startswith("5.8."):
        raise RuntimeError("Rounded physics lab requires Unreal Editor 5.8")
    if RECEIPT.exists():
        RECEIPT.unlink()
    # Validate ALL source-exported transforms before creating any level.
    if LAB_MAP.exists():
        raise RuntimeError("Existing rounded lab .umap must be reviewed before rerunning")
    data = read_verified_geometry(GEOMETRY_FILE)
    geometry_sha = sha256_file(GEOMETRY_FILE)
    editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world_api = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if editor is None or actors is None or world_api is None:
        raise RuntimeError("Required licensed Unreal Editor subsystems missing")
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not isinstance(cube, unreal.StaticMesh):
        raise RuntimeError("Editor world or genuine Unreal Cube mesh unavailable")
    count = author_map(editor, actors, world_api, cube, data["pieces"])
    ROOT_SAVED.mkdir(parents=True, exist_ok=True)
    RECEIPT.write_text(json.dumps({
        "schema_version": 1,
        "status": "saved_by_unreal_editor",
        "engine_version": "5.8",
        "map": LAB_PACKAGE,
        "pieces": count,
        "ground_pieces": 1,
        "geometry_sha256": geometry_sha,
        "map_bytes": LAB_MAP.stat().st_size,
        "map_sha256": sha256_file(LAB_MAP),
        "game_mode": "GameModeBase",
    }, sort_keys=True), encoding="utf-8")
    unreal.log("LATEBRAKE_ROUNDED_LAB_AUTHORED collision_road_pieces=68 ground=1 racing=disabled")
    unreal.log("This is a separate unverified Editor-only physics inspection level, not a playable Win64 build.")


main()
