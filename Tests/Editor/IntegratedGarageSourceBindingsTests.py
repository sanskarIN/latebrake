"""Regression gate for the combined Garage UI source integration.

Checks that original route-cost budgets and six-slot selection remain
simultaneously wired. These contracts do not replace Unreal 5.8 Editor,
GPU rendering, cooked Win64 gameplay, or real SaveGame failure testing.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class IntegratedGarageSourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.catalog = source("Source/Latebrake/Public/Garage/GarageCatalogView.h")
        cls.mode = source("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.pawn = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.config = source("Config/DefaultInput.ini")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")

    def test_showroom_retains_both_purchase_planner_and_keyboard_ux(self):
        page = self.hud.split("case latebrake::MenuPage::Garage:", 1)[1].split(
            "case latebrake::MenuPage::Results:", 1)[0]
        for essential in (
            "BuildGarageCatalogView(Mode->GetGarageSnapshot())",
            "Catalog.NextUnlockId", "Catalog.OwnedVehicleCount",
            "GARAGE CAR %d/%d | OWNED %d/%d", "Entry.RouteValid",
            "Entry.RouteBudgetReady", "Entry.RouteCost",
            "Entry.RouteCars", "Entry.RouteCreditsShort",
            "Q NEXT", "VS EQUIPPED", "CTRL+1..6: EQUIP OWNED CARS",
            "ROUTE %d CR | NEED %d CR", "ROUTE %d CR | BUDGET READY",
            "VIA %d SEQUENTIAL Q UNLOCK(S)",
            "Mode->GetGaragePurchaseDisplayFeedback()", "MenuLine(", "bTouchHUD",
            'FString::Printf(TEXT("CTRL+%d")',
            'FString::Printf(TEXT("%d.")',
        ):
            with self.subTest(marker=essential):
                self.assertIn(essential, page)
        self.assertIn("if (!bTouchHUD)", page)
        # Feedback is pinned even when menu detail pagination is not at 1/1.
        before_pager = self.hud.split(
            "MenuPager.Synchronize(Mode->GetMenuPage()", 1)[0]
        self.assertIn("if (Mode->GetMenuPage() == latebrake::MenuPage::Garage)",
                      before_pager)
        self.assertIn("DrawPinned(Mode->GetGaragePurchaseDisplayFeedback(), true);",
                      before_pager)
        self.assertLess(
            before_pager.index("DrawPinned(Mode->GetGaragePurchaseDisplayFeedback(), true);"),
            before_pager.index("switch (Mode->GetMenuPage())"))
        self.assertEqual(before_pager.count("DrawPinned(Mode->GetGaragePurchaseDisplayFeedback(), true);"), 1)
        self.assertLess(page.index("Mode->GetGaragePurchaseDisplayFeedback()"),
                        page.index("for (const latebrake::GarageCatalogEntry&"),
                        "transaction feedback must appear ahead of six car rows")
        for forbidden in ("Garage.Unlock(", "Garage.Upgrade(",
                          "Garage.Select(", "EarnCredits(", "SaveRecords("):
            self.assertNotIn(forbidden, page)

    def test_catalog_uses_one_planner_source_of_truth(self):
        for marker in (
            '#include "Garage/GaragePurchasePlanner.h"',
            "garage.Load(persisted);",
            "PlanGaragePurchasePath(garage.Snapshot()",
            "entry.RouteCost = route.UnlockCost;",
            "entry.RouteCreditsShort",
            "entry.RouteBudgetReady",
            "entry.Effective = route.Projected;",
            "entry.IsNextUnlock", "++view.OwnedVehicleCount",
        ):
            self.assertIn(marker, self.catalog)
        self.assertNotIn("garage.Unlock(", self.catalog)
        self.assertNotIn("garage.Upgrade(", self.catalog)

    def test_only_one_equipment_save_transaction(self):
        common = self.mode.split(
            "void ALatebrakeRaceGameMode::CommitGarageVehicleSelection(", 1)[1].split(
            "void ALatebrakeRaceGameMode::GarageNextVehicle()", 1)[0]
        cycle = self.mode.split(
            "void ALatebrakeRaceGameMode::GarageNextVehicle()", 1)[1].split(
            "void ALatebrakeRaceGameMode::GarageSelectVehicle(", 1)[0]
        slot = self.mode.split(
            "void ALatebrakeRaceGameMode::GarageSelectVehicle(", 1)[1].split(
            "void ALatebrakeRaceGameMode::GarageUnlockNext()", 1)[0]
        self.assertEqual(common.count("Garage.Select(VehicleId)"), 1)
        self.assertEqual(common.count("SaveRecords();"), 1)
        self.assertEqual(common.count("SavedProfile.Rollback();"), 1)
        self.assertIn("CommitGarageVehicleSelection(Next);", cycle)
        self.assertIn("CommitGarageVehicleSelection(Decision.VehicleId);", slot)
        self.assertIn("CanEditGarageDuringRace", slot)
        self.assertIn("PreviewGarageQuickSelect(", slot)
        self.assertIn("GaragePurchaseConfirm.Reset();", slot)

    def test_all_six_input_slots_are_single_dispatched_paused_only(self):
        keys = ("One", "Two", "Three", "Four", "Five", "Six")
        for slot, key in enumerate(keys, 1):
            name = f"GarageSlot{slot}"
            with self.subTest(slot=slot):
                self.assertEqual(self.config.count(
                    f'+ActionMappings=(ActionName="{name}",Key={key},bCtrl=True)'), 1)
                self.assertEqual(len(re.findall(
                    rf'BindAction\(\s*TEXT\("{name}"\)', self.pawn)), 1)
                self.assertIn(f"Slot{slot}.bExecuteWhenPaused = true;",
                              self.pawn)
                self.assertIn(f"SelectGarageSlot({slot-1});", self.pawn)
        method = self.pawn.split(
            "void ALatebrakeCarPawn::SelectGarageSlot(", 1)[1].split(
            "void ALatebrakeCarPawn::GarageSlot1()", 1)[0]
        self.assertIn("Mode->IsMenuOpen()", method)
        self.assertIn("Mode->GetMenuPage() == latebrake::MenuPage::Garage", method)

    def test_source_test_matrix_retains_both_feature_families(self):
        for name in (
            "GarageCatalogView", "GarageQuickSelect", "GaragePurchasePlanner",
            "GarageAppearance", "GarageGalleryCursor",
            "OriginalVehicleSurfaceShading", "OriginalVehicleObjPreview",
            "RecentLapReview", "WorldScaleVariantReferenceClock",
        ):
            with self.subTest(name=name):
                self.assertIn(name, self.cmake)
        for exact in (
            "GarageQuickSelect", "GarageCatalogView",
        ):
            # The multi-test foreach invocation registers each once.
            line = next(x for x in self.cmake.splitlines()
                        if x.startswith("foreach(name IN ITEMS GarageCatalogView"))
            self.assertEqual(line.split().count(exact), 1)


if __name__ == "__main__":
    unittest.main()
