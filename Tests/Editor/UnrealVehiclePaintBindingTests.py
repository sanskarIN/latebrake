"""Source-binding checks for six UE procedural car paint identities.

Does not execute licensed Unreal, render basic-shape materials or prove
their 'Color' vector parameter is exposed on installed UE 5.8 hardware.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealVehiclePaintBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.recipe = read("Source/Latebrake/Public/Vehicles/VehiclePaintRecipe.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_selection_and_constructor_lifecycle_are_distinct(self):
        constructor = self.pawn.split(
            "ALatebrakeCarPawn::ALatebrakeCarPawn()", 1)[1].split(
            "void ALatebrakeCarPawn::BeginPlay()", 1)[0]
        self.assertIn("ApplyVehicleVisualRecipe(0);", constructor)
        self.assertNotIn("ApplyVehiclePaintRecipe(", constructor,
                         "never create dynamic materials on the class default object")
        begin = self.pawn.split(
            "void ALatebrakeCarPawn::BeginPlay()", 1)[1].split(
            "void ALatebrakeCarPawn::", 1)[0]
        self.assertIn("ApplyVehiclePaintRecipe(GarageVehicleId);", begin)
        tune = self.pawn.split("void ALatebrakeCarPawn::SetGarageTuning(", 1)[1].split(
            "void ALatebrakeCarPawn::ClearDriverInputs()", 1)[0]
        self.assertIn("if (SafeId != GarageVehicleId)", tune)
        self.assertIn("ApplyVehicleVisualRecipe(SafeId);", tune)
        self.assertIn("HasActorBegunPlay()) ApplyVehiclePaintRecipe(SafeId)", tune)
        self.assertIn("void ApplyVehiclePaintRecipe(int32 VehicleId);", self.header)

    def test_paint_materials_reuse_original_mesh_parent(self):
        method = self.pawn.split(
            "void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(int32 VehicleId)", 1
        )[1].split("void ALatebrakeCarPawn::SetGarageTuning(", 1)[0]
        for expected in (
            "latebrake::PaintForVehicle(VehicleId)",
            "Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0))",
            "Part->CreateDynamicMaterialInstance(0)",
            'TEXT("Color")',
            "Instance->SetVectorParameterValue(",
            "Tint(Chassis, Paint.Body);",
            "Tint(Cabin, Paint.Canopy);",
            "Tint(FrontSplitter, Paint.Accent);",
            "Tint(RearDiffuser, Paint.Accent);",
            "Tint(SpoilerWing, Paint.Accent);",
        ):
            with self.subTest(expected=expected):
                self.assertIn(expected, method)
        for forbidden in (
            "SetSimulatePhysics(", "SetMassOverrideInKg(", "SetCollisionEnabled(",
            "AddForce(", "AddTorque(", "Garage.Unlock(", "SaveRecords(",
            "EarnCredits(", "NewObject<", "CreateDefaultSubobject<",
        ):
            self.assertNotIn(forbidden, method)
        tick = self.pawn.split(
            "void ALatebrakeCarPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "bool ALatebrakeCarPawn::IsRacePhysicsReady()", 1)[0]
        self.assertNotIn("ApplyVehiclePaintRecipe(", tick)
        self.assertNotIn("SetVectorParameterValue(", tick)

    def test_all_six_named_colors_appear_as_read_only_garage_identity(self):
        self.assertIn('#include "Vehicles/VehiclePaintRecipe.h"', self.hud)
        self.assertIn("latebrake::PaintForVehicle(Entry.VehicleId).Name", self.hud)
        self.assertIn("PAINT %s", self.hud)
        for paint in (
            "SKY BLUE", "EMBER ORANGE", "ELECTRIC VIOLET",
            "RACING SCARLET", "SEA GLASS", "SOLAR YELLOW",
        ):
            self.assertIn('"' + paint + '"', self.recipe)
        self.assertIn("NAME VehiclePaintRecipe", self.cmake)
        self.assertIn("VehiclePaintRecipeTests.cpp", self.cmake)


if __name__ == "__main__":
    unittest.main()
