"""Source render adapter contracts; no claim of Unreal GPU validation."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class OriginalVehicleSurfaceShadingBindingTests(unittest.TestCase):
    def test_native_adapter_uses_actual_generated_frames(self):
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        rebuild=pawn.split("void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1)[1].split(
            "int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts()",1)[0]
        self.assertLess(rebuild.index("BuildOriginalSurfaceShading(generated.Mesh)"),
                        rebuild.index("Part->CreateMeshSection("))
        for marker in ("BuildOriginalIndexedRender(", "vertex.Surface", "Surface.Normal.X",
                       "Tangents.Add(FProcMeshTangent(", "Surface.FlipTangentY",
                       "Tangents, false /* no collision, ever */",
                       "GarageCameraGeometry.Build(generated.Mesh);"):
            self.assertIn(marker,rebuild)
        self.assertNotIn("Normals.Add(FVector(vertex.Normal.X",rebuild)

    def test_obj_uses_same_frames_and_policy_does_not_change_mesh(self):
        obj=read("Source/Latebrake/Public/Vehicles/OriginalVehicleObjPreview.h")
        self.assertIn("BuildOriginalSurfaceShading(mesh)",obj)
        self.assertIn("for(const auto& frame:shading.Frames)",obj)
        policy=read("Source/Latebrake/Public/Vehicles/OriginalVehicleSurfaceShading.h")
        for marker in ("const OriginalVehicleMesh& mesh", "a.Wheel==b.Wheel",
                       "a.Material==b.Material", "count!=2u",
                       "frame.CornerAngle[c]", "SurfaceDot(out.Normal,out.Tangent)"):
            self.assertIn(marker,policy)
        self.assertIn("add_latebrake_portable_research_test(OriginalVehicleSurfaceShading)",
                      read("Tests/Standalone/CMakeLists.txt"))

if __name__=="__main__":unittest.main()
