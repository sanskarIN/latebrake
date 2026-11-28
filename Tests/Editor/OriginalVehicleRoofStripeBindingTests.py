"""Native surface-conforming stripe preparation and appearance binding."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class OriginalVehicleRoofStripeBindingTests(unittest.TestCase):
    def setUp(self):
        self.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.header=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        self.plan=read("Source/Latebrake/Public/Vehicles/OriginalVehicleRoofStripe.h")
        self.mesh=read("Source/Latebrake/Public/Vehicles/OriginalVehicleSurfaceMesh.h")

    def test_surface_sampler_uses_real_body_stations(self):
        self.assertIn("OriginalVehicleRoofSample SampleRoof(",self.mesh)
        sampler=self.mesh.split("OriginalVehicleRoofSample SampleRoof(",1)[1].split("private:",1)[0]
        for marker in ("Settings.LongitudinalSegments", "Station(start,visual)", "Station(end,visual)",
                       "a.RoofHalfWidth+(b.RoofHalfWidth-a.RoofHalfWidth)*u",
                       "a.RoofZ+(b.RoofZ-a.RoofZ)*u"):
            self.assertIn(marker,sampler)
        self.assertIn("composer.SampleRoof(id,xa)",self.plan)
        self.assertIn("composer.SampleRoof(id,xb)",self.plan)
        self.assertIn("OriginalRoofStripeClearanceCm",self.plan)

    def test_render_component_is_visual_only(self):
        self.assertIn("TObjectPtr<UProceduralMeshComponent> OriginalRoofStripes;",self.header)
        ctor=self.pawn.split("OriginalRoofStripes = CreateDefaultSubobject",1)[1]
        ctor=ctor.split("// Each source wheel",1)[0]
        for marker in ("SetupAttachment(OriginalVehicleVisual)", "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
                       "SetGenerateOverlapEvents(false)", "SetSimulatePhysics(false)",
                       "SetVisibility(false, false)"):
            self.assertIn(marker,ctor)
        clear=self.pawn.split("void ALatebrakeCarPawn::ClearOriginalRoofStripes()",1)[1]
        clear=clear.split("bool ALatebrakeCarPawn::PrepareOriginalRoofStripes(",1)[0]
        self.assertIn("ClearAllMeshSections()",clear)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::NoCollision)",clear)

    def test_native_commit_requires_all_prepared_stripes(self):
        body=self.pawn.split("void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1)[1]
        body=body.split("int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts()",1)[0]
        self.assertIn("ClearOriginalRoofStripes();",body)
        self.assertIn("if (!PrepareOriginalRoofStripes(VehicleId,BaseMaterial))",body)
        self.assertLess(body.index("if (!PrepareOriginalRoofStripes("),
                        body.index("bOriginalVehicleVisualReady = true;"))
        prepare=self.pawn.split("bool ALatebrakeCarPawn::PrepareOriginalRoofStripes(",1)[1]
        prepare=prepare.split("void ALatebrakeCarPawn::ApplyOriginalRoofStripeLook(",1)[0]
        for marker in ("BuildOriginalRoofStripePlan(VehicleId)", "Band.Count*4u",
                       "CreateMeshSection(Section,Positions,Indices", "false /* visual only */",
                       "CreateDynamicMaterialInstance(Section,Material)",
                       "SetMeshSectionVisible(Section,false)"):
            self.assertIn(marker,prepare)

    def test_cosmetic_updates_only_switch_visibility_and_tint(self):
        body=self.pawn.split("void ALatebrakeCarPawn::ApplyOriginalRoofStripeLook(",1)[1]
        body=body.split("void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1)[0]
        for marker in ("OriginalRoofStripeVisible(Look.Stripe,i)",
                       "SetMeshSectionVisible(Section,bVisible)",
                       "StripeTintForGarageLook(Look)", "SetVectorParameterValue(OriginalVehicleTintParameter,Tint)"):
            self.assertIn(marker,body)
        for forbidden in ("CreateMeshSection(", "CreateDynamicMaterialInstance(", "RebuildOriginalVehicleVisual(",
                          "SaveRecords(", "SetSimulatePhysics(true)"):
            self.assertNotIn(forbidden,body)
        paint=self.pawn.split("void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(",1)[1]
        paint=paint.split("void ALatebrakeCarPawn::RefreshFrontLampPositions()",1)[0]
        self.assertIn("Visible = Visible && !bOriginalVehicleVisualReady;",paint)
        self.assertIn("ApplyOriginalRoofStripeLook(Look);",paint)
        self.assertNotIn("OriginalVehicleMeshSettings{}",paint)
        cmake=read("Tests/Standalone/CMakeLists.txt")
        self.assertIn("add_latebrake_portable_research_test(OriginalVehicleRoofStripe)",cmake)


if __name__ == "__main__":
    unittest.main()
