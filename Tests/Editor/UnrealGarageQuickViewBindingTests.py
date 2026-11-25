"""Actual Unreal Canvas 2D schematic direct-preview source wiring."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def source(path): return (ROOT/path).read_text(encoding="utf-8")
class UnrealGarageQuickViewBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud=source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.pawn=source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.mode=source("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.header=source("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.policy=source("Source/Latebrake/Public/UI/GarageShowroomQuickView.h")
        cls.cursor=source("Source/Latebrake/Public/Garage/GarageGalleryCursor.h")
        cls.tests=source("Tests/Standalone/GarageShowroomQuickViewTests.cpp")
        cls.cmake=source("Tests/Standalone/CMakeLists.txt")
    def test_shared_render_hit_layout(self):
        self.assertEqual(self.hud.count("latebrake::CalculateGarageQuickViewLayout("),2)
        self.assertEqual(self.hud.count("latebrake::HitTestGarageQuickView("),2)
        self.assertIn("QuickView.Buttons.size()",self.hud)
        self.assertIn("GarageShowroomQuickView.h",self.hud)
        self.assertIn("Mode->CanBrowseGarage()",self.hud)
        self.assertIn("schematic.Pieces[j].Bounds",self.policy)
        self.assertIn("std::array<DesktopMenuMouseButton, VehicleCount>",self.policy)
    def test_controller_authority_is_read_only(self):
        for i in range(6):
            self.assertIn("DesktopMenuMouseAction::GarageQuickView"+str(i),self.pawn)
        self.assertIn("Mode->GarageBrowseVehicle(",self.pawn)
        self.assertIn("GarageQuickViewIndex(Action)",self.pawn)
        self.assertIn("void GarageBrowseVehicle(int32 VehicleId);",self.header)
        handler=self.mode.split(
            "void ALatebrakeRaceGameMode::GarageBrowseVehicle(int32 VehicleId)",1
        )[1].split("void ALatebrakeRaceGameMode::GarageBrowsePrevious()",1)[0]
        for marker in ("CanBrowseGarage()","GarageState::IsVehicle(VehicleId)",
                       "GarageGallery.Browse(VehicleId)","GaragePurchaseConfirm.Reset()"):
            self.assertIn(marker,handler)
        for forbidden in ("SaveRecords(","Garage.Select(","Garage.Unlock(",
                          "ApplyGarageToPlayer(","EarnCredits("):
            self.assertNotIn(forbidden,handler)
        self.assertIn("constexpr bool Browse(int vehicleId)",self.cursor)
    def test_real_portable_coverage(self):
        self.assertIn("add_latebrake_portable_research_test(GarageShowroomQuickView)",self.cmake)
        self.assertIn("AllResolutions();",self.tests)
        self.assertIn("FailClosed();",self.tests)
if __name__=="__main__":
    unittest.main()
