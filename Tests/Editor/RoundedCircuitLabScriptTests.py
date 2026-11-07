"""Mock-only UE5.8 Editor lab contracts; does NOT generate a genuine map."""
import copy
import hashlib
import json
import runpy
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[2] / "scripts" / "Unreal" / "CreateRoundedCircuitLab.py"


class FakeSettings:
    mode = None
    def set_editor_property(self, key, value):
        assert key == "default_game_mode"
        self.mode = value
    def get_editor_property(self, key):
        assert key == "default_game_mode"
        return self.mode


class FakeMesh:
    def __init__(self):
        self.asset = None
        self.collision = None
        self.profile = None
    def set_static_mesh(self, asset): self.asset = asset
    def set_collision_profile_name(self, profile): self.profile = profile
    def set_collision_enabled(self, collision): self.collision = collision


class FakeActor:
    def __init__(self):
        self.mesh = FakeMesh()
        self.label = None
        self.scale = None
    def get_editor_property(self, key):
        assert key == "static_mesh_component"
        return self.mesh
    def set_actor_scale3d(self, scale): self.scale = scale
    def set_actor_label(self, name): self.label = name


class FakeEditor:
    def __init__(self, path):
        self.path = path
        self.creates = 0
        self.saves = 0
        self.can_save = True
    def new_level(self, name, partitioned):
        assert name == "/Game/Maps/LatebrakeRoundedPhysicsLab"
        assert partitioned is False
        self.creates += 1
        return True
    def save_current_level(self):
        self.saves += 1
        if not self.can_save: return False
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_bytes(b"TEST_ONLY_NON_UNREAL_LEVEL" * 200)
        return True


class FakeActors:
    def __init__(self):
        self.spawned = []
        self.fail_at = None
    def spawn_actor_from_class(self, actor_class, location, rotation, transient):
        if self.fail_at is not None and len(self.spawned) == self.fail_at:
            return None
        actor = FakeActor()
        self.spawned.append((actor, location, rotation, transient))
        return actor


class RoundedCircuitLabScriptTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory(prefix="lb-rounded-fake-")
        self.addCleanup(directory.cleanup)
        root = Path(directory.name)
        self.content = root / "Content"
        self.saved = root / "Saved"
        self.map = self.content / "Maps" / "LatebrakeRoundedPhysicsLab.umap"
        self.geometry = self.saved / "LatebrakeAutomation" / "rounded-road-pieces.json"
        self.receipt = self.saved / "LatebrakeAutomation" / "rounded-physics-lab-result.json"
        self.editor = FakeEditor(self.map)
        self.actors = FakeActors()
        self.settings = FakeSettings()
        self.version = "5.8.0-SYNTHETIC_TEST"
        self.events = []
        self.data = {
            "schema_version": 1, "units": "cm", "road_top_cm": -1.0,
            "piece_count": 68, "pieces": [
                {"index": i, "position": [float(i-34), 150.0, -22.0],
                 "rotation_yaw": 0.0, "scale": [1.35, 9.0, 0.42]}
                for i in range(68)
            ]
        }
        self.write_data()

    def write_data(self):
        self.geometry.parent.mkdir(parents=True, exist_ok=True)
        self.geometry.write_text(json.dumps(self.data), encoding="utf-8")

    def run_script(self):
        fake = types.ModuleType("unreal")
        fake.Paths = types.SimpleNamespace(
            project_content_dir=lambda: str(self.content),
            project_saved_dir=lambda: str(self.saved))
        fake.SystemLibrary = types.SimpleNamespace(get_engine_version=lambda: self.version)
        fake.LevelEditorSubsystem = type("LevelEditorSubsystem", (), {})
        fake.EditorActorSubsystem = type("EditorActorSubsystem", (), {})
        fake.UnrealEditorSubsystem = type("UnrealEditorSubsystem", (), {})
        fake.StaticMeshActor = type("StaticMeshActor", (), {})
        fake.StaticMesh = type("StaticMesh", (), {})
        fake.GameModeBase = type("GameModeBase", (), {})
        fake.CollisionEnabled = types.SimpleNamespace(QUERY_AND_PHYSICS=3)
        fake.Vector = lambda *args: tuple(args)
        fake.Rotator = lambda *args: tuple(args)
        cube = fake.StaticMesh()
        fake.load_asset = lambda path: cube if path == "/Engine/BasicShapes/Cube.Cube" else None
        fake.get_editor_subsystem = lambda cls: {
            fake.LevelEditorSubsystem: self.editor,
            fake.EditorActorSubsystem: self.actors,
            fake.UnrealEditorSubsystem: types.SimpleNamespace(
                get_editor_world=lambda: types.SimpleNamespace(
                    get_world_settings=lambda: self.settings)),
        }[cls]
        fake.log = self.events.append
        with patch.dict(sys.modules, {"unreal": fake}):
            runpy.run_path(str(SCRIPT), run_name="__main__")

    def test_success_is_separate_from_racing_map_and_hashes_real_editor_calls(self):
        self.run_script()
        self.assertEqual(self.editor.creates, 1)
        self.assertEqual(self.editor.saves, 1)
        self.assertEqual(len(self.actors.spawned), 69)
        ground = self.actors.spawned[0][0]
        self.assertEqual(ground.label, "RoundedPhysicsLabGround")
        self.assertEqual(ground.scale, (110.0, 85.0, 1.25))
        for i, (actor, pos, angle, transient) in enumerate(self.actors.spawned[1:]):
            self.assertEqual(actor.label, f"RoundedPhysicsRoad_{i:02d}")
            self.assertEqual(actor.mesh.profile, "BlockAll")
            self.assertEqual(actor.mesh.collision, 3)
            self.assertEqual(actor.scale, (1.35, 9.0, 0.42))
            self.assertEqual(pos[2], -22.0)
            self.assertEqual(angle[1], 0.0)
            self.assertFalse(transient)
        self.assertFalse((self.content / "Maps" / "LatebrakeCircuit.umap").exists())
        receipt = json.loads(self.receipt.read_text(encoding="utf-8"))
        self.assertEqual(receipt["pieces"], 68)
        self.assertEqual(receipt["ground_pieces"], 1)
        self.assertEqual(receipt["game_mode"], "GameModeBase")
        self.assertEqual(receipt["status"], "saved_by_unreal_editor")
        self.assertEqual(receipt["geometry_sha256"], hashlib.sha256(self.geometry.read_bytes()).hexdigest())
        self.assertEqual(receipt["map_sha256"], hashlib.sha256(self.map.read_bytes()).hexdigest())

    def test_existing_user_map_is_never_overwritten(self):
        self.map.parent.mkdir(parents=True)
        self.map.write_bytes(b"OWNER_MAP_MUST_SURVIVE")
        with self.assertRaisesRegex(RuntimeError, "Existing rounded lab"):
            self.run_script()
        self.assertEqual(self.editor.creates, 0)
        self.assertEqual(self.map.read_bytes(), b"OWNER_MAP_MUST_SURVIVE")
        self.assertFalse(self.receipt.exists())

    def test_wrong_unreal_version_fails_before_map_creation(self):
        self.version = "5.7.0"
        with self.assertRaisesRegex(RuntimeError, "Editor 5.8"):
            self.run_script()
        self.assertEqual(self.editor.creates, 0)
        self.assertFalse(self.receipt.exists())

    def test_bad_source_json_never_creates_any_map(self):
        for mutation in ("missing", "reordered", "duplicate", "units", "missing_scale", "boolean_index"):
            with self.subTest(mutation=mutation):
                old = copy.deepcopy(self.data)
                if mutation == "missing": self.data["pieces"].pop()
                if mutation == "reordered": self.data["pieces"][0]["index"] = 1
                if mutation == "duplicate": self.data["pieces"][4] = self.data["pieces"][3].copy()
                if mutation == "units": self.data["units"] = "meters"
                if mutation == "missing_scale": self.data["pieces"][5].pop("scale")
                if mutation == "boolean_index": self.data["pieces"][6]["index"] = True
                self.write_data()
                with self.assertRaises(RuntimeError): self.run_script()
                self.assertEqual(self.editor.creates, 0)
                self.assertFalse(self.receipt.exists())
                self.data = old

    def test_nonfinite_wrong_height_or_width_rejected_before_authoring(self):
        cases = ("nan", "inf", "bool", "negative", "wide", "height", "yaw")
        for case in cases:
            with self.subTest(case=case):
                old = copy.deepcopy(self.data)
                piece = self.data["pieces"][7]
                if case == "nan": piece["scale"][0] = float("nan")
                if case == "inf": piece["scale"][0] = float("inf")
                if case == "bool": piece["scale"][0] = True
                if case == "negative": piece["scale"][0] = -1.0
                if case == "wide": piece["scale"][1] = 99.0
                if case == "height": piece["position"][2] = -30.0
                if case == "yaw": piece["rotation_yaw"] = 500.0
                self.write_data()
                with self.assertRaises(RuntimeError): self.run_script()
                self.assertEqual(self.editor.creates, 0)
                self.assertFalse(self.receipt.exists())
                self.data = old

    def test_failed_actor_spawn_cannot_write_success_receipt(self):
        self.actors.fail_at = 18  # 1 ground + 17 road slabs
        with self.assertRaisesRegex(RuntimeError, "slab 17"):
            self.run_script()
        self.assertEqual(self.editor.saves, 0)
        self.assertFalse(self.receipt.exists())

    def test_failed_unreal_save_never_produces_success_receipt(self):
        self.editor.can_save = False
        with self.assertRaisesRegex(RuntimeError, "failed to save"):
            self.run_script()
        self.assertFalse(self.receipt.exists())


if __name__ == "__main__":
    unittest.main()
