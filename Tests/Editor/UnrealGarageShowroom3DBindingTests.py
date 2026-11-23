"""Production Unreal viewed-car showroom bridges with no racing authority.

These inspect real source and pure C++ showroom transition tests.
They are not UE5.8 UHT, Editor, shader or installed Win64 verification.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def source(path):
    return (ROOT/path).read_text(encoding="utf-8")

class UnrealGarageShowroom3DBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn=source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.h=source("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.mode=source("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.mode_h=source("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.hud=source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy=source("Source/Latebrake/Public/Vehicles/GarageShowroomPresentation.h")
        cls.tests=source("Tests/Standalone/GarageShowroomPresentationTests.cpp")
        cls.cmake=source("Tests/Standalone/CMakeLists.txt")
        cls.qa=source("docs/PR452_UNREAL_3D_PLAYTEST.md")

    def test_transient_render_only_pawn_state(self):
        for key in (
            "UpdateGarageShowroomPresentation(",
            "GetPresentedGarageVehicleId() const noexcept",
            "GetSelectedVehicleId() const { return GarageVehicleId; }",
            "bGarageShowroomPreviewActive",
            "GarageShowroomVehicleId",
            "GarageShowroomLook",
            "GetPresentedGarageLook() const noexcept",
        ):
            self.assertIn(key,self.h)
        method=self.pawn.split(
            "void ALatebrakeCarPawn::UpdateGarageShowroomPresentation(",1
        )[1].split("void ALatebrakeCarPawn::ClearDriverInputs()",1)[0]
        for key in (
            "PlanGarageShowroomPresentation(",
            "bGarageShowroomPreviewActive=Plan.PreviewActive;",
            "GarageShowroomVehicleId=Plan.DisplayVehicleId;",
            "GarageShowroomLook=Plan.DisplayLook;",
            "if (Plan.ShapeChanged)",
            "ApplyVehicleVisualRecipe(Plan.DisplayVehicleId);",
            "if (Plan.RebuildSourceMesh)",
            "RebuildOriginalVehicleVisual(Plan.DisplayVehicleId);",
            "if (Plan.RefreshMaterials)",
            "ApplyVehiclePaintRecipe(Plan.DisplayVehicleId);",
        ):
            self.assertIn(key,method)
        for forbidden in (
            "GarageVehicleId =", "CurrentGarageLook =", "Garage.Select(",
            "SaveRecords(", "Garage.Unlock(", "EarnCredits(", "AddForce(",
            "SetMassOverrideInKg(", "SetPhysicsLinearVelocity(",
        ):
            self.assertNotIn(forbidden,method)

    def test_authoritative_car_changes_are_visually_deferred_in_showroom(self):
        tune=self.pawn.split(
            "void ALatebrakeCarPawn::SetGarageTuning(",1
        )[1].split("void ALatebrakeCarPawn::SetGarageAppearance(",1)[0]
        self.assertIn("GarageVehicleId = SafeId;",tune)
        self.assertIn("CurrentGarageLook = Transition.Look;",tune)
        self.assertIn("if (!bGarageShowroomPreviewActive)",tune)
        appearance=self.pawn.split(
            "void ALatebrakeCarPawn::SetGarageAppearance(",1
        )[1].split("void ALatebrakeCarPawn::UpdateGarageShowroomPresentation(",1)[0]
        self.assertIn("if (bGarageShowroomPreviewActive) return;",appearance)
        build=self.pawn.split(
            "void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1
        )[1].split("int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts()",1)[0]
        self.assertIn("GetPresentedGarageLook().Wheels",build)
        paint=self.pawn.split(
            "void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(",1
        )[1].split("void ALatebrakeCarPawn::RefreshFrontLampPositions()",1)[0]
        self.assertIn("GetPresentedGarageLook()",paint)

    def test_menu_transitions_restore_before_unpause(self):
        self.assertIn("void SyncGarageShowroomPreview();",self.mode_h)
        sync=self.mode.split(
            "void ALatebrakeRaceGameMode::SyncGarageShowroomPreview()",1
        )[1].split("bool ALatebrakeRaceGameMode::CanBrowseGarage()",1)[0]
        self.assertIn("Menu.Current()==latebrake::MenuPage::Garage",sync)
        self.assertIn("GarageGallery.Viewed()",sync)
        self.assertIn("GarageLooks.Get(Viewed)",sync)
        self.assertIn("Car->UpdateGarageShowroomPresentation(",sync)
        for first,last in (
            ("ToggleMenu","ToggleRacePause"),
            ("MenuNext","MenuPrevious"),
            ("MenuPrevious","MenuActivate"),
            ("GarageBrowseNext","GarageBrowseVehicle"),
            ("GarageBrowseVehicle","GarageBrowsePrevious"),
            ("GarageBrowsePrevious","GarageSelectViewedVehicle"),
        ):
            segment=self.mode.split(
                "void ALatebrakeRaceGameMode::"+first+"(",1
            )[1].split("void ALatebrakeRaceGameMode::"+last+"(",1)[0]
            self.assertIn("SyncGarageShowroomPreview();",segment,first)
        close=self.mode.split(
            "void ALatebrakeRaceGameMode::CloseMenuForRace(",1
        )[1].split("void ALatebrakeRaceGameMode::ToggleMenu(",1)[0]
        self.assertLess(close.index("Menu.Close();"),
                        close.index("SyncGarageShowroomPreview();"))
        self.assertLess(close.index("SyncGarageShowroomPreview();"),
                        close.index("SetGamePaused(this, false)"))
        apply=self.mode.split(
            "void ALatebrakeRaceGameMode::ApplyGarageToPlayer()",1
        )[1].split("void ALatebrakeRaceGameMode::SyncGarageShowroomPreview()",1)[0]
        self.assertIn("SyncGarageShowroomPreview();",apply)

    def test_showroom_styling_and_rollback_preview_without_new_authority(self):
        method=self.mode.split(
            "void ALatebrakeRaceGameMode::GarageCycleAppearance(",1
        )[1].split("void ALatebrakeRaceGameMode::GarageCyclePaint()",1)[0]
        self.assertIn("GarageLooks = Before;",method)
        self.assertIn("SyncGarageShowroomPreview();",method)
        self.assertEqual(method.count("if (Viewed == Garage.SelectedVehicle())"),2)
        browsing=self.mode.split(
            "void ALatebrakeRaceGameMode::GarageBrowseNext(",1
        )[1].split("void ALatebrakeRaceGameMode::GarageSelectViewedVehicle(",1)[0]
        for forbidden in ("Garage.Select(", "Garage.Unlock(", "SaveRecords(",
                          "SetGarageTuning(", "EarnCredits("):
            self.assertNotIn(forbidden,browsing)
        self.assertIn("LIVE SHOWROOM:",self.hud)
        self.assertIn("GetPresentedGarageVehicleId()",self.hud)
        self.assertIn("VISUAL ONLY",self.hud)
        self.assertIn("LEGACY CUBE FALLBACK",self.hud)
        self.assertIn("NAME GarageShowroomPresentation",self.cmake)
        self.assertIn("AllSixCarEntryExitPairs();",self.tests)
        self.assertIn("PlanGarageVisualTransition(",self.policy)
        self.assertIn("showroom",self.qa.lower())

if __name__=="__main__":
    unittest.main()
