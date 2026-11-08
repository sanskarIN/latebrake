"""Import original full-width 1.7km road as a STATIC Unreal Editor inspection map.

NOT a game runtime, UE build, cooked asset or proof of smooth Chaos tyre
contact. Do not alter the active LatebrakeCircuit, its game mode or the
older 68-slab lab. Fail closed if import/CollisionTraceFlag is unsupported.

Run only using scripts/New-ContinuousRoadLab.ps1 on authorized UE5.8.
"""
import hashlib
import hmac
import json
import math
import os
import re
import runpy
from pathlib import Path

import unreal

PROJECT_ROOT = Path(unreal.Paths.project_content_dir()).parent
ROOT_SAVED = Path(unreal.Paths.project_saved_dir()) / "LatebrakeAutomation"
OBJ = ROOT_SAVED / "SM_WorldScaleContinuousRoad.obj"
MTL = OBJ.with_suffix(".mtl")
LAB_PACKAGE = "/Game/Maps/LatebrakeWorldScaleContinuousCollisionLab"
LAB_MAP = PROJECT_ROOT / "Content/Maps/LatebrakeWorldScaleContinuousCollisionLab.umap"
ASSET_PACKAGE = "/Game/LatebrakeExperimental/SM_WorldScaleContinuousRoad"
ASSET_FILE = PROJECT_ROOT / "Content/LatebrakeExperimental/SM_WorldScaleContinuousRoad.uasset"
RECEIPT = ROOT_SAVED / "world-scale-continuous-road-import-result.json"
ROAD_LABEL = "ContinuousPhysicalRoadOnly"
GROUND_LABEL = "ContinuousCollisionLabGround"
HASH_OBJ_ENV = "LATEBRAKE_CONTINUOUS_ROAD_OBJ_SHA256"
HASH_MTL_ENV = "LATEBRAKE_CONTINUOUS_ROAD_MTL_SHA256"


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for piece in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(piece)
    return h.hexdigest()


def verify_source_file(path, variable):
    expected = os.environ.get(variable, "")
    if re.fullmatch(r"[a-f0-9]{64}", expected) is None:
        raise RuntimeError("Missing canonical physical-road source hash")
    if not path.is_file() or path.is_symlink() or path.stat().st_size < 200:
        raise RuntimeError("Original source OBJ/MTL missing, tiny or symlinked")
    if not hmac.compare_digest(expected, digest(path)):
        raise RuntimeError("Continuous road import source bytes drifted")
    return expected


def verify_source_geometry():
    # Independent source-side numeric acceptance is REQUIRED before calling
    # ANY Unreal asset/import/map mutation. This is not Chaos contact QA.
    verifier = PROJECT_ROOT / "Tests/Standalone/VerifyWorldScaleCollisionRoadObj.py"
    if not verifier.is_file():
        raise RuntimeError("Independent full-width source validator missing")
    import sys
    import_path = str(verifier.parent)
    sys.path.insert(0, import_path)
    try:
        validator = runpy.run_path(str(verifier))
        result = validator["verify_obj"](OBJ, MTL)
    finally:
        sys.path.remove(import_path)
    if result != (4096, 8192):
        raise RuntimeError("Original road OBJ numeric contract mismatch")


def require_unmodified_destination():
    if LAB_MAP.exists() or ASSET_FILE.exists():
        raise RuntimeError("Continuous road lab asset/map already exists; never overwrite")
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PACKAGE):
        raise RuntimeError("Original continuous-road asset path already occupied")


def inspect_mesh(mesh):
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("OBJ importer did not yield a real Unreal StaticMesh")
    body = mesh.get_editor_property("body_setup")
    expected = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
    if body is None or body.get_editor_property("collision_trace_flag") != expected:
        raise RuntimeError("Imported road lacks static triangle complex-as-simple collision")
    if mesh.get_num_lods() < 1 or mesh.get_num_triangles(0) != 8192:
        raise RuntimeError("Imported static mesh lost original 8192 manifold triangles")
    # Seam and UV splits can increase imported render-vertex counts, but the
    # original surface must not degenerate into a 3D bounding box.
    if not (4096 <= mesh.get_num_vertices(0) <= 16384):
        raise RuntimeError("Imported road render vertex count outside bounded source")
    bounds = mesh.get_bounds()
    half = bounds.box_extent
    if not (25000 < float(half.x) < 31000 and
            15000 < float(half.y) < 21000 and
            math.isfinite(float(half.z)) and 20 < float(half.z) < 22):
        raise RuntimeError("Unreal OBJ scale/axis import changed the 1.7-km road bounds")
    return body


def import_mesh():
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(OBJ))
    task.set_editor_property("destination_path", "/Game/LatebrakeExperimental")
    task.set_editor_property("destination_name", "SM_WorldScaleContinuousRoad")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", False)
    task.set_editor_property("async_", False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths = [str(item) for item in task.get_editor_property("imported_object_paths")]
    # Interchange may use custom naming settings, but we fail closed when
    # the importer creates any unexpected file or splits by OBJ group.
    if len(paths) != 1 or paths[0].split(".")[0] != ASSET_PACKAGE:
        raise RuntimeError(f"OBJ import yielded unexpected assets: {paths!r}")
    mesh = unreal.load_asset(ASSET_PACKAGE)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Unreal could not load imported continuous physical road")
    body = mesh.get_editor_property("body_setup")
    if body is None:
        raise RuntimeError("Importer created no usable collision BodySetup")
    body.set_editor_property(
        "collision_trace_flag",
        unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    inspect_mesh(mesh)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError("Could not save original Unreal collision StaticMesh asset")
    if not ASSET_FILE.is_file() or ASSET_FILE.stat().st_size < 1024:
        raise RuntimeError("Genuine Unreal .uasset was not written")
    inspect_mesh(unreal.load_asset(ASSET_PACKAGE))
    return mesh


def xyz(value):
    return float(value.x), float(value.y), float(value.z)


def close_vector(actual, expected, tolerance=0.001):
    return all(math.isfinite(a) and abs(a-b) < tolerance
               for a, b in zip(xyz(actual), expected))


def configure_actor(actor, mesh_asset, label, position, scale):
    if actor is None:
        raise RuntimeError("Unreal could not spawn collision-lab static actor")
    component = actor.get_editor_property("static_mesh_component")
    if component is None:
        raise RuntimeError("Collision-lab actor missing StaticMeshComponent")
    component.set_static_mesh(mesh_asset)
    component.set_collision_profile_name("BlockAll")
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    component.set_simulate_physics(False)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_actor_label(label)


def audit_level(actors, world_api, road, cube):
    world = world_api.get_editor_world()
    if world is None:
        raise RuntimeError("Dedicated continuous-road Editor world missing")
    settings = world.get_world_settings()
    if (settings is None or settings.get_editor_property("default_game_mode")
            != unreal.GameModeBase.static_class()):
        raise RuntimeError("Unreal road lab lost its isolated non-racing GameModeBase")
    found = {}
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() in (ROAD_LABEL, GROUND_LABEL):
            if actor.get_actor_label() in found:
                raise RuntimeError("Duplicate collision-lab road actor")
            found[actor.get_actor_label()] = actor
    if set(found) != {ROAD_LABEL, GROUND_LABEL}:
        raise RuntimeError("Missing 9-m physical road or dedicated off-road ground")
    for name, mesh_asset, origin, scale in (
        (ROAD_LABEL, road, (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)),
        (GROUND_LABEL, cube, (0.0, 0.0, -100.0), (1100.0, 850.0, 1.25)),
    ):
        actor = found[name]
        component = actor.get_editor_property("static_mesh_component")
        if component is None or component.get_editor_property("static_mesh") != mesh_asset:
            raise RuntimeError("Persisted physical road mesh identity drift")
        if (str(component.get_collision_profile_name()) != "BlockAll" or
            component.get_collision_enabled() !=
                unreal.CollisionEnabled.QUERY_AND_PHYSICS or
            component.is_simulating_physics() or
            not actor.get_actor_enable_collision()):
            raise RuntimeError("Continuous physical road lost STATIC BlockAll collision")
        if (not close_vector(actor.get_actor_location(), origin) or
            not close_vector(actor.get_actor_scale3d(), scale)):
            raise RuntimeError("Persisted continuous road transform/size drift")
        rotation = actor.get_actor_rotation()
        if any(abs(v) > 0.001 for v in (
            float(rotation.roll), float(rotation.pitch), float(rotation.yaw))):
            raise RuntimeError("Unexpected physical-road rotation")
    inspect_mesh(road)
    return 2


def author_level(road, actors, editor, world_api):
    if not editor.new_level(LAB_PACKAGE, False):
        raise RuntimeError("Unreal Editor could not create independent continuous lab")
    world = world_api.get_editor_world()
    settings = world.get_world_settings() if world is not None else None
    if settings is None:
        raise RuntimeError("Editor world/settings not available")
    settings.set_editor_property("default_game_mode", unreal.GameModeBase.static_class())
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not isinstance(cube, unreal.StaticMesh):
        raise RuntimeError("Engine original Cube missing for non-racing ground")
    ground = actors.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(0, 0, -100),
        unreal.Rotator(roll=0, pitch=0, yaw=0), False)
    configure_actor(ground, cube, GROUND_LABEL, (0, 0, -100),
                    (1100, 850, 1.25))
    actual = actors.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(0, 0, 0),
        unreal.Rotator(roll=0, pitch=0, yaw=0), False)
    configure_actor(actual, road, ROAD_LABEL, (0, 0, 0), (1, 1, 1))
    audit_level(actors, world_api, road, cube)
    if not editor.save_current_level() or not LAB_MAP.is_file():
        raise RuntimeError("Genuine Unreal continuous-road .umap did not save")
    if LAB_MAP.stat().st_size < 1024 or not editor.load_level(LAB_PACKAGE):
        raise RuntimeError("Genuine Unreal continuous-road lab reload failed")
    audit_level(actors, world_api, road, cube)
    return 2


def main():
    if not str(unreal.SystemLibrary.get_engine_version()).startswith("5.8."):
        raise RuntimeError("Original continuous physical road lab needs UE 5.8")
    require_unmodified_destination()
    if RECEIPT.exists():
        RECEIPT.unlink()
    source_obj = verify_source_file(OBJ, HASH_OBJ_ENV)
    source_mtl = verify_source_file(MTL, HASH_MTL_ENV)
    verify_source_geometry()
    if source_obj != digest(OBJ) or source_mtl != digest(MTL):
        raise RuntimeError("Original physical OBJ/MTL mutated after geometry validation")
    editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world_api = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if editor is None or actors is None or world_api is None:
        raise RuntimeError("Licensed Unreal Editor subsystems unavailable")
    road = import_mesh()
    if source_obj != digest(OBJ) or source_mtl != digest(MTL):
        raise RuntimeError("Physical road source changed during Unreal import")
    authored = author_level(road, actors, editor, world_api)
    if authored != 2 or not ASSET_FILE.is_file() or not LAB_MAP.is_file():
        raise RuntimeError("Incomplete saved continuous collision asset/map")
    ROOT_SAVED.mkdir(parents=True, exist_ok=True)
    RECEIPT.write_text(json.dumps({
        "schema_version": 1,
        "status": "saved_and_reloaded_by_unreal_editor",
        "engine_version": "5.8",
        "road_mesh": ASSET_PACKAGE,
        "map": LAB_PACKAGE,
        "actors_audited_after_reload": 2,
        "physical_road_width_cm": 900,
        "source_obj_sha256": source_obj,
        "source_mtl_sha256": source_mtl,
        "asset_sha256": digest(ASSET_FILE),
        "asset_bytes": ASSET_FILE.stat().st_size,
        "map_sha256": digest(LAB_MAP),
        "map_bytes": LAB_MAP.stat().st_size,
        "collision_trace_flag": "CTF_USE_COMPLEX_AS_SIMPLE",
        "collision_profile": "BlockAll",
        "static_road": True,
        "race_game_mode_disabled": True,
        "road_chaos_contact_tested": False,
        "actual_wheel_qa": "NOT_RUN",
    }, sort_keys=True), encoding="utf-8")
    unreal.log("LATEBRAKE_CONTINUOUS_ROAD_EDITOR_SOURCE_ASSET_SAVED: 1 road + 1 ground, no gameplay")
    unreal.log("NO CHAOS TYRE-CONTACT QA, NO COOK, NO PLAYABLE WINDOWS BUILD.")


main()
