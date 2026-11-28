"""Verify the live chooser and original mesh share the same appearance recipes."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageAppearanceGlyphBindingTests(unittest.TestCase):
    def test_native_rim_geometry_and_schematic_share_authored_parameters(self):
        mesh = read("Source/Latebrake/Public/Vehicles/OriginalVehicleSurfaceMesh.h")
        recipe = read("Source/Latebrake/Public/Vehicles/OriginalVehicleRimRecipe.h")
        glyph = read("Source/Latebrake/Public/UI/GarageAppearanceGlyph.h")
        self.assertIn('#include "Vehicles/OriginalVehicleRimRecipe.h"',mesh)
        self.assertIn('#include "Vehicles/OriginalVehicleRimRecipe.h"',glyph)
        self.assertIn("OriginalRimRecipeForStyle(rimStyle)",mesh)
        self.assertIn("OriginalRimRecipeForStyle(look.Wheels)",glyph)
        for field in ("Spokes","HalfAngle","InnerFraction","OuterFraction"):
            self.assertIn("recipe."+field,mesh)
            self.assertIn("recipe."+field,glyph)
        for value in ("{7,0.09,0.16,0.91}","{5,0.17,0.12,0.94}","{12,0.065,0.22,0.97}"):
            self.assertIn(value,recipe)

    def test_shared_finishes_used_by_every_native_presentation(self):
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        glyph = read("Source/Latebrake/Public/UI/GarageAppearanceGlyph.h")
        for name in ("RimTintForGarageLook","StripeTintForGarageLook"):
            self.assertIn("latebrake::"+name+"(Look)",pawn)
            self.assertIn("latebrake::"+name+"(PreviewLook)",hud)
            self.assertIn(name+"(look)",glyph)
        self.assertGreaterEqual(pawn.count("RimTintForGarageLook(Look)"),2)

    def test_live_chooser_reserves_icon_area_and_requires_visible_preview(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        panel = hud.split("// Direct choice overlay",1)[1].split("const auto OrbitButtons",1)[0]
        for marker in ("PreviewGarageDirectAppearance(", "Decision.Result,GaragePalette.Layer",
                       "CalculateGarageAppearanceGlyph(", "bPreviewDrawn = Glyph.Valid;",
                       "Accessory = IconSize+6.0*Layout.Scale;",
                       "j<Glyph.RectCount", "j<Glyph.LineCount",
                       "DrawLine(L.A.X,L.A.Y,L.B.X,L.B.Y,Color(L.Ink),L.Thickness)",
                       "Title,Ink,0.76f*UIScale,Accessory",
                       "Footer,Ink,0.62f*UIScale,Accessory",
                       "bTitle && bTag && bPreviewDrawn"):
            self.assertIn(marker,panel)
        for forbidden in ("Garage.Select(", "Garage.Unlock(", "SaveRecords(", "CreateMeshSection("):
            self.assertNotIn(forbidden,panel)
        cmake=read("Tests/Standalone/CMakeLists.txt")
        self.assertIn("add_latebrake_portable_research_test(GarageAppearanceGlyph)",cmake)


if __name__ == "__main__":
    unittest.main()
