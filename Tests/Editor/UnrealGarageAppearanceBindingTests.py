"""Source contracts for production Unreal garage appearance integration.

These checks verify real code paths exist and are connected, not that
Unreal Editor/UHT/cooked Win64 hardware tests were executed. Strict
C++20 garage state and desktop-pointer tests run separately via CTest.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "Source/Latebrake"
POLICY = BASE / "Public/Garage/GarageAppearance.h"
SAVE = BASE / "Public/Progression/LatebrakePlayerSave.h"
MODE_H = BASE / "Public/Game/LatebrakeRaceGameMode.h"
MODE_C = BASE / "Private/Game/LatebrakeRaceGameMode.cpp"
PAWN_H = BASE / "Public/Vehicles/LatebrakeCarPawn.h"
PAWN_C = BASE / "Private/Vehicles/LatebrakeCarPawn.cpp"
HUD = BASE / "Private/UI/LatebrakeRaceHUD.cpp"
MOUSE = BASE / "Public/UI/HudMenuMouseControls.h"
INPUT = ROOT / "Config/DefaultInput.ini"
TEST_CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class UnrealGarageAppearanceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.save = SAVE.read_text(encoding="utf-8")
        cls.mode_h = MODE_H.read_text(encoding="utf-8")
        cls.mode = MODE_C.read_text(encoding="utf-8")
        cls.pawn_h = PAWN_H.read_text(encoding="utf-8")
        cls.pawn = PAWN_C.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.mouse = MOUSE.read_text(encoding="utf-8")
        cls.input = INPUT.read_text(encoding="utf-8")
        cls.cmake = TEST_CMAKE.read_text(encoding="utf-8")

    def test_six_original_ue_ids_untouched(self):
        self.assertIn("static_assert(GarageAppearanceState::SerializedBytes == 18u", self.policy)
        self.assertIn("std::array<GarageLook, VehicleCount> Styles", self.policy)
        self.assertIn("GarageState::IsVehicle(id)", self.policy)
        self.assertIn("GaragePaintOptions = 8", self.policy)
        self.assertIn("GarageStripeOptions = 3", self.policy)
        self.assertIn("GarageWheelOptions = 3", self.policy)
        self.assertIn("bool Cycle(int id, GarageLookLayer layer)", self.policy)
        self.assertIn("bool Load(const std::uint8_t* raw, std::size_t count)", self.policy)
        self.assertIn("static constexpr std::uint32_t Checksum(", self.policy)
        self.assertIn("NAME GarageAppearance", self.cmake)
        self.assertIn("GarageAppearanceTests.cpp", self.cmake)

    def test_save_schema_preserves_old_career_and_ownership(self):
        for name in ("GarageAppearanceSchemaVersion", "GarageAppearanceBytes",
                     "GarageAppearanceChecksum"):
            self.assertIn("UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame", self.save)
            self.assertIn(name, self.save)
            self.assertIn("Save->" + name, self.mode)
        self.assertIn("Save->OwnedVehicleMask = Profile.OwnedVehicleMask;", self.mode)
        self.assertIn("Save->SelectedVehicleId = Profile.SelectedVehicle;", self.mode)
        self.assertIn("Save->CareerMedals.SetNum(latebrake::CareerEventCount)", self.mode)
        self.assertIn("GarageLooks.Load(packed.data(), packed.size())", self.mode)
        self.assertIn("GarageLooks.Reset();", self.mode)
        self.assertIn("Save->GarageAppearanceSchemaVersion >", self.mode)
        self.assertIn("bFutureSaveSchema = true;", self.mode)
        self.assertIn("Save->GarageAppearanceChecksum =\n        latebrake::GarageAppearanceState::Checksum(packedLooks)", self.mode)

    def test_actual_paid_write_rollback_and_page_authority(self):
        code = self.mode.split("void ALatebrakeRaceGameMode::GarageCycleAppearance(", 1)[1].split(
            "void ALatebrakeRaceGameMode::GarageUnlockNext()", 1)[0]
        for token in (
            "Menu.Current() != latebrake::MenuPage::Garage",
            "latebrake::CanEditGarageDuringRace",
            "CanStyleGalleryVehicle(Garage, GarageGallery)",
            "GaragePurchaseConfirm.Reset();",
            "const latebrake::GarageAppearanceState Before = GarageLooks;",
            "GarageLooks.Cycle(Viewed, layer)",
            "ApplyGarageToPlayer();",
            "SaveRecords();",
            "if (bProgressSaveFailed)",
            "GarageLooks = Before;",
            "Appearance NOT saved; previous car look restored.",
        ):
            self.assertIn(token, code)

    def test_real_pawn_material_and_geometry_updated(self):
        self.assertIn("void SetGarageAppearance(latebrake::GarageLook look);",
                      self.pawn_h)
        self.assertIn("void ALatebrakeCarPawn::SetGarageAppearance(", self.pawn)
        for token in (
            "PaintForGarageLook(VehicleId, Look)",
            'TEXT("RoofStripeLeft")',
            'TEXT("RoofStripeRight")',
            "Part->SetVisibility(Visible && Part->GetStaticMesh() != nullptr, true)",
            "Part->SetCollisionEnabled(ECollisionEnabled::NoCollision)",
            "ApplyStripe(RoofStripeLeft, Look.Stripe != 0u",
            "ApplyStripe(RoofStripeRight, Look.Stripe == 2u",
            "Look.Wheels == 2u",
            "FrontLeftSpoke.Get(), FrontRightSpoke.Get()",
            "Tint(Rim, RimRgb);",
            "if (HasActorBegunPlay()) ApplyVehiclePaintRecipe(GarageVehicleId)",
        ):
            self.assertIn(token, self.pawn)
        self.assertIn("Car->SetGarageAppearance(GarageLooks.Get(Garage.SelectedVehicle()))",
                      self.mode)
        self.assertLess(
            self.mode.index("Car->SetGarageTuning("),
            self.mode.index("Car->SetGarageAppearance("))
        self.assertNotIn("SetSimulatePhysics(true);\n        Tint(Rim", self.pawn)

    def test_keyboard_actions_only_work_in_garage(self):
        keys = {"F5": "GarageCyclePaint", "F6": "GarageCycleStripe",
                "F7": "GarageCycleWheels"}
        for key, action in keys.items():
            self.assertIn(f'+ActionMappings=(ActionName="{action}",Key={key})',
                          self.input)
            self.assertIn(f'TEXT("{action}")', self.pawn)
        for field in ("GaragePaintKey.bExecuteWhenPaused = true",
                      "GarageStripeKey.bExecuteWhenPaused = true",
                      "GarageWheelsKey.bExecuteWhenPaused = true"):
            self.assertIn(field, self.pawn)
        for method in ("GarageCyclePaint", "GarageCycleStripe",
                       "GarageCycleWheels"):
            self.assertIn("Mode->" + method + "();", self.pawn)
        self.assertIn("F5 PAINT | F6 STRIPES | F7 RIMS | F8 FACTORY", self.hud)
        self.assertIn("EQUIPPED LOOK: %s / %s / %s", self.hud)
        self.assertIn("VIEWED LOOK: %s / %s / %s", self.hud)

    def test_pointer_buttons_share_accurate_hit_bounds(self):
        for token in (
            "CalculateGarageStyleMouseLayout(",
            "HitTestGarageStyleMouse(",
            "page != MenuPage::Garage",
            "if (!dock.Valid) return {};",
            "DesktopMenuMouseAction::GaragePaint",
            "DesktopMenuMouseAction::GarageStripe",
            "DesktopMenuMouseAction::GarageWheels",
            "DesktopMenuMouseAction::GarageReset",
            "button.Bounds.Contains(x, y)",
        ):
            self.assertIn(token, self.mouse)
        self.assertIn("style, x, y, Mode->CanCustomizeGarage(),", self.hud)
        self.assertIn("Mode->CanBrowseGarage()", self.hud)
        self.assertIn("latebrake::HitTestGarageStyleMouse(", self.hud)
        for token in (
            "case latebrake::DesktopMenuMouseAction::GaragePaint:",
            "case latebrake::DesktopMenuMouseAction::GarageStripe:",
            "case latebrake::DesktopMenuMouseAction::GarageWheels:",
            "case latebrake::DesktopMenuMouseAction::GarageReset:",
        ):
            self.assertIn(token, self.pawn)
        self.assertIn("if (GarageStyleMenu.Valid)", self.hud)
        self.assertIn("const bool bCanEditLook = Mode->CanCustomizeGarage();", self.hud)
        self.assertIn("if (!layout.Valid || !std::isfinite(x)", self.mouse)
        self.assertIn("isBrowse ? browseEnabled : styleEnabled", self.mouse)

    def test_unequipped_owned_car_styling_never_changes_race_pawn(self):
        header = (BASE / "Public/Garage/GarageGalleryCursor.h").read_text(encoding="utf-8")
        self.assertIn("inline bool CanStyleGalleryVehicle(", header)
        self.assertIn("garage.Owns(gallery.Viewed())", header)
        method = self.mode.split(
            "void ALatebrakeRaceGameMode::GarageCycleAppearance(",1
        )[1].split("void ALatebrakeRaceGameMode::GarageCyclePaint()",1)[0]
        self.assertIn("const int32 Viewed = GarageGallery.Viewed();", method)
        self.assertNotIn("Garage.Select(", method)
        self.assertNotIn("Garage.Unlock(", method)
        self.assertIn("GarageLooks.Cycle(Viewed, layer)", method)
        self.assertIn("GarageLooks.ResetVehicle(Viewed)", method)
        self.assertEqual(method.count("if (Viewed == Garage.SelectedVehicle())"), 2)
        self.assertEqual(method.count("ApplyGarageToPlayer();"), 2)
        self.assertIn("SaveRecords();", method)
        self.assertIn("GarageLooks = Before;", method)
        self.assertLess(method.index("CanStyleGalleryVehicle("),
                        method.index("GarageLooks.Cycle(Viewed, layer)"))
        self.assertIn("VIEWED LOOK: %s / %s / %s", self.hud)
        self.assertIn("CUSTOM LOOK LOCKED: OWN THE VIEWED CAR FIRST", self.hud)
        self.assertIn("CanStyleGalleryVehicle(partiallyOwned, cursor)",
                      (ROOT / "Tests/Standalone/GarageGalleryCursorTests.cpp"
                       ).read_text(encoding="utf-8"))

    def test_live_dock_displays_viewed_cosmetics_and_factory_noop(self):
        self.assertIn("DescribeGarageStyleButton(", self.mouse)
        self.assertIn("GarageStyleButtonIndicator final", self.mouse)
        self.assertIn("SameGarageLook(look, FactoryGarageLook(viewedId))", self.mouse)
        self.assertIn("FACTORY OK", self.mouse)
        self.assertIn("RESET LOOK", self.mouse)
        self.assertIn("DescribeGarageStyleButton(", self.hud)
        self.assertIn("Mode->GetGaragePreviewVehicleId()", self.hud)
        self.assertIn("Mode->GetGaragePreviewLook()", self.hud)
        self.assertIn("Indicator.Ordinal, Indicator.Total", self.hud)
        self.assertIn("StylePaint.Body", self.hud)
        self.assertIn("12.f * UIScale, 8.f, 14.f", self.hud)
        self.assertIn("FACTORY LOOK ALREADY ACTIVE: no new save needed.", self.mode)
        style_method = self.mode.split(
            "void ALatebrakeRaceGameMode::GarageCycleAppearance(", 1
        )[1].split("void ALatebrakeRaceGameMode::GarageCyclePaint()", 1)[0]
        self.assertLess(style_method.index("if (!bChanged)"),
                        style_method.index("SaveRecords();"))
        self.assertIn("if (SameGarageLook(current, factory)) return false;", self.policy)
        self.assertIn("TestGarageStyleIndicators();",
            (ROOT / "Tests/Standalone/HudMenuMouseControlsTests.cpp"
             ).read_text(encoding="utf-8"))

    def test_controller_style_shortcuts_pause_only(self):
        mapping = {
            "GarageCyclePaint": "Gamepad_RightThumbstick",
            "GarageCycleStripe": "Gamepad_LeftThumbstick",
            "GarageCycleWheels": "Gamepad_LeftShoulder",
        }
        for action, button in mapping.items():
            self.assertIn(
                f'+ActionMappings=(ActionName="{action}",Key={button})',
                self.input)
        self.assertIn("bool CanCustomizeGarage() const;", self.mode_h)
        self.assertIn(
            "bool ALatebrakeRaceGameMode::CanCustomizeGarage() const",
            self.mode)
        self.assertIn(
            "latebrake::CanStyleGalleryVehicle(Garage, GarageGallery)", self.mode)
        self.assertIn("PAD: R3 PAINT / L3 STRIPES / LB RIMS", self.hud)
        self.assertIn(
            '+ActionMappings=(ActionName="GarageResetLook",Key=F8)',
            self.input)
        self.assertIn("GarageResetKey.bExecuteWhenPaused = true", self.pawn)
        self.assertIn("Mode->GarageResetAppearance();", self.pawn)
        self.assertIn("GarageLooks.ResetVehicle(Viewed)", self.mode)
        self.assertIn("FACTORY LOOK RESTORED AND SAVED", self.mode)
    def test_gallery_is_read_only_and_separate_from_owned_selection(self):
        path = ROOT / "Source/Latebrake/Public/Garage/GarageGalleryCursor.h"
        source = path.read_text(encoding="utf-8")
        self.assertIn("class GarageGalleryCursor final", source)
        self.assertIn("static_assert(VehicleCount == 6", source)
        self.assertIn("int Viewed() const noexcept", source)
        self.assertIn("GarageGallery.Next()", self.mode)
        self.assertIn("CanStyleGalleryVehicle(Garage, GarageGallery)", self.mode)
        self.assertIn("const int32 Viewed = GarageGallery.Viewed();", self.mode)
        self.assertIn("LOCKED CAR: unlock this vehicle before styling.", self.mode)
        self.assertIn("CUSTOM LOOK LOCKED: OWN THE VIEWED CAR FIRST", self.hud)
        self.assertIn("GarageGallery.Previous()", self.mode)
        self.assertIn("CanEquipGalleryVehicle(Garage, GarageGallery)", self.mode)
        self.assertIn("void ALatebrakeRaceGameMode::GarageSelectViewedVehicle()", self.mode)
        self.assertIn("GarageSelectViewedVehicle();", self.mode)
        self.assertIn("ENTER/SELECT: EQUIP VIEWED OWNED CAR", self.hud)
        equip = self.mode.split(
            "void ALatebrakeRaceGameMode::GarageSelectViewedVehicle()", 1)[1].split(
            "void ALatebrakeRaceGameMode::GarageNextVehicle()", 1)[0]
        for token in (
            "GaragePurchaseConfirm.Reset();",
            "latebrake::CanEditGarageDuringRace",
            "if (bFutureSaveSchema)",
            "latebrake::CanEquipGalleryVehicle(Garage, GarageGallery)",
            "Garage.Select(Viewed)",
            "SaveRecords();",
            "if (bProgressSaveFailed)",
            "SavedProfile.Rollback();",
            "ApplyGarageToPlayer();",
            "SavedProfile.Commit();",
        ):
            self.assertIn(token, equip)
        self.assertIn("bool ALatebrakeRaceGameMode::CanBrowseGarage() const", self.mode)
        self.assertIn("GaragePurchaseConfirm.Reset();", self.mode)
        self.assertIn("GetGaragePreviewVehicleId()", self.hud)
        self.assertIn("GarageBrowsePrevious", self.mouse)
        self.assertIn("GarageBrowseNext", self.mouse)
        self.assertIn("bCanBrowse", self.hud)
        self.assertIn("case latebrake::DesktopMenuMouseAction::GarageBrowsePrevious:", self.pawn)
        self.assertIn("case latebrake::DesktopMenuMouseAction::GarageBrowseNext:", self.pawn)
        self.assertIn("PREVIEW EST.: ACCEL", self.hud)
        self.assertIn("LOCKED / PREVIEW ONLY", self.hud)
        self.assertIn("VIEWING", self.hud)
        self.assertIn("GarageGalleryCursorTests.cpp", self.cmake)
        for name, key in (("GarageBrowseNext", "F9"),
                          ("GarageBrowsePrevious", "F10"),
                          ("GarageBrowseNext", "Gamepad_RightShoulder"),
                          ("GarageBrowsePrevious", "Gamepad_FaceButton_Right")):
            self.assertIn(
                f'+ActionMappings=(ActionName="{name}",Key={key})',
                self.input)
        self.assertIn("BrowseNextKey.bExecuteWhenPaused = true", self.pawn)
        self.assertIn("BrowsePreviousKey.bExecuteWhenPaused = true", self.pawn)
        self.assertIn("Mode->GarageBrowseNext();", self.pawn)
        self.assertIn("Mode->GarageBrowsePrevious();", self.pawn)


if __name__ == "__main__":
    unittest.main()
