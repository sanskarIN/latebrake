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
            "Garage.Owns(Selected)",
            "GaragePurchaseConfirm.Reset();",
            "const latebrake::GarageAppearanceState Before = GarageLooks;",
            "GarageLooks.Cycle(Selected, layer)",
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
        self.assertIn("F5 PAINT | F6 STRIPES | F7 RIMS", self.hud)
        self.assertIn("SAVED LOOK: %s / %s / %s", self.hud)

    def test_pointer_buttons_share_accurate_hit_bounds(self):
        for token in (
            "CalculateGarageStyleMouseLayout(",
            "HitTestGarageStyleMouse(",
            "page != MenuPage::Garage",
            "if (!dock.Valid) return {};",
            "DesktopMenuMouseAction::GaragePaint",
            "DesktopMenuMouseAction::GarageStripe",
            "DesktopMenuMouseAction::GarageWheels",
            "button.Bounds.Contains(x, y)",
        ):
            self.assertIn(token, self.mouse)
        self.assertIn("latebrake::HitTestGarageStyleMouse(style, x, y)", self.hud)
        self.assertIn("latebrake::HitTestGarageStyleMouse(", self.hud)
        for token in (
            "case latebrake::DesktopMenuMouseAction::GaragePaint:",
            "case latebrake::DesktopMenuMouseAction::GarageStripe:",
            "case latebrake::DesktopMenuMouseAction::GarageWheels:",
        ):
            self.assertIn(token, self.pawn)
        self.assertIn("if (GarageStyleMenu.Valid)", self.hud)


if __name__ == "__main__":
    unittest.main()
