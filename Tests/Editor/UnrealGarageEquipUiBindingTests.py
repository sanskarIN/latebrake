"""Prove Unreal 3D Garage's on-screen EQUIP button matches real authority.

Pure portable policy and source bindings only. UE5.8 Editor/UHT/cooked build and
hardware interaction are still mandatory before an Unreal game release.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/"Source/Latebrake"
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class UnrealGarageEquipUiBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy=read("Source/Latebrake/Public/Garage/GarageEquipUiPolicy.h")
        cls.modeh=read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.pointer=read("Source/Latebrake/Public/UI/HudMenuMouseControls.h")
        cls.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
        cls.tests=read("Tests/Standalone/GarageEquipUiTests.cpp")

    def test_ui_reasons_are_grounded_in_real_game_authority(self):
        self.assertIn('Garage/GarageEquipUiPolicy.h',self.modeh)
        self.assertIn("GetGarageEquipUiState() const",self.modeh)
        self.assertIn("CanEquipGarageViewed() const",self.modeh)
        source=self.mode.split(
            "ALatebrakeRaceGameMode::GetGarageEquipUiState() const",1
        )[1].split("void ALatebrakeRaceGameMode::GarageBrowseNext()",1)[0]
        for marker in (
            "Menu.IsOpen(), Menu.Current()",
            "ManualPause.IsSuspended() || AppFocusGuard.IsSuspended()",
            "bFutureSaveSchema",
            "CanEditGarageDuringRace(bCareerRace, Race)",
            "CanEquipGalleryVehicle(Garage, GarageGallery)",
            "GarageGallery.Viewed(), Garage.SelectedVehicle()",
            "CanActivateGarageEquip(GetGarageEquipUiState())",
        ):
            self.assertIn(marker,source)
        for marker in ("GarageEquipUiState::Ready", "GarageEquipUiState::Equipped",
                       "GarageEquipUiState::LockedVehicle",
                       "GarageEquipUiState::ReadOnly",
                       "GarageEquipUiState::CareerLocked",
                       "GarageEquipUiState::Suspended"):
            self.assertIn(marker,self.policy)

    def test_same_availability_drives_draw_and_hit_test(self):
        self.assertEqual(self.hud.count(
            "latebrake::AvailableDesktopMenuActions("),2)
        self.assertGreaterEqual(self.hud.count(
            "Mode->CanEquipGarageViewed()"),2)
        self.assertIn("return availability.Activate;",self.pointer)
        switch=self.pointer.split("inline bool DesktopMenuMouseActionEnabled(",1)[1].split(
            "inline DesktopMenuMouseLayout CalculateDesktopMenuMouseLayout(",1)[0]
        nav=switch.split("case DesktopMenuMouseAction::PreviousPage:",1)[1].split(
            "case DesktopMenuMouseAction::Activate:",1)[0]
        self.assertIn("case DesktopMenuMouseAction::NextPage:",nav)
        self.assertIn("case DesktopMenuMouseAction::Close:",nav)
        self.assertIn("return true;",nav)
        self.assertNotIn("availability.Activate",nav)
        self.assertIn("availability.Activate = canEquipGarageViewed;",
                      self.pointer)
        self.assertIn("Mode->GetGarageEquipUiState()",self.hud)
        self.assertIn("GarageEquipUiCaption(",self.hud)
        self.assertIn("DrawText(Caption,",self.hud)
        self.assertIn("HitTestDesktopMenuMouse(buttons, x, y,",self.hud)
        self.assertIn("DesktopMenuMouseActionEnabled(",self.hud)
        self.assertIn("case latebrake::DesktopMenuMouseAction::Activate:",self.pawn)
        self.assertIn("Mode->MenuActivate();",self.pawn)

    def test_authoritative_select_keeps_existing_save_rollback(self):
        method=self.mode.split("void ALatebrakeRaceGameMode::GarageSelectViewedVehicle()",1)[1].split(
            "void ALatebrakeRaceGameMode::CommitGarageVehicleSelection(",1)[0]
        self.assertLess(method.index("RejectGarageMutationIfBlocked()"),
                        method.index("GarageSelectVehicle(GarageGallery.Viewed())"))
        direct=self.mode.split("void ALatebrakeRaceGameMode::GarageSelectVehicle(int32 VehicleId)",1)[1].split(
            "bool ALatebrakeRaceGameMode::CanCustomizeGarage()",1)[0]
        self.assertIn("PreviewGarageQuickSelect(",direct)
        self.assertIn("CommitGarageVehicleSelection(Decision.VehicleId)",direct)
        txn=self.mode.split("void ALatebrakeRaceGameMode::CommitGarageVehicleSelection(",1)[1].split(
            "void ALatebrakeRaceGameMode::GarageNextVehicle()",1)[0]
        for marker in ("Garage.Select(VehicleId)", "SavedProfile.Rollback()",
                       "ApplyGarageToPlayer()", "SavedProfile.Commit()",
                       "GarageGallery.Reset(Garage.SelectedVehicle())",
                       "SyncGarageShowroomPreview()"):
            self.assertIn(marker,txn)
        self.assertIn("add_latebrake_portable_research_test(GarageEquipUi)",self.cmake)
        self.assertIn("300000",self.tests)
        self.assertIn("EQUIP CAR",self.policy)

if __name__=="__main__":
    unittest.main()
