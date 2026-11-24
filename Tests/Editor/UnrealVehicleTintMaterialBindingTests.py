"""Production Unreal original car appearance: no silent untintable 3D mesh."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def src(path):
    return (ROOT/path).read_text(encoding="utf-8")
class UnrealVehicleTintMaterialBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.actor=src("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn=src("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy=src("Source/Latebrake/Public/Vehicles/OriginalVehicleTintMaterialPolicy.h")
        cls.tests=src("Tests/Standalone/OriginalVehicleTintMaterialPolicyTests.cpp")
        cls.cmake=src("Tests/Standalone/CMakeLists.txt")
        cls.qa=src("docs/UE_ORIGINAL_VEHICLE_RUNTIME_QA.md")
    def test_engine_actual_material_query_before_mesh_commit(self):
        build=self.actor.split(
            "void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1
        )[1].split("void ALatebrakeCarPawn::ApplyOriginalVehiclePalette(",1)[0]
        for marker in ("GetAllVectorParameterInfo(VectorInfo, VectorGuids)",
                       "SelectVehicleTintCandidate(","VectorInfo.ContainsByPredicate(",
                       "OriginalVehicleTintParameter = NAME_None;",
                       "OriginalVehicleTintParameter = ResolvedTintName;",
                       "CanPresentOriginalVehicleVisual(",
                       "HideLegacyVisualPrimitives(false);",
                       "HideLegacyVisualPrimitives(true);"):
            self.assertIn(marker,build)
        self.assertLess(build.index("GetAllVectorParameterInfo("),
                        build.index("CreateDynamicMaterialInstance("))
        self.assertLess(build.index("CanPresentOriginalVehicleVisual("),
                        build.index("HideLegacyVisualPrimitives(true);"))
        self.assertIn("FName OriginalVehicleTintParameter = NAME_None;",self.pawn)
    def test_real_paint_uses_discovered_parameter(self):
        palette=self.actor.split(
            "void ALatebrakeCarPawn::ApplyOriginalVehiclePalette(",1
        )[1].split("void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(",1)[0]
        self.assertIn("OriginalVehicleTintParameter.IsNone()",palette)
        self.assertIn("OriginalVehicleTintParameter, FLinearColor(",palette)
        self.assertNotIn('TEXT("Color"), FLinearColor(',palette)
        for name in ("Color","Colour","BaseColor","BaseColour","Tint"):
            self.assertIn('"'+name+'"',self.policy)
        self.assertIn("NAME OriginalVehicleTintMaterial",self.cmake)
        self.assertIn("65536u",self.tests)
        self.assertIn("65,536 parameter masks",self.qa)
    def test_no_gameplay_writes_in_render_flow(self):
        body=self.actor.split(
            "void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1
        )[1].split("void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(",1)[0]
        for forbidden in ("SaveRecords(", "EarnCredits(", "SetMassOverrideInKg(",
                          "AddForce(", "Garage.Unlock(", "SetSimulatePhysics(true)"):
            self.assertNotIn(forbidden,body)
if __name__=="__main__":
    unittest.main()
