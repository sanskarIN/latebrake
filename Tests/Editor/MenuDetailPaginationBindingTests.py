"""v0.5.69: Real pause-menu paging is wired to input and HUD.

Portable source binding tests cannot substitute for actual UE 5.8
Editor/UHT compilation, runtime Canvas pixels, or Android touch delivery.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD_H = ROOT / "Source/Latebrake/Public/UI/LatebrakeRaceHUD.h"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
PAWN_H = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
INPUT = ROOT / "Config/DefaultInput.ini"
PAGER = ROOT / "Source/Latebrake/Public/UI/MenuDetailPager.h"
PORTABLE = ROOT / "Tests/Standalone/MenuDetailPagerTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class MenuDetailPaginationBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for label, path in (
            ("hud_h", HUD_H), ("hud", HUD), ("pawn_h", PAWN_H),
            ("pawn", PAWN), ("input", INPUT), ("pager", PAGER),
            ("portable", PORTABLE), ("cmake", CMAKE),
        ):
            setattr(cls, label, path.read_text(encoding="utf-8"))

    def test_unreal_hud_collects_every_detail_before_paging(self):
        for mark in (
            '#include "UI/MenuDetailPager.h"',
            "latebrake::MenuDetailPager MenuPager;",
            "void ScrollMenuDetails(int direction);",
        ):
            self.assertIn(mark, self.hud_h)
        for mark in (
            "TArray<TPair<FString, bool>> MenuDetails;",
            "MenuDetails.Emplace(Fitted, bAccent);",
            "MenuPager.Synchronize(Mode->GetMenuPage(), MenuDetails.Num()",
            "const latebrake::MenuDetailWindow DetailWindow = MenuPager.Window();",
            "Index = DetailWindow.Begin;", "Index < DetailWindow.End;",
            "DetailWindow.HasPrevious || DetailWindow.HasNext",
            "DETAIL PAGE %d/%d | UP/DOWN",
            "MenuPager.Next();", "MenuPager.Previous();", "MenuPager.Reset();",
            "DrawMenuPad(Pads.Restart, TEXT(\"UP\"));",
            "DrawMenuPad(Pads.Recover, TEXT(\"DOWN\"));",
            "MenuPlan.SaveWarningY",
        ):
            self.assertIn(mark, self.hud)
        self.assertLess(self.hud.index("MenuDetails.Emplace("),
                        self.hud.index("switch (Mode->GetMenuPage())"))
        self.assertLess(self.hud.index("switch (Mode->GetMenuPage())"),
                        self.hud.index("MenuPager.Synchronize("))
        self.assertLess(self.hud.index("MenuPager.Synchronize("),
                        self.hud.index("DetailWindow.Begin"))
        self.assertNotIn('MenuLine(TEXT("WARNING: PROGRESS NOT SAVED")', self.hud)

    def test_menu_scroll_does_not_trigger_racing_actions(self):
        for s in (
            '#include "UI/LatebrakeRaceHUD.h"',
            "void ALatebrakeCarPawn::MenuDetailsNext()",
            "void ALatebrakeCarPawn::MenuDetailsPrevious()",
            "Mode->IsMenuOpen()", "PC->GetHUD()",
            "Hud->ScrollMenuDetails(1);", "Hud->ScrollMenuDetails(-1);",
            "Region == latebrake::TouchControl::Restart) MenuDetailsPrevious();",
            "Region == latebrake::TouchControl::Recover) MenuDetailsNext();",
            "DetailsNext.bExecuteWhenPaused = true;",
            "DetailsPrevious.bExecuteWhenPaused = true;",
        ):
            self.assertIn(s, self.pawn)
        for action in ("MenuDetailsNext", "MenuDetailsPrevious"):
            self.assertIn(f"void {action}();", self.pawn_h)
        self.assertIn('ActionName="MenuDetailsNext",Key=Down', self.input)
        self.assertIn('ActionName="MenuDetailsPrevious",Key=Up', self.input)
        self.assertIn('ActionName="MenuNext",Key=PageDown', self.input)
        self.assertIn('ActionName="MenuPrevious",Key=PageUp', self.input)
        self.assertIn("if (Mode && Mode->IsMenuOpen())", self.pawn)
        self.assertLess(
            self.pawn.index("if (Mode && Mode->IsMenuOpen())"),
            self.pawn.index(
                "Region == latebrake::TouchControl::Restart) MenuDetailsPrevious();"))

    def test_portable_policy_is_fail_closed_and_cross_platform(self):
        for marker in (
            "void Synchronize(MenuPage page, int total, int capacity)",
            "CurrentPage != page", "SelectedPage = 0;",
            "std::clamp(total, 0, 1024)",
            "std::clamp(capacity, 0, 100)",
            "bool Next() noexcept", "bool Previous() noexcept",
            "MenuDetailWindow Window() const noexcept",
            "MenuPage::Closed",
        ):
            self.assertIn(marker, self.pager)
        for forbidden in (
            "PassCheckpoint(", "TryRecoverTo(", "AddTimePenalty(",
            "SaveRecords(", "SetActorLocation(", "HitTestTouchControl(",
            "GarageUpgrade", "CareerAward(",
        ):
            self.assertNotIn(forbidden, self.pager)
        for marker in (
            "MenuDetailPagerTests.cpp", "NAME MenuDetailPager",
            "cxx_std_20", "/W4 /WX", "-Wpedantic -Werror",
        ):
            self.assertIn(marker, self.cmake)
        for marker in (
            "MenuPage::Results", "MenuPage::Closed", "MenuPage::Garage",
            "pager.Next()", "pager.Previous()", "pager.Reset()",
            "1000000", "PageCount == 4",
        ):
            self.assertIn(marker, self.portable)


if __name__ == "__main__":
    unittest.main()
