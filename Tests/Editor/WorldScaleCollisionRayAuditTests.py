"""Portable synthetic QA for the real-Editor-only static collision ray auditor.

All collision responses below are deliberately FAKE. Success here tests
fail-closed audit logic, not Unreal/Chaos physics, cooking or 3D gameplay.
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

SCRIPT = Path(__file__).resolve().parents[2] / "scripts/Unreal/AuditWorldScaleContinuousRoad.py"
TESTS_PATH = Path(__file__).resolve().parents[1] / "Standalone"
sys.path.insert(0, str(TESTS_PATH))
from WorldScaleCollisionTracePlan import make_plan, validate_plan  # noqa: E402


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class FakeVec:
    def __init__(self, x, y, z):
        self.x, self.y, self.z = x, y, z


class FakeComponent:
    def __init__(self):
        self.mesh = None
        self.enabled = 3
        self.profile = "BlockAll"
        self.simulates = False
    def get_collision_enabled(self): return self.enabled
    def get_collision_profile_name(self): return self.profile
    def is_simulating_physics(self): return self.simulates
    def get_editor_property(self, name):
        assert name == "static_mesh"
        return self.mesh


class FakeActor:
    def __init__(self, label, mesh=None):
        self.label = label
        self.comp = FakeComponent()
        self.comp.mesh = mesh
    def get_actor_label(self): return self.label
    def get_editor_property(self, name):
        assert name == "static_mesh_component"
        return self.comp
    def get_actor_enable_collision(self): return True


class FakeMesh:
    def __init__(self):
        self.body = types.SimpleNamespace(
            get_editor_property=lambda name: 4 if name == "collision_trace_flag" else None)
    def get_editor_property(self, name):
        assert name == "body_setup"
        return self.body


class FakeHit:
    def __init__(self, actor, x, y, height, up=1.0, blocking=True):
        self.props = {
            "blocking_hit": blocking,
            "hit_actor": actor,
            "impact_point": FakeVec(x, y, height),
            "impact_normal": FakeVec(0, 0, up),
        }
    def get_editor_property(self, name): return self.props[name]


class FakeEditor:
    def __init__(self):
        self.loaded = False
        self.load_ok = True
    def load_level(self, path):
        assert path == "/Game/Maps/LatebrakeWorldScaleContinuousCollisionLab"
        self.loaded = True
        return self.load_ok


class WorldScaleCollisionRayAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sample_plan = make_plan()

    def setUp(self):
        fixture = tempfile.TemporaryDirectory(prefix="lb-ue58-query-mock-")
        self.addCleanup(fixture.cleanup)
        self.root = Path(fixture.name)
        self.content = self.root / "Content"
        self.saved = self.root / "Saved/LatebrakeAutomation"
        self.map = self.content / "Maps/LatebrakeWorldScaleContinuousCollisionLab.umap"
        self.asset = self.content / "LatebrakeExperimental/SM_WorldScaleContinuousRoad.uasset"
        self.obj = self.saved / "SM_WorldScaleContinuousRoad.obj"
        self.mtl = self.obj.with_suffix(".mtl")
        self.receipt = self.saved / "world-scale-continuous-road-import-result.json"
        self.plan = self.saved / "world-scale-continuous-ray-probes.json"
        self.output = self.saved / "world-scale-continuous-ray-audit-result.json"
        for path, contents in (
            (self.map, b"SYNTHETIC_UMAP_NOT_ENGINE"*30),
            (self.asset, b"SYNTHETIC_UASSET_NOT_ENGINE"*30),
            (self.obj, b"SYNTHETIC_CXX_OBJ_NOT_REAL"*30),
            (self.mtl, b"SYNTHETIC_CXX_MTL_NOT_REAL"*30),
        ):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(contents)
        self.plan.write_text(json.dumps(self.sample_plan, sort_keys=True,
                                        separators=(",", ":")) + "\n", encoding="utf-8")
        self.receipt.write_text(json.dumps({
            "schema_version": 1,
            "status": "saved_and_reloaded_by_unreal_editor",
            "engine_version": "5.8",
            "road_mesh": "/Game/LatebrakeExperimental/SM_WorldScaleContinuousRoad",
            "map": "/Game/Maps/LatebrakeWorldScaleContinuousCollisionLab",
            "actors_audited_after_reload": 2,
            "physical_road_width_cm": 900,
            "collision_trace_flag": "CTF_USE_COMPLEX_AS_SIMPLE",
            "collision_profile": "BlockAll",
            "static_road": True, "race_game_mode_disabled": True,
            "road_chaos_contact_tested": False, "actual_wheel_qa": "NOT_RUN",
            "map_sha256": sha(self.map), "map_bytes": self.map.stat().st_size,
            "asset_sha256": sha(self.asset), "asset_bytes": self.asset.stat().st_size,
            "source_obj_sha256": sha(self.obj), "source_mtl_sha256": sha(self.mtl),
        }), encoding="utf-8")
        self.road_mesh = FakeMesh()
        self.road_actor = FakeActor("ContinuousPhysicalRoadOnly", self.road_mesh)
        self.ground_actor = FakeActor("ContinuousCollisionLabGround")
        self.actors = [self.ground_actor, self.road_actor]
        self.editor = FakeEditor()
        self.version = "5.8.1"
        self.world_mode = True
        self.ray_hits = 0
        self.fail_at = None
        self.failure_mode = None
        self.sample_by_xy = {
            (p["x_cm"], p["y_cm"]): p for p in self.sample_plan["samples"]
        }
        self.logs = []

    def run_script(self, *, plan_sha=None):
        fake = types.ModuleType("unreal")
        fake.Paths = types.SimpleNamespace(
            project_content_dir=lambda: str(self.content),
            project_saved_dir=lambda: str(self.root / "Saved"))
        fake.Vector = FakeVec
        fake.SystemLibrary = types.SimpleNamespace(
            get_engine_version=lambda: self.version,
            line_trace_single=self.fake_trace)
        fake.TraceTypeQuery = types.SimpleNamespace(TRACE_TYPE_QUERY1=1)
        fake.DrawDebugTrace = types.SimpleNamespace(NONE=0)
        fake.CollisionEnabled = types.SimpleNamespace(QUERY_AND_PHYSICS=3)
        fake.CollisionTraceFlag = types.SimpleNamespace(
            CTF_USE_COMPLEX_AS_SIMPLE=4)
        fake.LevelEditorSubsystem = type("LevelEditorSubsystem", (), {})
        fake.EditorActorSubsystem = type("EditorActorSubsystem", (), {})
        fake.UnrealEditorSubsystem = type("UnrealEditorSubsystem", (), {})
        fake.GameModeBase = type("GameModeBase", (), {
            "static_class": classmethod(lambda cls: cls)})
        fake.load_asset = lambda name: (
            self.road_mesh
            if name == "/Game/LatebrakeExperimental/SM_WorldScaleContinuousRoad"
            else None
        )
        fake.get_editor_subsystem = lambda cls: {
            fake.LevelEditorSubsystem: self.editor,
            fake.EditorActorSubsystem: types.SimpleNamespace(
                get_all_level_actors=lambda: self.actors),
            fake.UnrealEditorSubsystem: types.SimpleNamespace(
                get_editor_world=lambda: types.SimpleNamespace(
                    get_world_settings=lambda: types.SimpleNamespace(
                        get_editor_property=lambda name: fake.GameModeBase
                        if self.world_mode and name == "default_game_mode"
                        else None))),
        }[cls]
        fake.log = self.logs.append
        old_runpy = runpy.run_path
        def intercept(path, *args, **kwargs):
            if str(path).endswith("WorldScaleCollisionTracePlan.py"):
                return {"validate_plan": validate_plan}
            return old_runpy(path, *args, **kwargs)
        environment = {
            "LATEBRAKE_CONTINUOUS_RAY_PLAN_SHA256":
                sha(self.plan) if plan_sha is None else plan_sha,
        }
        with (patch.dict(sys.modules, {"unreal": fake}),
              patch.dict(os.environ, environment),
              patch.object(runpy, "run_path", side_effect=intercept)):
            runpy.run_path(str(SCRIPT), run_name="__main__")

    def fake_trace(self, world, start, end, channel, complex_hit, ignored,
                   debug, ignore_self):
        self.assertEqual(channel, 1)
        self.assertTrue(complex_hit)
        self.assertEqual(debug, 0)
        self.assertEqual((start.z, end.z), (250.0, -200.0))
        self.assertEqual(ignored, [])
        self.assertTrue(ignore_self)
        p = self.sample_by_xy.get((start.x, start.y))
        self.assertIsNotNone(p)
        self.ray_hits += 1
        if self.fail_at == self.ray_hits and self.failure_mode == "missing":
            return None
        if self.fail_at == self.ray_hits and self.failure_mode == "wrong_actor":
            actor = self.ground_actor if p["expected_actor"] == "road" else self.road_actor
        else:
            actor = self.road_actor if p["expected_actor"] == "road" else self.ground_actor
        z = p["expected_z_cm"]
        n = 1.0
        if self.fail_at == self.ray_hits and self.failure_mode == "height":
            z += 12.0
        if self.fail_at == self.ray_hits and self.failure_mode == "normal":
            n = 0.65
        if self.fail_at == self.ray_hits and self.failure_mode == "bad_coord":
            start = FakeVec(start.x + 20.0, start.y, start.z)
        return FakeHit(actor, start.x, start.y, z, n)

    def test_exact_2322_real_query_calls_required_for_success_receipt(self):
        self.run_script()
        self.assertTrue(self.editor.loaded)
        self.assertEqual(self.ray_hits, 2322)
        data = json.loads(self.output.read_text())
        self.assertEqual(data["actual_road_hits"], 1290)
        self.assertEqual(data["actual_ground_hits"], 1032)
        self.assertEqual(data["ray_queries_performed"], 2322)
        self.assertTrue(data["query_complex"])
        self.assertEqual(data["map_sha256"], sha(self.map))
        self.assertEqual(data["asset_sha256"], sha(self.asset))
        self.assertEqual(data["query_plan_sha256"], sha(self.plan))
        self.assertFalse(data["runtime_track_enabled"])
        self.assertFalse(data["dynamic_chaos_wheel_contact_tested"])
        self.assertFalse(data["playable_game_tested"])

    def test_no_fake_success_if_any_real_ray_misses_or_hits_wrong_actor(self):
        for problem in ("missing", "wrong_actor", "height", "normal", "bad_coord"):
            with self.subTest(problem=problem):
                self.fail_at = 2322
                self.failure_mode = problem
                self.ray_hits = 0
                with self.assertRaisesRegex(RuntimeError, "Real Editor collision trace failed"):
                    self.run_script()
                self.assertEqual(self.ray_hits, 2322)
                self.assertFalse(self.output.exists())

    def test_near_edge_wrong_actor_is_not_accepted(self):
        # Fifth and sixth ray calls straddle the negative physical rim.
        # A near-edge false-ground and false-road result must both fail.
        for ray_number in (4, 6):
            with self.subTest(ray_number=ray_number):
                self.ray_hits = 0
                self.fail_at = ray_number
                self.failure_mode = "wrong_actor"
                with self.assertRaisesRegex(
                    RuntimeError, "Real Editor collision trace failed"
                ):
                    self.run_script()
                self.assertEqual(self.ray_hits, ray_number)
                self.assertFalse(self.output.exists())

    def test_hash_drift_rejected_before_any_query(self):
        self.obj.write_bytes(self.obj.read_bytes() + b"tampered!")
        with self.assertRaisesRegex(RuntimeError, "source bytes changed"):
            self.run_script()
        self.assertFalse(self.editor.loaded)
        self.assertEqual(self.ray_hits, 0)

    def test_missing_or_malformed_plan_sha_fails_before_editor(self):
        for value in ("", "A"*64, "x"*64, "f"*63):
            with self.subTest(value=value), self.assertRaisesRegex(
                RuntimeError, "Missing 64-hex canonical"
            ):
                self.run_script(plan_sha=value)
        self.assertFalse(self.editor.loaded)

    def test_rehashed_but_wrong_numeric_probe_plan_is_rejected(self):
        data = json.loads(self.plan.read_text())
        data["samples"][400]["x_cm"] += 11
        self.plan.write_text(json.dumps(data), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "Displaced continuous collision probe"):
            self.run_script()
        self.assertEqual(self.ray_hits, 0)

    def test_existing_query_evidence_never_overwritten(self):
        self.output.write_bytes(b"OWNER_QA_DO_NOT_OVERWRITE"*20)
        with self.assertRaisesRegex(RuntimeError, "Existing ray-query receipt"):
            self.run_script()
        self.assertEqual(self.output.read_bytes(), b"OWNER_QA_DO_NOT_OVERWRITE"*20)
        self.assertFalse(self.editor.loaded)

    def test_wrong_unreal_version_or_game_mode_rejected(self):
        self.version = "5.7.1"
        with self.assertRaisesRegex(RuntimeError, "UE5.8 Editor"):
            self.run_script()
        self.assertFalse(self.editor.loaded)
        self.version = "5.8.1"
        self.world_mode = False
        with self.assertRaisesRegex(RuntimeError, "no longer isolated"):
            self.run_script()
        self.assertEqual(self.ray_hits, 0)
        self.assertFalse(self.output.exists())

    def test_editor_missing_road_or_wrong_collision_settings_rejected(self):
        for problem in ("missing_actor", "no_collision", "not_complex"):
            with self.subTest(problem=problem):
                self.actors = [self.ground_actor] if problem == "missing_actor" else [
                    self.ground_actor, self.road_actor
                ]
                self.road_actor.comp.enabled = 0 if problem == "no_collision" else 3
                if problem == "not_complex":
                    self.road_mesh.body = types.SimpleNamespace(
                        get_editor_property=lambda name: 2)
                else:
                    self.road_mesh.body = types.SimpleNamespace(
                        get_editor_property=lambda name: 4)
                with self.assertRaises(RuntimeError):
                    self.run_script()
                self.assertEqual(self.ray_hits, 0)
                self.assertFalse(self.output.exists())

    def test_editing_map_after_import_rejects_qa(self):
        self.map.write_bytes(self.map.read_bytes() + b"editor_drift")
        with self.assertRaisesRegex(RuntimeError, "drifted since import"):
            self.run_script()
        self.assertEqual(self.ray_hits, 0)


if __name__ == "__main__":
    unittest.main()
