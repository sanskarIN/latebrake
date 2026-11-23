"""Real Unreal Pawn linkage for six generated vehicle surfaces.

These source contracts and portable C++ tests ARE NOT equivalent to
an Unreal 5.8 UHT/Editor/cooked Win64, physical materials or rendering QA.
"""
from pathlib import Path
import unittest
import json

ROOT=Path(__file__).resolve().parents[2]

def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class UnrealOriginalVehicleRenderBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.actor=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.sections=read("Source/Latebrake/Public/Vehicles/OriginalVehicleRenderSections.h")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
        cls.tests=read("Tests/Standalone/OriginalVehicleRenderSectionsTests.cpp")
        cls.module=read("Source/Latebrake/Latebrake.Build.cs")
        cls.project=json.loads(read("Latebrake.uproject"))

    def test_live_pawn_uses_real_generator_not_only_source_fake(self):
        self.assertIn("TObjectPtr<UProceduralMeshComponent> OriginalVehicleVisual;",self.header)
        self.assertIn("ProceduralMeshComponent.h",self.actor)
        self.assertIn("OriginalVehicleRenderSections.h",self.actor)
        self.assertIn('TEXT("OriginalVehicleVisual")',self.actor)
        self.assertIn("OriginalVehicleSurfaceComposer{}.Build(",self.actor)
        self.assertIn("MakeOriginalVehicleRenderSections(",self.actor)
        self.assertIn("OriginalVehicleVisual->CreateMeshSection(",self.actor)
        self.assertIn("OriginalVehicleVisual->CreateDynamicMaterialInstance(",self.actor)
        self.assertIn("OriginalVehicleVisual->ClearAllMeshSections();",self.actor)
        self.assertIn("ProceduralMeshComponent",self.module)
        self.assertIn("ProceduralMeshComponent",
            [entry["Name"] for entry in self.project["Plugins"] if entry["Enabled"]])
        self.assertIn("NAME OriginalVehicleRenderSections",self.cmake)
        self.assertIn("OriginalVehicleRenderSectionsTests.cpp",self.cmake)
        self.assertIn("InvalidMeshingCannotRender();",self.tests)
        self.assertIn("AllCarSections();",self.tests)

    def test_physics_chassis_untouched_and_mesh_never_collides(self):
        proc=self.actor.split(
            "void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1
        )[1].split("void ALatebrakeCarPawn::ApplyOriginalVehiclePalette(",1)[0]
        for token in (
            "generated.Valid", "sections.Valid",
            "sections.TotalTriangles != generated.Mesh.Triangles.size()",
            "false /* no collision, ever */",
            "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
            "SetGenerateOverlapEvents(false)",
            "SetSimulatePhysics(false)",
            "HideLegacyVisualPrimitives(false);",
            "HideLegacyVisualPrimitives(true);",
            "OriginalVehicleVisual->SetVisibility(true, false);",
        ):
            self.assertIn(token,proc)
        for token in ("AddForce(", "SetMassOverrideInKg(", "Garage.Unlock(",
                      "SetCollisionProfileName(", "SaveRecords(",
                      "SetRelativeScale3D(RootScale)", "EarnCredits("):
            self.assertNotIn(token,proc)
        self.assertIn("!mesh.VisualOnly || mesh.ChangesChaosContact",self.sections)
        self.assertIn("OriginalVehicleSurfaceComposer{}.ValidateMesh(mesh)",self.sections)
        self.assertIn("OriginalVehicleVisual->SetupAttachment(Chassis);",self.actor)
        self.assertIn("OriginalVehicleVisualOffsetZCm",self.actor)
        self.assertIn("OriginalVehicleVisualOffsetZCm+40.0 == -22.0",self.sections)
        self.assertIn("CabinRoofHeightCm +",self.actor)
        self.assertIn("RoofStripeLeft",self.actor)
        self.assertIn("HideLegacyVisualPrimitives(true);",self.actor)
        self.assertIn("Part->SetHiddenInGame(bHide, false);",self.actor)

    def test_only_equip_switch_rebuilds_surface_and_paint_reuses_materials(self):
        tick=self.actor.split(
            "void ALatebrakeCarPawn::Tick(float DeltaSeconds)",1
        )[1].split("bool ALatebrakeCarPawn::IsRacePhysicsReady()",1)[0]
        self.assertNotIn("RebuildOriginalVehicleVisual(",tick)
        self.assertNotIn("MakeOriginalVehicleRenderSections(",tick)
        self.assertIn("RebuildOriginalVehicleVisual(GarageVehicleId);",self.actor)
        selected=self.actor.split(
            "void ALatebrakeCarPawn::SetGarageTuning(",1
        )[1].split("void ALatebrakeCarPawn::SetGarageAppearance(",1)[0]
        self.assertIn("if (SafeId != GarageVehicleId)",selected)
        self.assertIn("RebuildOriginalVehicleVisual(SafeId);",selected)
        appearance=self.actor.split(
            "void ALatebrakeCarPawn::SetGarageAppearance(",1
        )[1].split("void ALatebrakeCarPawn::",1)[0]
        self.assertIn("const bool bRimsChanged =",appearance)
        self.assertIn("if (bRimsChanged) RebuildOriginalVehicleVisual(",appearance)
        self.assertIn("ApplyVehiclePaintRecipe(GarageVehicleId);",appearance)
        self.assertLess(appearance.index("RebuildOriginalVehicleVisual("),
                        appearance.index("ApplyVehiclePaintRecipe("))
        style=self.actor.split(
            "void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(",1
        )[1].split("void ALatebrakeCarPawn::SetGarageTuning(",1)[0]
        self.assertIn("ApplyOriginalVehiclePalette(VehicleId, Look);",style)
        self.assertIn("OriginalVehicleVisual->GetMaterial(section)",self.actor)
        self.assertIn('TEXT("Color")',self.actor)

    def test_use_editor_verified_assets_before_release(self):
        self.assertIn("EngineAssociation",self.project)
        self.assertEqual(self.project["EngineAssociation"],"5.8")
        guide=read("docs/UE_ORIGINAL_VEHICLE_RUNTIME_QA.md")
        for required in ("UHT", "cooked", "collision", "Material", "fallback",
                         "six", "unverified", "real Windows"):
            self.assertIn(required.lower(),guide.lower())

if __name__=="__main__":
    unittest.main()
