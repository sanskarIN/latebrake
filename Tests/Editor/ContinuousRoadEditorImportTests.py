"""Mock-only fail-closed UE5.8 continuous collision import acceptance gates.

All examples use disposable synthetic byte sequences, never genuine .uasset
or a licensed engine. The independent compiled C++ OBJ validator is a
separate mandatory CTest; this suite only verifies Editor orchestration.
"""
import hashlib
import json
import os
from pathlib import Path
import runpy
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[2] / "scripts/Unreal/ImportWorldScaleContinuousRoad.py"


class FakeBody:
    def __init__(self):
        self.flag = 0
    def set_editor_property(self, key, value):
        assert key == "collision_trace_flag"
        self.flag = value
    def get_editor_property(self, key):
        assert key == "collision_trace_flag"
        return self.flag


class FakeStaticMesh:
    def __init__(self):
        self.body = FakeBody()
        self.triangles = 8192
        self.vertices = 6150
        self.half = [28000.0, 18000.0, 21.0]
    def get_editor_property(self, key):
        assert key == "body_setup"
        return self.body
    def get_num_lods(self): return 1
    def get_num_triangles(self, level): assert level == 0; return self.triangles
    def get_num_vertices(self, level): assert level == 0; return self.vertices
    def get_bounds(self):
        return types.SimpleNamespace(box_extent=types.SimpleNamespace(
            x=self.half[0], y=self.half[1], z=self.half[2]))


class FakeTask:
    def __init__(self):
        self.values = {}
    def set_editor_property(self, key, value):
        self.values[key] = value
    def get_editor_property(self, key):
        return self.values.get(key)


class FakeComponent:
    def __init__(self):
        self.asset = None
        self.profile = "NoCollision"
        self.collision = 0
        self.simulating = False
    def set_static_mesh(self, mesh): self.asset = mesh
    def set_collision_profile_name(self, profile): self.profile = profile
    def set_collision_enabled(self, kind): self.collision = kind
    def set_simulate_physics(self, value): self.simulating = value
    def get_editor_property(self, key):
        assert key == "static_mesh"
        return self.asset
    def get_collision_profile_name(self): return self.profile
    def get_collision_enabled(self): return self.collision
    def is_simulating_physics(self): return self.simulating


class FakeActor:
    def __init__(self, pos, rot):
        self.pos, self.rot = pos, rot
        self.scale = None
        self.label = None
        self.component = FakeComponent()
    def get_editor_property(self, key):
        assert key == "static_mesh_component"
        return self.component
    def set_actor_scale3d(self, scale): self.scale = scale
    def set_actor_label(self, label): self.label = label
    def get_actor_label(self): return self.label
    def get_actor_location(self): return self.pos
    def get_actor_rotation(self): return self.rot
    def get_actor_scale3d(self): return self.scale
    def get_actor_enable_collision(self): return True


class FakeActors:
    def __init__(self):
        self.spawned = []
        self.on_spawn = None
    def spawn_actor_from_class(self, cls, pos, rot, transient):
        actor = FakeActor(pos, rot)
        self.spawned.append(actor)
        return actor
    def get_all_level_actors(self): return self.spawned


class FakeSettings:
    def __init__(self): self.mode = None
    def set_editor_property(self, key, value):
        assert key == "default_game_mode"
        self.mode = value
    def get_editor_property(self, key):
        assert key == "default_game_mode"
        return self.mode


class FakeLevel:
    def __init__(self, path):
        self.path = path
        self.created = self.saved = self.reloaded = False
        self.on_reload = None
        self.can_save = True
    def new_level(self, pkg, partitioned):
        assert pkg == "/Game/Maps/LatebrakeWorldScaleContinuousCollisionLab"
        assert not partitioned
        self.created = True
        return True
    def save_current_level(self):
        self.saved = True
        if not self.can_save: return False
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_bytes(b"FAKE_EDITOR_LEVEL_NEVER_COOKED" * 100)
        return True
    def load_level(self, pkg):
        assert pkg == "/Game/Maps/LatebrakeWorldScaleContinuousCollisionLab"
        self.reloaded = True
        if self.on_reload: self.on_reload()
        return True


class FakeImportTools:
    def __init__(self):
        self.tasks = []
        self.paths = ["/Game/LatebrakeExperimental/"
                      "SM_WorldScaleContinuousRoad.SM_WorldScaleContinuousRoad"]
    def import_asset_tasks(self, tasks):
        self.tasks += tasks
        for task in tasks:
            task.values["imported_object_paths"] = self.paths


class ContinuousRoadEditorImportTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix="latebrake-continuous-mock-")
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.obj = self.root / "Saved/LatebrakeAutomation/SM_WorldScaleContinuousRoad.obj"
        self.mtl = self.obj.with_suffix(".mtl")
        self.asset = self.root / "Content/LatebrakeExperimental/SM_WorldScaleContinuousRoad.uasset"
        self.map = self.root / "Content/Maps/LatebrakeWorldScaleContinuousCollisionLab.umap"
        self.receipt = self.root / "Saved/LatebrakeAutomation/world-scale-continuous-road-import-result.json"
        self.verifier = self.root / "Tests/Standalone/VerifyWorldScaleCollisionRoadObj.py"
        self.obj.parent.mkdir(parents=True, exist_ok=True)
        self.verifier.parent.mkdir(parents=True, exist_ok=True)
        self.verifier.write_text("# synthetic mock validator stub\n", encoding="utf-8")
        self.obj.write_bytes(b"FAKE_FULL_WIDTH_OBJ_NUMERIC_CONTRACT_ONLY" * 30)
        self.mtl.write_bytes(b"FAKE_PHYSICAL_ROAD_MTL_ONLY" * 30)
        self.road = FakeStaticMesh()
        self.cube = FakeStaticMesh()
        self.asset_tools = FakeImportTools()
        self.actors = FakeActors()
        self.level = FakeLevel(self.map)
        self.settings = FakeSettings()
        self.existing_asset = False
        self.can_save_asset = True
        self.engine_version = "5.8.1"
        self.events = []

    @staticmethod
    def hash(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    def run_import(self, *, environment=None):
        fake = types.ModuleType("unreal")
        fake.Paths = types.SimpleNamespace(
            project_content_dir=lambda: str(self.root / "Content"),
            project_saved_dir=lambda: str(self.root / "Saved"))
        fake.SystemLibrary = types.SimpleNamespace(
            get_engine_version=lambda: self.engine_version)
        fake.StaticMesh = FakeStaticMesh
        fake.AssetImportTask = FakeTask
        fake.CollisionTraceFlag = types.SimpleNamespace(
            CTF_USE_COMPLEX_AS_SIMPLE=3)
        fake.CollisionEnabled = types.SimpleNamespace(QUERY_AND_PHYSICS=3)
        fake.LevelEditorSubsystem = type("LevelEditorSubsystem", (), {})
        fake.EditorActorSubsystem = type("EditorActorSubsystem", (), {})
        fake.UnrealEditorSubsystem = type("UnrealEditorSubsystem", (), {})
        fake.StaticMeshActor = type("StaticMeshActor", (), {})
        fake.GameModeBase = type("GameModeBase", (), {
            "static_class": classmethod(lambda cls: cls)})
        fake.Vector = lambda *xyz: types.SimpleNamespace(
            x=xyz[0], y=xyz[1], z=xyz[2])
        fake.Rotator = lambda roll=0, pitch=0, yaw=0: types.SimpleNamespace(
            roll=roll, pitch=pitch, yaw=yaw)
        fake.load_asset = lambda name: (
            self.road if name == "/Game/LatebrakeExperimental/SM_WorldScaleContinuousRoad"
            else self.cube if name == "/Engine/BasicShapes/Cube.Cube" else None)
        fake.AssetToolsHelpers = types.SimpleNamespace(
            get_asset_tools=lambda: self.asset_tools)
        def save_mesh(mesh):
            if not self.can_save_asset: return False
            self.asset.parent.mkdir(parents=True, exist_ok=True)
            self.asset.write_bytes(b"FAKE_EDITOR_ASSET_NOT_A_REAL_UASSET" * 100)
            return True
        fake.EditorAssetLibrary = types.SimpleNamespace(
            does_asset_exist=lambda pkg: self.existing_asset,
            save_loaded_asset=save_mesh)
        fake.get_editor_subsystem = lambda cls: {
            fake.LevelEditorSubsystem: self.level,
            fake.EditorActorSubsystem: self.actors,
            fake.UnrealEditorSubsystem: types.SimpleNamespace(
                get_editor_world=lambda: types.SimpleNamespace(
                    get_world_settings=lambda: self.settings)),
        }[cls]
        fake.log = self.events.append
        original = runpy.run_path
        def intercept(path, *args, **kwargs):
            if str(path).endswith("/Tests/Standalone/VerifyWorldScaleCollisionRoadObj.py"):
                return {"verify_obj": lambda obj, mtl: (4096, 8192)}
            return original(path, *args, **kwargs)
        env = {
            "LATEBRAKE_CONTINUOUS_ROAD_OBJ_SHA256": self.hash(self.obj),
            "LATEBRAKE_CONTINUOUS_ROAD_MTL_SHA256": self.hash(self.mtl),
        }
        if environment: env.update(environment)
        with (patch.dict(sys.modules, {"unreal": fake}),
              patch.dict(os.environ, env), patch.object(runpy, "run_path",
                                                       side_effect=intercept)):
            runpy.run_path(str(SCRIPT), run_name="__main__")

    def test_success_writes_isolated_map_and_asset_and_verified_receipt(self):
        self.run_import()
        self.assertEqual(len(self.asset_tools.tasks), 1)
        task = self.asset_tools.tasks[0]
        self.assertFalse(task.values["replace_existing"])
        self.assertTrue(task.values["automated"])
        self.assertFalse(task.values["async_"])
        self.assertTrue(self.level.reloaded)
        self.assertEqual(len(self.actors.spawned), 2)
        road_actor = self.actors.spawned[1]
        self.assertEqual(road_actor.label, "ContinuousPhysicalRoadOnly")
        self.assertIs(road_actor.component.asset, self.road)
        self.assertEqual(road_actor.component.profile, "BlockAll")
        self.assertFalse(road_actor.component.simulating)
        self.assertEqual(self.road.body.flag, 3)
        receipt = json.loads(self.receipt.read_text())
        self.assertEqual(receipt["map_sha256"], self.hash(self.map))
        self.assertEqual(receipt["asset_sha256"], self.hash(self.asset))
        self.assertEqual(receipt["source_obj_sha256"], self.hash(self.obj))
        self.assertEqual(receipt["source_mtl_sha256"], self.hash(self.mtl))
        self.assertFalse(receipt["road_chaos_contact_tested"])
        self.assertEqual(receipt["actual_wheel_qa"], "NOT_RUN")

    def test_bad_original_digest_prevents_any_editor_mutation(self):
        with self.assertRaisesRegex(RuntimeError, "source bytes drifted"):
            self.run_import(environment={
                "LATEBRAKE_CONTINUOUS_ROAD_OBJ_SHA256": "0"*64})
        self.assertFalse(self.level.created)
        self.assertEqual(self.asset_tools.tasks, [])
        self.assertFalse(self.receipt.exists())

    def test_bad_hash_shape_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "Missing canonical"):
            self.run_import(environment={
                "LATEBRAKE_CONTINUOUS_ROAD_MTL_SHA256": "NOT_A_SHA"})
        self.assertFalse(self.level.created)
        self.assertFalse(self.asset.exists())

    def test_existing_map_or_asset_protected(self):
        for field, content in ((self.map, b"USER_MAP"), (self.asset, b"USER_ASSET")):
            with self.subTest(field=field):
                field.parent.mkdir(parents=True, exist_ok=True)
                field.write_bytes(content)
                with self.assertRaisesRegex(RuntimeError, "already exists"):
                    self.run_import()
                self.assertEqual(field.read_bytes(), content)
                self.assertFalse(self.level.created)
                field.unlink()

    def test_unreal_asset_registry_occupied_blocks_import(self):
        self.existing_asset = True
        with self.assertRaisesRegex(RuntimeError, "already occupied"):
            self.run_import()
        self.assertEqual(self.asset_tools.tasks, [])

    def test_wrong_ue_version_is_rejected(self):
        self.engine_version = "5.7.5"
        with self.assertRaisesRegex(RuntimeError, "needs UE 5.8"):
            self.run_import()
        self.assertFalse(self.level.created)

    def test_interchange_wrong_asset_path_does_not_create_level(self):
        self.asset_tools.paths = ["/Game/Another/BadImport.BadImport"]
        with self.assertRaisesRegex(RuntimeError, "unexpected assets"):
            self.run_import()
        self.assertFalse(self.level.created)
        self.assertFalse(self.receipt.exists())

    def test_editor_asset_import_with_wrong_triangles_is_rejected(self):
        self.road.triangles = 8191
        with self.assertRaisesRegex(RuntimeError, "lost original 8192"):
            self.run_import()
        self.assertFalse(self.level.created)
        self.assertFalse(self.receipt.exists())

    def test_importer_wrong_world_scale_is_rejected(self):
        self.road.half = [2800.0, 18000.0, 21.0]
        with self.assertRaisesRegex(RuntimeError, "scale/axis import changed"):
            self.run_import()
        self.assertFalse(self.level.created)

    def test_missing_collision_body_rejected_before_map_creation(self):
        self.road.body = None
        with self.assertRaisesRegex(RuntimeError, "no usable collision BodySetup"):
            self.run_import()
        self.assertFalse(self.level.created)

    def test_saving_asset_failure_prevents_map_or_receipt(self):
        self.can_save_asset = False
        with self.assertRaisesRegex(RuntimeError, "Could not save"):
            self.run_import()
        self.assertFalse(self.level.created)
        self.assertFalse(self.receipt.exists())

    def test_saved_level_must_reload_with_exact_static_blockall(self):
        self.level.on_reload = lambda: setattr(
            self.actors.spawned[1].component, "collision", 0)
        with self.assertRaisesRegex(RuntimeError, "lost STATIC BlockAll"):
            self.run_import()
        self.assertTrue(self.level.reloaded)
        self.assertFalse(self.receipt.exists())

    def test_save_level_failure_rejects_success_receipt(self):
        self.level.can_save = False
        with self.assertRaisesRegex(RuntimeError, "did not save"):
            self.run_import()
        self.assertFalse(self.receipt.exists())


if __name__ == "__main__":
    unittest.main()
