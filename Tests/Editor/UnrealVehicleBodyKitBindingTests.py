"""Source-only wiring checks for six UE cosmetic vehicle body kits.

These checks do not compile UnrealHeaderTool, inspect materials, or prove the
rendered meshes on actual hardware. They protect collision and garage identity.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


PARTS = (
    "FrontSplitter", "LeftSideSkirt", "RightSideSkirt", "RearDiffuser",
    "HoodScoop", "LeftRearFin", "RightRearFin",
)


class UnrealVehicleBodyKitBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.kits = read("Source/Latebrake/Public/Vehicles/VehicleBodyKitRecipe.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_all_body_kit_parts_exist_and_attach_to_same_chassis(self):
        for part in PARTS:
            with self.subTest(part=part):
                self.assertIn("TObjectPtr<UStaticMeshComponent> " + part + ";",
                              self.header)
                self.assertIn(
                    part + ' = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("' +
                    part + '"));', self.pawn
                )
        setup = self.pawn.split(
            "// Seven additional original geometric details", 1
        )[1].split("FrontLeftWheel = CreateDefaultSubobject", 1)[0]
        self.assertIn("Trim->SetupAttachment(Chassis);", setup)
        self.assertIn("Trim->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
                      setup)
        self.assertIn("Trim->SetGenerateOverlapEvents(false);", setup)
        self.assertIn("Trim->SetSimulatePhysics(false);", setup)
        self.assertIn("Trim->SetCastShadow(false);", setup)
        self.assertIn("if (Cube.Succeeded())", setup)
        self.assertNotIn("SetMassOverrideInKg", setup)

    def test_equipping_a_car_reapplies_complete_visual_recipe(self):
        method = self.pawn.split(
            "void ALatebrakeCarPawn::ApplyVehicleVisualRecipe(int32 VehicleId)", 1
        )[1].split("void ALatebrakeCarPawn::SetGarageTuning", 1)[0]
        self.assertIn("latebrake::BodyKitForVehicle(VehicleId)", method)
        self.assertIn("SetVisibility(bVisible, true);", method)
        self.assertIn("Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
                      method)
        self.assertIn("Part->SetGenerateOverlapEvents(false);", method)
        for part in PARTS:
            self.assertIn("SetTrim(" + part + ",", method)
        self.assertIn("Kit.HasHoodScoop", method)
        self.assertIn("Kit.HasRearFins", method)
        for forbidden in ("Chassis->SetRelativeScale3D(",
                          "Chassis->SetCollisionEnabled(",
                          "Garage.Unlock(", "SaveRecords(",
                          "AddForce(", "SetSimulatePhysics(true)"):
            self.assertNotIn(forbidden, method)

        tune = self.pawn.split(
            "void ALatebrakeCarPawn::SetGarageTuning(", 1
        )[1].split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[0]
        self.assertIn("if (SafeId != GarageVehicleId)", tune)
        self.assertIn("ApplyVehicleVisualRecipe(SafeId);", tune)

    def test_named_body_kits_match_existing_six_garage_ids(self):
        for id_ in range(1, 6):
            self.assertIn(f"case {id_}:", self.kits)
        self.assertIn('return {"STREET"', self.kits)
        self.assertIn('return {"SPRINT"', self.kits)
        self.assertIn('return {"GT"', self.kits)
        self.assertIn('return {"TRACK"', self.kits)
        self.assertIn('return {"TOURING"', self.kits)
        self.assertIn('return {"APEX"', self.kits)
        self.assertIn("latebrake::BodyKitForVehicle(Entry.VehicleId).Name",
                      self.hud)
        self.assertIn("NAME VehicleBodyKit", self.cmake)
        self.assertIn("VehicleBodyKitRecipeTests.cpp", self.cmake)

    def test_body_geometry_updates_are_not_per_frame(self):
        tick = self.pawn.split(
            "void ALatebrakeCarPawn::Tick(float DeltaSeconds)", 1
        )[1].split("bool ALatebrakeCarPawn::IsRacePhysicsReady()", 1)[0]
        for part in PARTS:
            self.assertNotIn(part + "->SetRelative", tick)
        self.assertNotIn("BodyKitForVehicle(", tick)
        self.assertNotIn("CreateDefaultSubobject", tick)


if __name__ == "__main__":
    unittest.main()
