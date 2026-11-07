"""Run genuine UE5.8 Editor static-complex road/ground line-trace QA.

Works ONLY with a previously authored dedicated experimental collision lab.
No imports, game runtime, cook, career integration, dynamic tire/suspension
simulation, rendering or packaged game. A PASS here proves static collision
queries in an actual Editor world, NOT driving quality or Chaos wheel contact.
"""
import hashlib
import hmac
import json
import math
import os
from pathlib import Path
import re
import runpy
import sys

import unreal

CONTENT_ROOT = Path(unreal.Paths.project_content_dir())
PROJECT = CONTENT_ROOT.parent
ROOT_SAVED = Path(unreal.Paths.project_saved_dir()) / "LatebrakeAutomation"
MAP_PACKAGE = "/Game/Maps/LatebrakeWorldScaleContinuousCollisionLab"
MAP_FILE = CONTENT_ROOT / "Maps/LatebrakeWorldScaleContinuousCollisionLab.umap"
MESH_PACKAGE = "/Game/LatebrakeExperimental/SM_WorldScaleContinuousRoad"
MESH_FILE = CONTENT_ROOT / "LatebrakeExperimental/SM_WorldScaleContinuousRoad.uasset"
SOURCE_OBJ = ROOT_SAVED / "SM_WorldScaleContinuousRoad.obj"
SOURCE_MTL = SOURCE_OBJ.with_suffix(".mtl")
IMPORT_RECEIPT = ROOT_SAVED / "world-scale-continuous-road-import-result.json"
PLAN_FILE = ROOT_SAVED / "world-scale-continuous-ray-probes.json"
RESULT_FILE = ROOT_SAVED / "world-scale-continuous-ray-audit-result.json"
PLAN_SHA_ENV = "LATEBRAKE_CONTINUOUS_RAY_PLAN_SHA256"
ROAD_LABEL = "ContinuousPhysicalRoadOnly"
GROUND_LABEL = "ContinuousCollisionLabGround"

MAX_HEIGHT_ERROR_CM = 1.5
MAX_LATERAL_ERROR_CM = 2.0
MIN_NORMAL_Z = 0.98


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def safe_source_file(path):
    return path.is_file() and not path.is_symlink() and path.stat().st_size >= 200


def validate_preflight():
    if not str(unreal.SystemLibrary.get_engine_version()).startswith("5.8."):
        raise RuntimeError("Continuous static-ray QA must run under real UE5.8 Editor")
    if RESULT_FILE.exists():
        raise RuntimeError("Existing ray-query receipt must be preserved; no overwriting QA")
    for path in (MAP_FILE, MESH_FILE, SOURCE_OBJ, SOURCE_MTL, IMPORT_RECEIPT, PLAN_FILE):
        if not safe_source_file(path):
            raise RuntimeError(f"Missing, tiny or symlinked licensed-Editor QA input: {path.name}")
    source = json.loads(IMPORT_RECEIPT.read_text(encoding="utf-8"))
    fields = {
        "schema_version": 1,
        "status": "saved_and_reloaded_by_unreal_editor",
        "engine_version": "5.8",
        "road_mesh": MESH_PACKAGE,
        "map": MAP_PACKAGE,
        "actors_audited_after_reload": 2,
        "physical_road_width_cm": 900,
        "collision_trace_flag": "CTF_USE_COMPLEX_AS_SIMPLE",
        "collision_profile": "BlockAll",
        "static_road": True,
        "race_game_mode_disabled": True,
        "road_chaos_contact_tested": False,
        "actual_wheel_qa": "NOT_RUN",
    }
    if not isinstance(source, dict) or any(
        type(source.get(k)) is not type(v) or source.get(k) != v
        for k, v in fields.items()
    ):
        raise RuntimeError("Original real-Editor import receipt failed static road identity checks")
    for filename, field, length_field in (
        (MAP_FILE, "map_sha256", "map_bytes"),
        (MESH_FILE, "asset_sha256", "asset_bytes"),
    ):
        if (source.get(field) != digest(filename) or
            type(source.get(length_field)) is not int or
            source[length_field] != filename.stat().st_size):
            raise RuntimeError(f"Saved experimental Unreal {filename.name} drifted since import")
    for file, field in (
        (SOURCE_OBJ, "source_obj_sha256"),
        (SOURCE_MTL, "source_mtl_sha256"),
    ):
        if source.get(field) != digest(file):
            raise RuntimeError(f"Independent compiled road {file.name} source bytes changed")
    expected_sha = os.environ.get(PLAN_SHA_ENV, "")
    if re.fullmatch(r"[a-f0-9]{64}", expected_sha) is None:
        raise RuntimeError("Missing 64-hex canonical source collision trace-plan SHA")
    if not hmac.compare_digest(expected_sha, digest(PLAN_FILE)):
        raise RuntimeError("Editor collision trace-plan bytes changed after wrapper verification")
    tests_path = str(PROJECT / "Tests/Standalone")
    sys.path.insert(0, tests_path)
    try:
        validator = runpy.run_path(str(PROJECT / "Tests/Standalone/WorldScaleCollisionTracePlan.py"))
        plan = json.loads(PLAN_FILE.read_text(encoding="utf-8"))
        count = validator["validate_plan"](plan)
    finally:
        sys.path.remove(tests_path)
    if count != 1290:
        raise RuntimeError("Missing or fabricated static-road query samples")
    if not hmac.compare_digest(expected_sha, digest(PLAN_FILE)):
        raise RuntimeError("Static-road query plan changed during numeric verification")
    return source, plan, expected_sha


def _editor_field(hit, name):
    if hasattr(hit, "get_editor_property"):
        return hit.get_editor_property(name)
    return getattr(hit, name)


def checked_trace(world, position, expected_actor, expected_height):
    """One REAL world query, not a source-only simulated result.

    Epic's Python wrapper may expose either a nullable HitResult or
    a bool/HitResult tuple. Both must actually report a blocking hit.
    """
    x, y = position
    answer = unreal.SystemLibrary.line_trace_single(
        world,
        unreal.Vector(float(x), float(y), 250.0),
        unreal.Vector(float(x), float(y), -200.0),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        True, [], unreal.DrawDebugTrace.NONE, True,
    )
    if isinstance(answer, tuple):
        if len(answer) != 2 or answer[0] is not True:
            raise RuntimeError(f"Static-road Editor query returned no blocking hit at {position}")
        hit = answer[1]
    else:
        hit = answer
    if hit is None or _editor_field(hit, "blocking_hit") is not True:
        raise RuntimeError(f"Static-road Editor query missed physical geometry: {position}")
    actor = _editor_field(hit, "hit_actor")
    if actor is None or actor.get_actor_label() != (
        ROAD_LABEL if expected_actor == "road" else GROUND_LABEL
    ):
        raise RuntimeError(f"Static-road Editor query hit wrong actor near {position}")
    impact = _editor_field(hit, "impact_point")
    normal = _editor_field(hit, "impact_normal")
    if impact is None or normal is None:
        raise RuntimeError("Physical-road query has no collision contact location/normal")
    hit_x, hit_y, hit_z, nz = (float(impact.x), float(impact.y),
                              float(impact.z), float(normal.z))
    if not all(math.isfinite(v) for v in (hit_x, hit_y, hit_z, nz)):
        raise RuntimeError("Nonfinite real Editor complex-collision query")
    if (abs(hit_x - x) > MAX_LATERAL_ERROR_CM or
        abs(hit_y - y) > MAX_LATERAL_ERROR_CM or
        abs(hit_z - expected_height) > MAX_HEIGHT_ERROR_CM or
        nz < MIN_NORMAL_Z or nz > 1.001):
        raise RuntimeError(
            f"Physical-road query has incorrect contact location/height/normal at {position}: "
            f"hit=({hit_x},{hit_y},{hit_z}), up={nz}"
        )
    return abs(hit_z - expected_height), nz


def find_lab_world():
    editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_api = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world_api = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if editor is None or actor_api is None or world_api is None:
        raise RuntimeError("Genuine Unreal Editor world/actor subsystems missing")
    if not editor.load_level(MAP_PACKAGE):
        raise RuntimeError("Could not load existing 1.70-km isolated Editor lab map")
    world = world_api.get_editor_world()
    if world is None:
        raise RuntimeError("UE Editor returned no loaded world to trace")
    settings = world.get_world_settings()
    if (settings is None or settings.get_editor_property("default_game_mode")
            != unreal.GameModeBase.static_class()):
        raise RuntimeError("Continuous-road lab is no longer isolated from RaceGameMode")
    actors = {}
    for actor in actor_api.get_all_level_actors():
        name = actor.get_actor_label()
        if name in (ROAD_LABEL, GROUND_LABEL):
            if name in actors:
                raise RuntimeError("Duplicate road/ground actor in dedicated Editor lab")
            actors[name] = actor
    if set(actors) != {ROAD_LABEL, GROUND_LABEL}:
        raise RuntimeError("Missing real static road or dedicated offroad ground")
    for name in (ROAD_LABEL, GROUND_LABEL):
        actor = actors[name]
        component = actor.get_editor_property("static_mesh_component")
        if component is None or component.get_collision_enabled() != unreal.CollisionEnabled.QUERY_AND_PHYSICS:
            raise RuntimeError(f"Missing static collision on loaded actor {name}")
        if (str(component.get_collision_profile_name()) != "BlockAll" or
            component.is_simulating_physics() or not actor.get_actor_enable_collision()):
            raise RuntimeError(f"Editor actor {name} lost static BlockAll qualification")
    mesh = unreal.load_asset(MESH_PACKAGE)
    component = actors[ROAD_LABEL].get_editor_property("static_mesh_component")
    if mesh is None or component.get_editor_property("static_mesh") != mesh:
        raise RuntimeError("Editor trace would hit unrelated road StaticMesh")
    if (mesh.get_editor_property("body_setup") is None or
        mesh.get_editor_property("body_setup").get_editor_property("collision_trace_flag")
            != unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE):
        raise RuntimeError("Unreal road lost complex-as-simple static triangle collision")
    return world


def run_qa(world, plan):
    road_hits = ground_hits = 0
    max_height_error = 0.0
    min_up = 1.0
    for i, sample in enumerate(plan["samples"]):
        try:
            height_error, nz = checked_trace(
                world, (sample["x_cm"], sample["y_cm"]),
                sample["expected_actor"], sample["expected_z_cm"]
            )
        except Exception as exc:
            raise RuntimeError(
                f"Real Editor collision trace failed at probe {i}/1290 "
                f"station {sample['station']} lateral {sample['offset_cm']}cm"
            ) from exc
        road_hits += int(sample["expected_actor"] == "road")
        ground_hits += int(sample["expected_actor"] == "ground")
        max_height_error = max(max_height_error, height_error)
        min_up = min(min_up, nz)
    if road_hits != 774 or ground_hits != 516:
        raise RuntimeError("Genuine continuous-road trace counts unexpected")
    return road_hits, ground_hits, max_height_error, min_up


def main():
    source, plan, plan_sha = validate_preflight()
    world = find_lab_world()
    road, ground, error, normal_z = run_qa(world, plan)
    # Also close the time-of-check window after actual engine queries.
    if (plan_sha != digest(PLAN_FILE) or
        source["map_sha256"] != digest(MAP_FILE) or
        source["asset_sha256"] != digest(MESH_FILE) or
        source["source_obj_sha256"] != digest(SOURCE_OBJ) or
        source["source_mtl_sha256"] != digest(SOURCE_MTL)):
        raise RuntimeError("UE world, mesh or query source mutated during QA")
    RESULT_FILE.parent.mkdir(parents=True, exist_ok=True)
    RESULT_FILE.write_text(json.dumps({
        "schema_version": 1,
        "status": "genuine_ue58_static_complex_line_queries_passed",
        "engine_version": "5.8",
        "map": MAP_PACKAGE,
        "road_mesh": MESH_PACKAGE,
        "query_plan_sha256": plan_sha,
        "map_sha256": source["map_sha256"],
        "asset_sha256": source["asset_sha256"],
        "source_obj_sha256": source["source_obj_sha256"],
        "source_mtl_sha256": source["source_mtl_sha256"],
        "ray_queries_performed": road + ground,
        "actual_road_hits": road,
        "actual_ground_hits": ground,
        "max_height_error_cm": error,
        "minimum_upward_normal_z": normal_z,
        "query_complex": True,
        "import_asset_static": True,
        "playable_game_tested": False,
        "dynamic_chaos_wheel_contact_tested": False,
        "runtime_track_enabled": False,
    }, sort_keys=True, allow_nan=False), encoding="utf-8")
    unreal.log(f"LATEBRAKE_STATIC_COMPLEX_EDITOR_TRACE_PASS road={road} ground={ground} "
               f"maximum_z_error_cm={error:.6f} minimum_normal_z={normal_z:.6f}")
    unreal.log("Source-to-static-Editor collision verified only. "
               "NO DYNAMIC CHAOS WHEEL CONTACT, NO DRIVING, NO PACKAGED GAME.")


main()
