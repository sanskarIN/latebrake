"""Real Unreal original car: five articulated, purely visual render parts.

Source-only binding checks cannot replace licensed UE5.8/UHT/shader
or installed Win64 driving and manual steering visual verification.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class UnrealAnimatedWheelRenderBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.c=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.h=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.parts=read("Source/Latebrake/Public/Vehicles/OriginalVehicleAnimatedWheels.h")
        cls.mesh=read("Source/Latebrake/Public/Vehicles/OriginalVehicleSurfaceMesh.h")
        cls.test=read("Tests/Standalone/OriginalVehicleAnimatedWheelsTests.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
        cls.qa=read("docs/UE_ORIGINAL_VEHICLE_RUNTIME_QA.md")
    def test_native_engine_components_are_real_and_noncolliding(self):
        ctor=self.c.split("ALatebrakeCarPawn::ALatebrakeCarPawn()",1)[1].split(
            "void ALatebrakeCarPawn::BeginPlay()",1)[0]
        for part in ("OriginalFrontLeftWheel","OriginalFrontRightWheel",
                     "OriginalRearLeftWheel","OriginalRearRightWheel"):
            self.assertIn(f"TObjectPtr<UProceduralMeshComponent> {part};",self.h)
            self.assertIn('TEXT("'+part+'")',ctor)
        for token in ("Wheel->SetupAttachment(OriginalVehicleVisual);",
                      "Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
                      "Wheel->SetGenerateOverlapEvents(false);",
                      "Wheel->SetSimulatePhysics(false);",
                      "OriginalWheelPivot(i)", "Wheel->SetRelativeLocation("):
            self.assertIn(token,ctor)
    def test_full_geometry_materials_and_rollback(self):
        rebuild=self.c.split(
            "void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1
        )[1].split("void ALatebrakeCarPawn::ApplyOriginalVehiclePalette(",1)[0]
        for marker in ("MakeOriginalVehicleRenderSections(",
                       "MakeOriginalVehicleAnimatedParts(",
                       "RenderParts[]", "OriginalWheelPivot(",
                       "vertex.Position.X-Pivot.X",
                       "Part->CreateMeshSection(", "Part->CreateDynamicMaterialInstance(",
                       "false /* no collision, ever */",
                       "for (auto* Part : RenderParts)",
                       "Part->ClearAllMeshSections();",
                       "OriginalVehicleVisual->SetVisibility(true, false);",
                       "RenderParts[i]->SetVisibility(true, false);",
                       "HideLegacyVisualPrimitives(false);",
                       "HideLegacyVisualPrimitives(true);"):
            self.assertIn(marker,rebuild)
        body=rebuild.split("const auto animated =",1)[1]
        self.assertLess(body.index("MakeOriginalVehicleAnimatedParts("),
                        body.index("Part->CreateMeshSection("))
        self.assertLess(body.index("CanPresentOriginalVehicleVisual("),
                        body.index("HideLegacyVisualPrimitives(true);"))
        palette=self.c.split(
            "void ALatebrakeCarPawn::ApplyOriginalVehiclePalette(",1
        )[1].split("void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(",1)[0]
        self.assertIn("Part->GetMaterial(section)",palette)
        self.assertIn("OriginalVehicleTintParameter, FLinearColor(",palette)
        self.assertIn("NAME OriginalVehicleAnimatedWheels",self.cmake)
        self.assertIn("AllCarsAllRims();",self.test)
    def test_animation_uses_measured_sign_speed_and_real_radius(self):
        pose=self.c.split(
            "void ALatebrakeCarPawn::ApplyWheelVisualPose()",1
        )[1].split("bool ALatebrakeCarPawn::IsRacePhysicsReady()",1)[0]
        self.assertIn("WheelMotion.Current()",pose)
        self.assertIn("FQuat SourceRoll(FVector::RightVector,",pose)
        self.assertIn("OriginalWheels[Index]->SetRelativeRotation(Visual)",pose)
        self.assertIn("Index<2 ? Steering : FQuat::Identity",pose)
        self.assertIn("bOriginalVehicleVisualReady",pose)
        self.assertIn("OriginalVehicleMeshSettings{}",pose)
        self.assertIn("WheelRadiusCm",pose)
        self.assertIn("WheelMotion.Advance({",pose)
        for forbidden in ("AddForce(", "AddTorqueInRadians(", "SaveRecords(",
                          "SetCollisionProfileName(", "CreateMeshSection("):
            self.assertNotIn(forbidden,pose)
    def test_docs_explicit_not_physics_verified(self):
        self.assertIn("actual UE5.8/UHT compile",self.qa)
        self.assertIn("OriginalVehicleAnimatedPartCount=5u",self.parts)
        self.assertIn("18 car/rim",self.qa)
if __name__=="__main__":
    unittest.main()
