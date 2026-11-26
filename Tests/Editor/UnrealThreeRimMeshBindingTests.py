"""Real production 3D wheel geometry tracks the three saved rim choices."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class UnrealThreeRimMeshBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mesh=read("Source/Latebrake/Public/Vehicles/OriginalVehicleSurfaceMesh.h")
        cls.recipe=read("Source/Latebrake/Public/Vehicles/OriginalVehicleRimRecipe.h")
        cls.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.tests=read("Tests/Standalone/OriginalVehicleSurfaceMeshTests.cpp")
    def test_three_distinct_actual_spoke_meshes(self):
        for text in ("RimSpokeQuads", "Build(int vehicleId,int rimStyle=0)",
                     "BuildFourWheels(writer,rimStyle)",
                     "mesh.RimSpokeQuads!=",
                     "OriginalVehicleMaterial::Alloy",
                     "OriginalRimRecipeForStyle(rimStyle)", "recipe.Spokes"):
            self.assertIn(text,self.mesh)
        for text in ("OriginalVehicleRimStyleCount=3", "OriginalVehicleSpokesForStyle(int style)",
                     "{7,0.09,0.16,0.91}", "{5,0.17,0.12,0.94}", "{12,0.065,0.22,0.97}"):
            self.assertIn(text,self.recipe)
        self.assertIn("ThreeActualWheelDesigns();",self.tests)
        self.assertIn("MakeOriginalVehicleRenderSections(mesh)",self.tests)
    def test_real_pawn_rebuilds_on_rim_change_not_on_tick(self):
        self.assertIn("VehicleId,static_cast<int>(GetPresentedGarageLook().Wheels)",
                      self.pawn)
        appearance=self.pawn.split(
            "void ALatebrakeCarPawn::SetGarageAppearance(",1
        )[1].split("void ALatebrakeCarPawn::",1)[0]
        self.assertIn("const bool bRimsChanged",appearance)
        self.assertIn("if (bRimsChanged) RebuildOriginalVehicleVisual(",appearance)
        self.assertIn("ApplyVehiclePaintRecipe(GarageVehicleId);",appearance)
        self.assertLess(appearance.index("RebuildOriginalVehicleVisual("),
                        appearance.index("ApplyVehiclePaintRecipe("))
        tick=self.pawn.split(
            "void ALatebrakeCarPawn::Tick(float DeltaSeconds)",1
        )[1].split("bool ALatebrakeCarPawn::IsRacePhysicsReady()",1)[0]
        self.assertNotIn("RebuildOriginalVehicleVisual(",tick)
    def test_mesh_generator_cannot_change_gameplay_authority(self):
        generated=self.mesh.split("OriginalVehicleMeshResult Build(",1)[1].split(
            "bool ValidateMesh(",1)[0]
        for x in ("AddForce(", "SetMassOverrideInKg(", "SaveRecords(",
                  "Garage.Unlock(", "SetCollisionEnabled("):
            self.assertNotIn(x,generated)
if __name__=="__main__":
    unittest.main()
