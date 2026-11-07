"""Ensure C++ geometry, Editor lab, wrapper, and source-only QA stay aligned."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RoundedCircuitLabBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.exporter = read("Tests/Standalone/RoundedCircuitLabExporter.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.editor = read("scripts/Unreal/CreateRoundedCircuitLab.py")
        cls.wrapper = read("scripts/New-RoundedCircuitLab.ps1")
        cls.game = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.docs = read("docs/ROUNDED_CIRCUIT_PLAN_QA.md")

    def test_portable_exporter_uses_single_physical_source_of_truth(self):
        for key in (
            '#include "Track/RoundedCircuitRoadPlan.h"',
            "RoundedPhysicalRoadCount",
            "PlannedRoundedPhysicalRoadPiece(i)",
            "IsValidRoundedPhysicalRoadPiece(road)",
            "std::locale::classic()",
            '"piece_count"',
        ):
            # C++ escaping is allowed for the JSON keys.
            if key == '"piece_count"':
                self.assertIn("piece_count", self.exporter)
            else:
                self.assertIn(key, self.exporter)
        self.assertIn("RoundedCircuitLabExporter.cpp", self.cmake)
        self.assertIn("NAME RoundedCircuitLabExport", self.cmake)

    def test_genuine_separate_editor_map_cannot_start_race(self):
        for required in (
            'LAB_PACKAGE = "/Game/Maps/LatebrakeRoundedPhysicsLab"',
            "Existing rounded lab .umap must be reviewed",
            "read_verified_geometry(GEOMETRY_FILE)",
            'settings.set_editor_property("default_game_mode", unreal.GameModeBase)',
            "unreal.StaticMeshActor",
            "unreal.CollisionEnabled.QUERY_AND_PHYSICS",
            "subsystem.save_current_level()",
            "geometry_sha256",
            "map_sha256",
            "RoundedPhysicsLabGround",
        ):
            self.assertIn(required, self.editor)
        self.assertNotIn("latebrake.RaceGameMode", self.editor)
        self.assertNotIn("RoundedCircuitRoadPlan.h", self.game)
        self.assertNotIn("PlannedRoundedPhysicalRoadPiece(", self.game)

    def test_shell_builds_exporter_and_verifies_real_receipt(self):
        for required in (
            "latebrake_rounded_circuit_lab_exporter",
            "UnrealEditor.exe",
            "EngineAssociation -cne '5.8'",
            "Get-FileHash",
            "expectedDataHash",
            "Verify-UnrealMapHeader.ps1",
            "Test-Path -LiteralPath $map -PathType Any",
        ):
            self.assertIn(required, self.wrapper)
        self.assertIn(
            "unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)", self.editor
        )
        self.assertIn("audit_saved_physics_lab(", self.editor)
        self.assertIn("subsystem.load_level(LAB_PACKAGE)", self.editor)
        self.assertIn('"verified_after_reload": True', self.editor)
        self.assertIn("audited_actors", self.editor)
        self.assertIn("verified_after_reload", self.wrapper)
        for required in (
            "NOT been executed in a licensed Unreal Editor",
            "never overwrites an existing lab map",
            "68 BlockAll",
        ):
            self.assertIn(required, self.docs)


if __name__ == "__main__":
    unittest.main()
