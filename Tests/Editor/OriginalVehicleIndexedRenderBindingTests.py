"""Indexed upload source contracts, not native UE rendering evidence."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")
class OriginalVehicleIndexedRenderBindingTests(unittest.TestCase):
    def test_native_packed_geometry_upload_preserves_original_source(self):
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        rebuild=pawn.split("void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1)[1].split(
            "int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts()",1)[0]
        self.assertLess(rebuild.index("BuildOriginalIndexedRender("),
                        rebuild.index("Part->CreateMeshSection("))
        for mark in ("generated.Mesh, shading, animated", "Packed.IndexCount",
                     "Packed.Parts[partIndex]", "Data.Vertices.size()",
                     "Data.Indices.size()", "vertex.Surface", "Data.Indices)",
                     "Indices.Add(static_cast<int32>(Index))",
                     "Tangents, false /* no collision, ever */",
                     "GarageCameraGeometry.Build(generated.Mesh);"):
            self.assertIn(mark,rebuild)
        self.assertNotIn("Indices.Add(next)",rebuild)
        self.assertNotIn("Positions.Num()",rebuild)
        policy=read("Source/Latebrake/Public/Vehicles/OriginalVehicleIndexedRender.h")
        for mark in ("IndexedVertexKey", "SameIndexedVertex", "seen[id]!=0u",
                     "OriginalWheelPivot", "ValidateOriginalIndexedRender(",
                     "s.Indices[cursor++]", "if(!SameIndexedVertex"):
            self.assertIn(mark,policy)
        self.assertIn("add_latebrake_portable_research_test(OriginalVehicleIndexedRender)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
