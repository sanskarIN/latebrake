"""Unit tests for Unreal editor-script control flow, NOT real UE or .umap tests.

The synthetic short-lived fixture deliberately uses fake bytes in a disposable
temp directory. No fixture is playable, reusable or committed to the repo.
Only a licensed Unreal Editor can create an authentic .umap.
"""

import runpy
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[2] / "scripts" / "Unreal" / "CreateStarterMap.py"


class FakeLevelEditor:
    def __init__(self, path: Path):
        self.path = path
        self.load_count = 0
        self.new_count = 0
        self.save_count = 0
        self.fail_load = False
        self.fail_new = False
        self.fail_save = False
        self.write_tiny = False

    def load_level(self, name):
        assert name == "/Game/Maps/LatebrakeCircuit"
        self.load_count += 1
        return not self.fail_load

    def new_level(self, name, is_partitioned_world):
        assert name == "/Game/Maps/LatebrakeCircuit"
        assert is_partitioned_world is False
        self.new_count += 1
        if self.fail_new:
            return False
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_bytes(
            b"TEST_ONLY_NOT_REAL_UNREAL_ASSET" *
            (1 if self.write_tiny else 100)
        )
        return True

    def save_current_level(self):
        self.save_count += 1
        return not self.fail_save


class UnrealEditorMapScriptTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="latebrake-map-test-")
        self.addCleanup(self.directory.cleanup)
        self.content = Path(self.directory.name)
        self.map = self.content / "Maps" / "LatebrakeCircuit.umap"
        self.engine = FakeLevelEditor(self.map)
        self.events = []

    def run_script(self, subsystem=True):
        fake = types.ModuleType("unreal")
        fake.Paths = types.SimpleNamespace(project_content_dir=lambda: str(self.content))
        fake.LevelEditorSubsystem = type("LevelEditorSubsystem", (), {})
        fake.get_editor_subsystem = lambda cls: self.engine if subsystem else None
        fake.log = self.events.append
        with patch.dict(sys.modules, {"unreal": fake}):
            runpy.run_path(str(SCRIPT), run_name="__main__")

    def test_new_level_is_created_and_saved_by_engine_interface(self):
        self.run_script()
        self.assertEqual(self.engine.new_count, 1)
        self.assertEqual(self.engine.load_count, 0)
        self.assertEqual(self.engine.save_count, 1)
        self.assertTrue(any("created" in text for text in self.events))

    def test_existing_map_is_loaded_without_recreating(self):
        self.map.parent.mkdir(parents=True)
        payload = b"TEST_ONLY_UNREAL_MAP_FIXTURE" * 100
        self.map.write_bytes(payload)
        self.run_script()
        self.assertEqual(self.engine.new_count, 0)
        self.assertEqual(self.engine.load_count, 1)
        self.assertEqual(self.engine.save_count, 1)
        self.assertEqual(self.map.read_bytes(), payload)
        self.assertTrue(any("loaded" in text for text in self.events))

    def test_failure_to_load_existing_map_is_fatal(self):
        self.map.parent.mkdir(parents=True)
        self.map.write_bytes(b"TEST_ONLY_FIXTURE" * 100)
        self.engine.fail_load = True
        with self.assertRaisesRegex(RuntimeError, "could not be loaded"):
            self.run_script()
        self.assertEqual(self.engine.new_count, 0)

    def test_failure_to_create_map_is_fatal(self):
        self.engine.fail_new = True
        with self.assertRaisesRegex(RuntimeError, "create/save"):
            self.run_script()
        self.assertEqual(self.engine.save_count, 0)

    def test_failure_to_save_map_is_fatal(self):
        self.engine.fail_save = True
        with self.assertRaisesRegex(RuntimeError, "save the current"):
            self.run_script()

    def test_tiny_asset_is_rejected(self):
        self.engine.write_tiny = True
        with self.assertRaisesRegex(RuntimeError, "unexpectedly small"):
            self.run_script()

    def test_editor_subsystem_is_required(self):
        with self.assertRaisesRegex(RuntimeError, "unavailable"):
            self.run_script(subsystem=False)


if __name__ == "__main__":
    unittest.main()
