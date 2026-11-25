"""Actually committed source rim identity is visible in Development Settings.

The shown identity is the successful source mesh's authored RimStyle, not
merely the user's requested GarageLook. GPU pixels still need UE QA.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class UnrealCommittedRimIdentityBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
    def test_failed_or_legacy_mesh_never_claims_an_authored_rim(self):
        build=self.pawn.split(
            "void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1
        )[1].split("int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts()",1)[0]
        self.assertIn("OriginalRenderedRimStyle = -1;",build)
        self.assertIn("OriginalRenderedRimStyle = Packed.RimStyle;",build)
        self.assertIn("Prepared->Packed()",build)
        cache=read("Source/Latebrake/Public/Vehicles/OriginalVehicleRenderCache.h")
        indexed=read("Source/Latebrake/Public/Vehicles/OriginalVehicleIndexedRender.h")
        self.assertIn("packed_.RimStyle==key_.RimStyle",cache)
        self.assertIn("out.RimStyle=mesh.RimStyle",indexed)
        self.assertLess(build.index("CanPresentOriginalVehicleVisual("),
                        build.index("OriginalRenderedRimStyle = Packed.RimStyle;"))
        self.assertLess(build.index("RenderParts[i]->SetVisibility(true, false);"),
                        build.index("OriginalRenderedRimStyle = Packed.RimStyle;"))
        self.assertIn("bOriginalVehicleVisualReady ? OriginalRenderedRimStyle : -1",self.header)
    def test_qualified_report_uses_actual_source_not_view_cursor(self):
        settings=self.hud.split("case latebrake::MenuPage::Settings:",1
        )[1].split("case latebrake::MenuPage::Profile:",1)[0]
        self.assertIn("LIVE 3D RIM:",settings)
        self.assertIn("Car->GetCommittedOriginalRimStyle()",settings)
        self.assertIn("GarageLookChoiceName(",settings)
        self.assertIn("FALLBACK / NO SOURCE MESH",settings)
        self.assertNotIn("GarageGallery.Viewed()",settings)
        self.assertIn("GarageLooks.Get(Garage.SelectedVehicle())",self.mode)
if __name__=="__main__":
    unittest.main()
