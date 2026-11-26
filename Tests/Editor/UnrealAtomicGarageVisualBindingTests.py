"""Atomic equipped-car identity+saved-look delivery in real Unreal GameMode.

These tests examine live source calling order and pure transition policy;
they cannot certify Unreal 5.8 UHT, cooked game, or GPU rendering.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class UnrealAtomicGarageVisualBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.h=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy=read("Source/Latebrake/Public/Vehicles/GarageVisualTransition.h")
        cls.tests=read("Tests/Standalone/GarageVisualTransitionTests.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
    def test_gamemode_uses_actual_owned_selected_appearance_before_render(self):
        mode=self.mode.split("void ALatebrakeRaceGameMode::ApplyGarageToPlayer()",1
        )[1].split("bool ALatebrakeRaceGameMode::CanBrowseGarage()",1)[0]
        self.assertIn("Car->SetGarageTuning(",mode)
        self.assertIn("Garage.SelectedVehicle(),\n            GarageLooks.Get(Garage.SelectedVehicle()));",mode)
        self.assertIn("Car->SetGarageAppearance(GarageLooks.Get(Garage.SelectedVehicle()))",mode)
        self.assertLess(mode.index("Car->SetGarageTuning("),
                        mode.index("Car->SetGarageAppearance("))
        self.assertIn("LastAppliedTuning = Requested;",mode)
        self.assertIn("LastTunedPawn = Car;",mode)
    def test_one_source_rebuild_with_committed_new_saved_look(self):
        selected=self.pawn.split("void ALatebrakeCarPawn::SetGarageTuning(",1
        )[1].split("void ALatebrakeCarPawn::SetGarageAppearance(",1)[0]
        self.assertIn("GarageVehicleId = SafeId;",selected)
        self.assertIn("CurrentGarageLook = Transition.Look;",selected)
        self.assertIn("PlanGarageVisualTransition(",selected)
        self.assertIn("if (Transition.RebuildSourceMesh)",selected)
        self.assertIn("if (Transition.RefreshMaterials)",selected)
        self.assertEqual(selected.count("RebuildOriginalVehicleVisual("),1)
        self.assertLess(selected.index("GarageVehicleId = SafeId;"),
                        selected.index("RebuildOriginalVehicleVisual("))
        self.assertLess(selected.index("CurrentGarageLook = Transition.Look;"),
                        selected.index("RebuildOriginalVehicleVisual("))
        self.assertLess(selected.index("RebuildOriginalVehicleVisual("),
                        selected.index("ApplyVehiclePaintRecipe("))
        self.assertIn("int32 VehicleId, latebrake::GarageLook Look);",self.h)
        source=self.pawn.split("void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1
        )[1].split("void ALatebrakeCarPawn::ApplyOriginalVehiclePalette(",1)[0]
        self.assertIn("GetPresentedGarageLook().Wheels",source)
    def test_color_only_still_does_not_tune_or_rebuild(self):
        appearance=self.pawn.split("void ALatebrakeCarPawn::SetGarageAppearance(",1
        )[1].split("void ALatebrakeCarPawn::ClearDriverInputs()",1)[0]
        self.assertIn("const bool bRimsChanged =",appearance)
        self.assertIn("if (bRimsChanged) RebuildOriginalVehicleVisual(",appearance)
        self.assertIn("ApplyVehiclePaintRecipe(GarageVehicleId);",appearance)
        self.assertNotIn("SetGarageTuning(",appearance)
        self.assertNotIn("GarageAcceleration =",appearance)
        self.assertNotIn("GarageGrip =",appearance)
        self.assertNotIn("GarageTopSpeed =",appearance)
    def test_policy_rejects_invalid_and_exhaustively_checks_six_car_options(self):
        for text in ("GarageState::IsVehicle(requestedId)",
                     "ValidGarageLook(requestedLook)",
                     "FactoryGarageLook(plan.VehicleId)",
                     "plan.VehicleChanged || plan.RimsChanged",
                     "plan.VehicleChanged || plan.AppearanceChanged"):
            self.assertIn(text,self.policy)
        self.assertIn("all distinct current/next car/rim/paint/stripe options traversed",self.tests)
        self.assertIn("add_latebrake_portable_research_test(GarageVisualTransition)",self.cmake)
if __name__=="__main__":unittest.main()
