"""Source-only guarantees for original procedural garage car silhouettes.

The Unreal Editor, material appearance and collision cooking must still be
tested on a qualified workstation; this file does not simulate them.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageVehicleVisualBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = source("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.recipe = source("Source/Latebrake/Public/Vehicles/VehicleVisualRecipe.h")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")
        cls.hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.garage = source("Source/Latebrake/Public/Garage/GarageState.h")

    def test_real_pawn_uses_original_primitive_meshes(self):
        for name in ("Hood", "RearDeck", "SpoilerWing",
                     "SpoilerSupportLeft", "SpoilerSupportRight"):
            self.assertIn(f'TEXT("{name}")', self.pawn)
            self.assertIn(f"TObjectPtr<UStaticMeshComponent> {name};", self.header)
            self.assertIn(f"{name}->SetupAttachment(Chassis);", self.pawn)
            self.assertIn(f"ConfigureDecorativeMesh({name}, Cube.Object,", self.pawn)
        self.assertIn("CreateDefaultSubobject<UStaticMeshComponent>", self.pawn)
        self.assertIn('FObjectFinder<UStaticMesh> Cube(', self.pawn)
        self.assertIn("Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
                      self.pawn)
        self.assertIn("Component->SetSimulatePhysics(false);", self.pawn)

    def test_selection_uses_cosmetic_recipe_without_mutating_physics(self):
        body = self.pawn.split(
            "void ALatebrakeCarPawn::ApplyVehicleVisualRecipe(", 1)[1].split(
            "void ALatebrakeCarPawn::SetGarageTuning(", 1)[0]
        for marker in ("GarageState::Definition(", "VisualRecipeForVehicle(",
                       "Cabin->SetRelativeScale3D(", "Hood->SetRelativeScale3D(",
                       "RearDeck->SetRelativeScale3D(",
                       "Part->SetVisibility(Recipe.HasRearWing, true)",
                       "Part->SetCollisionEnabled(ECollisionEnabled::NoCollision)"):
            self.assertIn(marker, body)
        for forbidden in ("AddForce(", "SetMass", "SetSimulatePhysics(true)",
                          "AddTorque", "Award", "Unlock(",
                          "SaveRecords(", "SetGarageTuning("):
            self.assertNotIn(forbidden, body)
        self.assertIn("ApplyVehicleVisualRecipe(0);", self.pawn)
        self.assertIn("ApplyVehicleVisualRecipe(SafeId);", self.pawn)
        self.assertIn("if (SafeId != GarageVehicleId)", self.pawn)

    def test_six_fictional_variants_and_portable_suite(self):
        for part in ("case 1:", "case 2:", "case 3:", "case 4:", "case 5:", "default:",
                     "HasRearWing", "HoodLengthScale",
                     "RearDeckLengthScale"):
            self.assertIn(part, self.recipe)
        self.assertIn("VehicleVisualRecipeTests.cpp", self.cmake)
        self.assertIn("NAME VehicleVisualRecipe", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("VehicleCount == 6", self.garage)
        self.assertIn("KnownVehicleMask", self.garage)
        self.assertIn("latebrake::GarageState::IsVehicle(VehicleId)", self.pawn)
        self.assertIn("Recipe.WingSpanScale", self.pawn)
        self.assertIn("CAR %d/%d", self.hud)
        self.assertIn("BASE ACCEL %.2fx | GRIP %.2fx | TOP %.2fx", self.hud)
        self.assertIn("Mode->GetNextLockedVehicleCost()", self.hud)


if __name__ == "__main__":
    unittest.main()
