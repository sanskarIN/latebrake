"""Static source policy checks only, NOT an Unreal 5.8 Editor build.

These bridge files came from the original UE-preview research branch and
must never silently add physics-bearing track collision or change the
shipping driving loop while awaiting a genuine qualified UE workstation.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "Source" / "Latebrake"
PREVIEWS = [
    BASE / "Private/Track/OriginalAsphaltUnrealPreviewBridge.cpp",
    BASE / "Private/Track/OriginalArmcoUnrealPreviewBridge.cpp",
    BASE / "Private/Track/OriginalKerbUnrealPreviewBridge.cpp",
    BASE / "Private/Vehicles/OriginalVehicleUnrealPreviewBridge.cpp",
]
SKID = BASE / "Private/Vehicles/LatebrakeSkidTrailActor.cpp"
SUSPENSION = BASE / "Private/Vehicles/OriginalChaosSuspensionPreview.cpp"
CHASSIS = BASE / "Private/Vehicles/LatebrakeCarPawn.cpp"
GAMEMODE = BASE / "Private/Game/LatebrakeRaceGameMode.cpp"
RULES = BASE / "Latebrake.Build.cs"


class NativePreviewBridgeSourceTests(unittest.TestCase):
    def test_procedural_component_linked_but_not_asserted_engine_ready(self):
        text = RULES.read_text(encoding="utf-8")
        self.assertIn('"ProceduralMeshComponent"', text)
        self.assertIn('"PhysicsCore"', text)
        self.assertIn('"Engine"', text)

    def test_four_procedural_previews_are_noncolliding(self):
        for path in PREVIEWS:
            with self.subTest(path=path.name):
                text = path.read_text(encoding="utf-8")
                self.assertIn("CreateMeshSection_LinearColor", text)
                self.assertIn("ECollisionEnabled::NoCollision", text)
                self.assertIn("SetGenerateOverlapEvents(false)", text)
                self.assertNotIn("SetSimulatePhysics(true)", text)
                self.assertNotIn("SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)", text)
                self.assertNotIn("AddForceAtLocation(", text)
                self.assertNotIn("SetActorLocation(", text)

    def test_visual_sections_are_not_launched_implicitly(self):
        pawn = CHASSIS.read_text(encoding="utf-8")
        game = GAMEMODE.read_text(encoding="utf-8")
        for source in [pawn, game]:
            self.assertNotIn("CreateOriginalVehicleVisualSections(", source)
            self.assertNotIn("SpawnOriginalKerbPreview(", source)
            self.assertNotIn("SpawnOriginalAsphaltPreview(", source)
            self.assertNotIn("SpawnOriginalArmcoPreview(", source)
            self.assertNotIn("ApplyOriginalChaosSprungPreview(", source)

    def test_skid_trail_is_visual_only(self):
        text = SKID.read_text(encoding="utf-8")
        self.assertIn("ECollisionEnabled::NoCollision", text)
        self.assertIn("SetSimulatePhysics(false)", text)
        self.assertIn("SetGenerateOverlapEvents(false)", text)
        self.assertNotIn("SetActorLocation(", text)
        self.assertNotIn("AddForceAtLocation(", text)

    def test_sprung_physics_requires_separate_native_qualification(self):
        text = SUSPENSION.read_text(encoding="utf-8")
        self.assertIn("AddForceAtLocation(", text)
        self.assertIn("GetPhysicsLinearVelocityAtPoint", text)
        self.assertNotIn("ApplyOriginalChaosSprungPreview(", CHASSIS.read_text(encoding="utf-8"))
        self.assertNotIn("ApplyOriginalChaosSprungPreview(", GAMEMODE.read_text(encoding="utf-8"))

    def test_public_bridge_contracts_are_present(self):
        headers = [
            BASE / "Public/Track/OriginalAsphaltUnrealPreviewBridge.h",
            BASE / "Public/Track/OriginalArmcoUnrealPreviewBridge.h",
            BASE / "Public/Track/OriginalKerbUnrealPreviewBridge.h",
            BASE / "Public/Vehicles/OriginalVehicleUnrealPreviewBridge.h",
            BASE / "Public/Vehicles/OriginalChaosSuspensionPreview.h",
            BASE / "Public/Vehicles/LatebrakeSkidTrailActor.h",
            BASE / "Public/Visual/OriginalPreviewMaterials.h",
        ]
        for path in headers:
            with self.subTest(path=path.name):
                text = path.read_text(encoding="utf-8")
                self.assertTrue(text.strip())
                self.assertNotIn("NotImplemented", text)


if __name__ == "__main__":
    unittest.main()
