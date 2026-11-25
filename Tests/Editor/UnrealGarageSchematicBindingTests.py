"""Real Unreal HUD binding for a bounded noninteractive 2D showroom
schematic; source tests do NOT claim engine-side art/render verification."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source/Latebrake"
HUD = SRC / "Private/UI/LatebrakeRaceHUD.cpp"
MODE_H = SRC / "Public/Game/LatebrakeRaceGameMode.h"
POLICY = SRC / "Public/UI/GarageShowroomSchematic.h"
MOUSE = SRC / "Public/UI/HudMenuMouseControls.h"
TEST = ROOT / "Tests/Standalone/GarageShowroomSchematicTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
DOC = ROOT / "docs/NATIVE_GARAGE_RECONCILIATION_QA.md"


class UnrealGarageSchematicBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.mode = MODE_H.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.mouse = MOUSE.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.doc = DOC.read_text(encoding="utf-8")

    def test_gallery_identity_drives_view_not_equipped_physics(self):
        self.assertIn("latebrake::GarageLook GetGaragePreviewLook() const", self.mode)
        self.assertIn("GarageLooks.Get(GarageGallery.Viewed())", self.mode)
        self.assertIn('#include "UI/GarageShowroomSchematic.h"', self.hud)
        self.assertIn("Mode->GetGaragePreviewVehicleId()", self.hud)
        self.assertIn("Mode->GetGaragePreviewLook()", self.hud)
        self.assertIn("CalculateGarageSchematic(", self.hud)
        self.assertIn("PaintForGarageLook(\n                            PreviewVehicle, PreviewLook)", self.hud)
        diagram = self.hud.split("const auto Diagram = latebrake::CalculateGarageSchematic(", 1)[1].split(
            "// SAVE failures must stay visible", 1)[0]
        for forbidden in ("SaveRecords(", "Garage.Unlock(", "SetGarageTuning(",
                          "AddForce(", "SpawnActor", "CreateDefaultSubobject<"):
            self.assertNotIn(forbidden, diagram)

    def test_real_canvas_draws_all_layers_but_no_fake_3d_capture(self):
        for name in ("Shadow", "Tire", "Rim", "Body", "Accent",
                     "Canopy", "Stripe"):
            self.assertIn("GarageSchematicInk::" + name, self.hud)
            self.assertIn("GarageSchematicInk::" + name, self.policy)
        self.assertIn("CAR TOP VIEW | 2D SCHEMATIC", self.hud)
        self.assertIn("for (std::size_t i = 0; i < Diagram.Count; ++i)", self.hud)
        self.assertIn("DrawRect(Ink,", self.hud)
        self.assertNotIn("SceneCaptureComponent2D", self.policy)
        self.assertNotIn("RenderTarget", self.policy)

    def test_safe_geometry_uses_exact_nonoverlapping_style_dock(self):
        self.assertIn("const GarageStyleMouseLayout& styleDock", self.policy)
        self.assertIn("styleDock.Buttons[4].Bounds", self.policy)
        self.assertIn("styleDock.Buttons[5].Bounds", self.policy)
        for token in (
            "if (!styleDock.Valid || hud.Compact",
            "!GarageState::IsVehicle(vehicleId)",
            "!ValidGarageLook(look)",
            "viewportWidth < 900.0 || viewportHeight < 600.0",
            "r.Y + r.Height > top",
            "GarageSchematicPlan{}",
            "result.Pieces[result.Count++]",
            "result.Valid = true",
        ):
            self.assertIn(token, self.policy)
        self.assertIn("styleDock.Buttons", self.policy)

    def test_production_recipe_and_all_options_stay_bound(self):
        for recipe in ("VisualRecipeForVehicle(vehicleId)",
                       "BodyKitForVehicle(vehicleId)",
                       "GarageState::Definition(vehicleId)",
                       "look.Wheels == 2u",
                       "look.Stripe == 1u",
                       "look.Stripe == 2u",
                       "silhouette.HasRearWing",
                       "kit.HasHoodScoop",
                       "kit.HasRearFins"):
            self.assertIn(recipe, self.policy)
        self.assertIn("for (int paint = 0;paint < GaragePaintOptions;++paint)", self.test)
        self.assertIn("for (int stripe = 0;stripe < GarageStripeOptions;++stripe)", self.test)
        self.assertIn("for (int rims = 0;rims < GarageWheelOptions;++rims)", self.test)
        self.assertIn("add_latebrake_portable_research_test(GarageShowroomSchematic)", self.cmake)
        self.assertIn("GarageShowroomSchematicTests.cpp", str(TEST))
        self.assertIn("not native Unreal tests", self.doc)
        self.assertIn("actual UE5.8/UHT compile", self.doc)


if __name__ == "__main__":
    unittest.main()
