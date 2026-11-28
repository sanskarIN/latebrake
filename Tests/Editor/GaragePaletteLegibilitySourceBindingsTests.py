"""Bind the authored-palette advice to the six-car Garage menu without
becoming a paint purchase, persistence, hit-test or real rendered-pixel claim.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT/relative).read_text(encoding="utf-8")


class GaragePaletteLegibilitySourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy=read("Source/Latebrake/Public/Garage/GaragePaletteLegibility.h")
        cls.source=read("Source/Latebrake/Public/Vehicles/VehiclePaintRecipe.h")
        cls.hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")

    def test_contrast_uses_exact_authored_source_tints(self):
        self.assertIn('"Vehicles/VehiclePaintRecipe.h"',self.policy)
        self.assertIn("PaintForGarageLook(vehicle,look)",self.policy)
        self.assertIn("StripeTintForGarageLook(look)",self.policy)
        self.assertIn("GarageSrgbLinearize",self.policy)
        self.assertIn("0.2126*GarageSrgbLinearize(color.R)",self.policy)
        self.assertIn("GarageContrastIllustrativeLargeShape=3.0",self.policy)
        self.assertIn("GarageContrastIllustrativeSmallText=4.5",self.policy)
        self.assertIn("GarageState::IsVehicle(vehicle)",self.policy)
        self.assertIn("ValidGarageLook(look)",self.policy)
        self.assertIn("case 6: return {\"GRAPHITE\"",self.source)
        self.assertIn("case 7: return {\"PEARL WHITE\"",self.source)

    def test_hud_only_shows_schematic_advice(self):
        self.assertIn('#include "Garage/GaragePaletteLegibility.h"',self.hud)
        garage_page=self.hud.split("case latebrake::MenuPage::Garage:",1)[1].split(
            "case latebrake::MenuPage::Results:",1)[0]
        self.assertIn("EvaluateGaragePaletteLegibility(PreviewId,ViewedLook)",garage_page)
        self.assertIn("SCHEMATIC LOOK",garage_page)
        self.assertIn("NOT A RENDERED CONTRAST AUDIT",garage_page)
        self.assertIn("GaragePaletteVisibilityLabel(",garage_page)
        self.assertIn("GaragePaletteInkLabel(",garage_page)
        for forbidden in ("Garage.Unlock(", "Garage.Choose(",
                          "Garage.EarnCredits(", "SaveRecords(",
                          "ApplyGarageToPlayer("):
            self.assertNotIn(forbidden,garage_page)
            self.assertNotIn(forbidden,self.policy)

    def test_invalid_or_manipulated_contrast_cannot_be_authoritative(self):
        for marker in ("IsOfficialAccessibilityAudit=false",
                       "UsesCalibratedDisplayEvidence=false",
                       "ChangesGarageEconomy=false",
                       "ChangesAppearance=false",
                       "ValidGaragePaletteLegibility(",
                       "CompareGaragePalettePaintOptions(",
                       "RequiresPlayerConsent=true"):
            self.assertIn(marker,self.policy)
        self.assertIn("GaragePaletteLegibilityTests.cpp",self.cmake)
        self.assertIn("NAME GaragePaletteLegibility",self.cmake)


if __name__=="__main__":
    unittest.main()
