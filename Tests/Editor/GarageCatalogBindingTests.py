"""Source-level integration contract for the real Unreal Garage Canvas view.

These tests do not compile UnrealHeaderTool, inspect installed graphics, or
qualify an Unreal 5.8 package. The C++20 suite tests the actual pure policy.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageCatalogBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode_header = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy = read("Source/Latebrake/Public/Garage/GarageCatalogView.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.test_file = read("Tests/Standalone/GarageCatalogViewTests.cpp")

    def test_live_hud_receives_sanitized_read_only_snapshot(self):
        self.assertIn("GarageSnapshot GetGarageSnapshot() const", self.mode_header)
        self.assertIn("return Garage.Snapshot();", self.mode_header)
        self.assertIn('#include "Garage/GarageCatalogView.h"', self.hud)
        garage_page = self.hud.split(
            "case latebrake::MenuPage::Garage:", 1
        )[1].split("case latebrake::MenuPage::Results:", 1)[0]
        for marker in (
            "BuildGarageCatalogView(Mode->GetGarageSnapshot())",
            "Catalog.Cars", "Entry.VehicleId", "Entry.Name",
            "GarageCatalogOwnership::Equipped",
            "GarageCatalogOwnership::Owned",
            "GarageCatalogOwnership::LockedAffordable",
            "GarageCatalogOwnership::LockedInsufficientCredits",
            "Catalog.NextUnlockId", "Entry.RouteCreditsShort",
            "Entry.Effective.Acceleration", "Entry.Effective.Grip",
            "Entry.Effective.TopSpeed", "Mode->GetGarageFeedback()"
        ):
            self.assertIn(marker, garage_page)
        for forbidden in ("Garage.Unlock(", "Garage.Select(",
                          "Garage.Upgrade(", "SaveRecords(",
                          "ApplyGarageToPlayer(", "RestartRace(",
                          "RegisterCheckpoint(", "EarnCredits("):
            self.assertNotIn(forbidden, garage_page)
        self.assertLess(garage_page.index("Mode->GetGarageFeedback()"),
                        garage_page.index("for (const latebrake::GarageCatalogEntry&"),
                        "purchase confirmation remains visible near first page")

    def test_live_catalogue_discloses_sequential_budget_not_sticker_affordability(self):
        garage_page = self.hud.split("case latebrake::MenuPage::Garage:", 1
        )[1].split("case latebrake::MenuPage::Results:", 1)[0]
        self.assertIn("PlanGaragePurchasePath(garage.Snapshot()", self.policy)
        self.assertIn("entry.Effective = route.Projected;", self.policy)
        for marker in ("Q NEXT: %s | %d CR | NEED %d",
                       "Next.UnlockCost, Next.RouteCreditsShort",
                       "LOCKED ROWS SHOW TOTAL SEQUENTIAL ROUTE COST.",
                       "Entry.RouteValid", "Entry.RouteBudgetReady",
                       "Entry.RouteCars",
                       "VIA %d SEQUENTIAL Q UNLOCK(S)",
                       "ROUTE %d CR | BUDGET READY",
                       "ROUTE %d CR | NEED %d CR",
                       "Entry.RouteCost, Entry.RouteCreditsShort"):
            self.assertIn(marker, garage_page)
        self.assertNotIn("LOCKED %d CR | AFFORDABLE", garage_page)
        self.assertNotIn("Entry.CreditsShort", garage_page)
        self.assertIn("Nova sticker affordability is not sequential route affordability",
                      self.test_file)
        self.assertIn("complete unlock route matches independent ownership/cost oracle",
                      self.test_file)

    def test_catalogue_uses_existing_paginated_menu_and_no_save_schema(self):
        self.assertIn("MenuPager.Synchronize(Mode->GetMenuPage()", self.hud)
        self.assertIn("MenuPager.Window()", self.hud)
        self.assertIn("GarageState garage;", self.policy)
        self.assertIn("garage.Load(persisted);", self.policy)
        self.assertIn("GarageCatalogView", self.cmake)
        self.assertIn('#include "Garage/GarageCatalogView.h"', self.test_file)
        self.assertIn("original three-car save", self.test_file)
        for mutation in ("SaveRecords(", "garage.Unlock(", "garage.Select(",
                         "garage.Upgrade(", "garage.EarnCredits("):
            self.assertNotIn(mutation, self.policy)


if __name__ == "__main__":
    unittest.main()
