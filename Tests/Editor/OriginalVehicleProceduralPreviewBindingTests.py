import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
PUBLIC = ROOT / "Source" / "Latebrake" / "Public" / "Vehicles"
PRIVATE = ROOT / "Source" / "Latebrake" / "Private" / "Vehicles"

class OriginalVehicleProceduralPreviewBindingTests(unittest.TestCase):
    def test_preview_is_opt_in_and_retains_chaos_chassis(self):
        pawn_h = (PUBLIC / "LatebrakeCarPawn.h").read_text(encoding="utf-8")
        pawn_cpp = (PRIVATE / "LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
        self.assertIn("bEnableOriginalProceduralPreview = false", pawn_h)
        self.assertIn("UPROPERTY(EditDefaultsOnly", pawn_h)
        self.assertIn("UProceduralMeshComponent", pawn_h)
        self.assertIn("RefreshOriginalBodyPreview();", pawn_cpp)
        self.assertIn("Chassis->SetSimulatePhysics(true)", pawn_cpp)
        self.assertIn("OriginalBodyPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision)", pawn_cpp)
        self.assertIn("CreateOriginalVehicleVisualSections(", pawn_cpp)

    def test_bridge_creates_10_cosmetic_sections_no_physics(self):
        bridge = (PRIVATE / "OriginalVehicleUnrealPreviewBridge.cpp").read_text(encoding="utf-8")
        header = (PUBLIC / "OriginalVehicleUnrealPreviewBridge.h").read_text(encoding="utf-8")
        self.assertIn("CreateOriginalVehicleVisualSections", header)
        self.assertIn("CreateMeshSection_LinearColor(", bridge)
        self.assertIn("false, // No runtime collision mesh, ever.", bridge)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::NoCollision)", bridge)
        self.assertIn("SetGenerateOverlapEvents(false)", bridge)
        self.assertIn("OriginalVehicleSurfaceComposer builder", bridge)
        self.assertIn("builder.ValidateMesh", bridge)
        self.assertIn("CreateDynamicMaterialInstance", bridge)
        self.assertNotIn("SetSimulatePhysics(true)", bridge)
        self.assertNotIn("CreateMeshSection_LinearColor(", header)

    def test_native_runtime_dependency_is_declared(self):
        build = (ROOT / "Source/Latebrake/Latebrake.Build.cs").read_text(encoding="utf-8")
        project = (ROOT / "Latebrake.uproject").read_text(encoding="utf-8")
        self.assertIn("ProceduralMeshComponent", build)
        self.assertIn('"Name": "ProceduralMeshComponent"', project)

if __name__ == "__main__":
    unittest.main()
