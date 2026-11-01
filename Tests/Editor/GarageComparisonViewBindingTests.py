"""Production Unreal garage UI read-only comparison source contract.

This is NOT an Unreal 5.8 compilation or a verified on-device rendering test.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class GarageComparisonViewBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy = read("Source/Latebrake/Public/Garage/GarageComparisonView.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_all_six_cars_have_compared_performance(self):
        self.assertIn('#include "Garage/GarageComparisonView.h"', self.hud)
        section = self.hud.split(
            "case latebrake::MenuPage::Garage:", 1
        )[1].split("case latebrake::MenuPage::Results:", 1)[0]
        for marker in (
            "BuildGarageCatalogView(Mode->GetGarageSnapshot())",
            "PreviewGarageUpgrade(Catalog.Credits, Catalog.EngineLevel,",
            "PreviewGarageUpgrade(Catalog.Credits, Catalog.GripLevel,",
            "CompareGaragePerformance(Catalog, Entry.VehicleId)",
            "Comparison.AccelerationPercent", "Comparison.GripPercent",
            "Comparison.TopSpeedPercent", "if (!bCompact)",
            "GarageUpgradeOfferStatus::Maxed",
            "GarageUpgradeOfferStatus::NeedCredits",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, section)
        self.assertLess(section.index("Mode->GetGarageFeedback()"),
                        section.index("for (const latebrake::GarageCatalogEntry&"))

    def test_no_hidden_purchase_or_save_authority(self):
        section = self.hud.split(
            "case latebrake::MenuPage::Garage:", 1
        )[1].split("case latebrake::MenuPage::Results:", 1)[0]
        for forbidden in (
            "Garage.Unlock(", "Garage.Select(", "Garage.Upgrade(",
            "SaveRecords(", "EarnCredits(", "ApplyGarageToPlayer(",
            "RegisterCheckpoint(", "RestartRace("
        ):
            self.assertNotIn(forbidden, section)
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("GarageState::UpgradeCost(kind, currentLevel)", self.policy)
        self.assertIn("catalog.Cars[static_cast<std::size_t>(catalog.SelectedVehicle)]",
                      self.policy)

    def test_cmake_warnings_as_errors(self):
        self.assertIn("GarageComparisonViewTests.cpp", self.cmake)
        self.assertIn("NAME GarageComparisonView", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)


if __name__ == "__main__":
    unittest.main()
