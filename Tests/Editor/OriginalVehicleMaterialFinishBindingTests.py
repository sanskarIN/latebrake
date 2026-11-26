"""Source contracts for capability-gated vehicle finish writes, not shader QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

class OriginalVehicleMaterialFinishBindingTests(unittest.TestCase):
    def setUp(self):
        self.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.header=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        self.hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")

    def test_only_real_global_scalar_parameters_are_admitted(self):
        body=self.pawn.split("uint32 DiscoverOriginalVehicleFinishParameters(",1)[1]
        body=body.split("void ApplyOriginalVehicleFinish(",1)[0]
        for marker in ("GetAllScalarParameterInfo(ScalarInfo,ScalarGuids)",
                       "VehicleFinishParameterNames", "Info.Name==Name",
                       "Info.Association==EMaterialParameterAssociation::GlobalParameter",
                       "Mask|=1u<<i"):
            self.assertIn(marker,body)
        self.assertNotIn("SetScalarParameterValue(",body)
        apply=self.pawn.split("void ApplyOriginalVehicleFinish(",1)[1]
        apply=apply.split("void ALatebrakeCarPawn::ClearOriginalRoofStripes()",1)[0]
        self.assertIn("PlanVehicleFinishWrites(Exposed,Finish)",apply)
        self.assertIn("i<Plan.Count",apply)
        self.assertIn("Material->SetScalarParameterValue(Name,static_cast<float>(Write.Value))",apply)

    def test_capabilities_reset_before_rebuilding_and_commit_only_after_preflight(self):
        body=self.pawn.split("void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1)[1]
        body=body.split("int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts()",1)[0]
        reset=body.index("OriginalVehicleFinishMask = 0u;")
        discovery=body.index("OriginalVehicleFinishMask = DiscoverOriginalVehicleFinishParameters(BaseMaterial);")
        self.assertLess(reset,body.index("LoadObject<UMaterialInterface>"))
        self.assertLess(body.index("if (!PrepareOriginalRoofStripes("),discovery)
        self.assertLess(discovery,body.index("bOriginalVehicleVisualReady = true;"))

    def test_same_palette_updates_tint_and_finish_without_geometry_or_physics(self):
        body=self.pawn.split("void ALatebrakeCarPawn::ApplyOriginalVehiclePalette(",1)[1]
        body=body.split("void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(",1)[0]
        self.assertIn("material->SetVectorParameterValue(",body)
        self.assertIn("ApplyOriginalVehicleFinish(material,OriginalVehicleFinishMask,",body)
        self.assertIn("OriginalSurfaceFinish(",body)
        stripe=self.pawn.split("void ALatebrakeCarPawn::ApplyOriginalRoofStripeLook(",1)[1]
        stripe=stripe.split("void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1)[0]
        self.assertIn("OriginalStripeFinish(Look)",stripe)
        self.assertIn("ApplyOriginalVehicleFinish(Instance,OriginalVehicleFinishMask,",stripe)
        for section in (body,stripe):
            for forbidden in ("CreateMeshSection(", "SetCollisionProfileName(", "SaveRecords(",
                              "SetSimulatePhysics(true)", "AddForce("):
                self.assertNotIn(forbidden,section)

    def test_diagnostic_never_claims_shader_verified(self):
        self.assertIn("GetOriginalVehicleFinishParameterCount()",self.header)
        self.assertIn("bOriginalVehicleVisualReady ? latebrake::vehiclemesh::VehicleFinishCapabilityCount",self.header)
        for marker in ("Car->GetOriginalVehicleFinishParameterCount()",
                       "FINISH PARAMETERS: %d/5 EXPOSED", 'TEXT("VERIFY SHADER")',
                       'TEXT("TINT ONLY")'):
            self.assertIn(marker,self.hud)
        self.assertIn("add_latebrake_portable_research_test(OriginalVehicleMaterialFinish)",
                      read("Tests/Standalone/CMakeLists.txt"))

if __name__=="__main__":
    unittest.main()
