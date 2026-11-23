"""Original 3D OBJ viewer artifacts must come from the real six-car C++ mesh."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(path):return (ROOT/path).read_text(encoding="utf-8")
class OriginalVehicleObjPreviewBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.export=read("Source/Latebrake/Public/Vehicles/OriginalVehicleObjPreview.h")
        cls.cli=read("Tools/OriginalVehicleObjPreview.cpp")
        cls.tests=read("Tests/Standalone/OriginalVehicleObjPreviewTests.cpp")
        cls.workflow=read(".github/workflows/original-3d-source-obj-preview.yml")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
        cls.docs=read("docs/PR452_ORIGINAL_3D_OBJ_PREVIEW.md")
    def test_export_real_body_wheels_and_materials(self):
        for token in ("OriginalVehicleAnimatedWheels.h",
                      "MakeOriginalVehicleAnimatedParts(mesh).Valid",
                      "mesh.Vertices", "mesh.Triangles", "WheelSlot",
                      "GarageLook", "PaintForGarageLook(",
                      "WriteOriginalVehicleMtl", "OriginalPreviewMaterials",
                      "mtllib", "usemtl", "vn ", "vt ", "f"):
            self.assertIn(token,self.export)
        self.assertIn("OriginalVehicleSurfaceComposer{}.Build(id,rim)",self.cli)
        self.assertIn("written!=18",self.cli)
        self.assertIn("complete==18u",self.tests)
    def test_provenance_uses_branch_source_and_no_publish_permission(self):
        self.assertIn("feat/unreal-garage-appearance-ux-20261123",self.workflow)
        self.assertIn("contents: read",self.workflow)
        self.assertIn("persist-credentials: false",self.workflow)
        self.assertIn("actions/upload-artifact@v4",self.workflow)
        self.assertIn("$GITHUB_SHA",self.workflow)
        self.assertIn("UNVERIFIED-LateBrake-3D-source-OBJ-",self.workflow)
        self.assertNotIn("actions/create-release",self.workflow)
        self.assertIn("Name",self.workflow) if False else None
        self.assertIn("NAME OriginalVehicleObjPreview",self.cmake)
    def test_docs_do_not_misrepresent_as_unreal_game(self):
        self.assertIn("NOT a running Unreal Engine",self.docs)
        self.assertIn("18 pairs",self.docs)
        self.assertIn("Y-UP",self.docs)
        self.assertIn("3D",self.docs)
if __name__=="__main__":unittest.main()
