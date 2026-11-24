"""Actual Unreal pawn 3D rear signal lights follow the visible bumper."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(path):return (ROOT/path).read_text(encoding="utf-8")
class UnrealRearSignalRenderBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy=read("Source/Latebrake/Public/Vehicles/VehicleRearSignalRenderRig.h")
        cls.tests=read("Tests/Standalone/VehicleRearSignalRenderRigTests.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
    def test_real_signal_lamps_update_only_on_render_changes(self):
        visual=self.cpp.split("void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(",1
        )[1].split("void ALatebrakeCarPawn::SetGarageTuning(",1)[0]
        self.assertIn("RefreshRearSignalPositions();",visual)
        self.assertIn("RefreshFrontLampPositions();",visual)
        self.assertIn("void ALatebrakeCarPawn::RefreshRearSignalPositions()",visual)
        for token in ("PlanRearSignalVisualOffsets(", "bOriginalVehicleVisualReady",
                      "RearLeftSignal.Get()", "RearRightSignal.Get()",
                      "ReverseSignal.Get()", "RearSignalNearVisibleBumper(",
                      "Lamps[i]->SetRelativeLocation("):
            self.assertIn(token,visual)
        self.assertIn("void RefreshRearSignalPositions();",self.pawn)
        tick=self.cpp.split("void ALatebrakeCarPawn::Tick(float DeltaSeconds)",1
        )[1].split("bool ALatebrakeCarPawn::IsRacePhysicsReady()",1)[0]
        self.assertNotIn("RefreshRearSignalPositions(",tick)
        self.assertNotIn("PlanRearSignalVisualOffsets(",tick)
    def test_no_physics_save_or_gameplay_authority(self):
        method=self.cpp.split(
            "void ALatebrakeCarPawn::RefreshRearSignalPositions()",1
        )[1].split("void ALatebrakeCarPawn::SetGarageTuning(",1)[0]
        for forbidden in ("AddForce(", "SetMassOverrideInKg(",
                          "EarnCredits(", "SaveRecords(", "SpawnActor<"):
            self.assertNotIn(forbidden,method)
        self.assertIn("VehicleRearLampOffsets",self.policy)
        self.assertIn("OriginalVehicleMeshSettings",self.policy)
        self.assertIn("200000",self.tests)
        self.assertIn("NAME VehicleRearSignalRenderRig",self.cmake)
if __name__=="__main__":
    unittest.main()
