"""Source-only regression: real preview renderers must request PBR assets.

Creating actual material assets, proving render pixels, and packaging them
requires Unreal Editor 5.8. This does not replace that native validation.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"Source"/"Latebrake"/"Private"

class OriginalPreviewMaterialBridgeTests(unittest.TestCase):
    def read(self,path):
        return (SOURCE/path).read_text(encoding="utf-8")

    def test_asphalt_and_curbs_use_road_vertex_colors(self):
        for path in (
            "Track/OriginalAsphaltUnrealPreviewBridge.cpp",
            "Track/OriginalKerbUnrealPreviewBridge.cpp",
        ):
            content=self.read(path)
            self.assertIn('#include "Visual/OriginalPreviewMaterials.h"',content)
            self.assertIn("FindOriginalPreviewMaterial(",content)
            self.assertIn("OriginalPreviewMaterialKind::Road",content)
            self.assertIn("CreateMeshSection_LinearColor",content)
            self.assertIn("ECollisionEnabled::NoCollision",content)

    def test_armco_uses_stainless_visual_material_only(self):
        source=self.read("Track/OriginalArmcoUnrealPreviewBridge.cpp")
        self.assertIn("OriginalPreviewMaterialKind::Steel",source)
        self.assertIn("FindOriginalPreviewMaterial(",source)
        self.assertIn("CreateMeshSection_LinearColor(",source)
        self.assertIn("ECollisionEnabled::NoCollision",source)
        self.assertNotIn("SetSimulatePhysics(true)",source)

    def test_six_car_render_sections_select_real_material_groups(self):
        source=self.read("Vehicles/OriginalVehicleUnrealPreviewBridge.cpp")
        for name in ("CarPaint","Glass","Rubber","Steel","Emissive"):
            self.assertIn("OriginalPreviewMaterialKind::"+name,source)
        self.assertIn("OriginalVehiclePreviewMaterial(",source)
        self.assertIn("FindOriginalPreviewMaterial(",source)
        self.assertIn("CreateMeshSection_LinearColor(",source)
        self.assertIn("ECollisionEnabled::NoCollision",source)

if __name__=="__main__":
    unittest.main()
