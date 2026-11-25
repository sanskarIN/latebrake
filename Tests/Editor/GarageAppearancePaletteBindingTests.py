"""Native chooser bindings; actual UE rendering and cooked QA remain separate."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageAppearancePaletteBindingTests(unittest.TestCase):
    def setUp(self):
        self.hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.mode = source("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        self.pawn = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.policy = source("Source/Latebrake/Public/UI/GarageAppearancePalette.h")

    def test_mouse_opens_palette_and_keyboard_fallback_survives(self):
        click = self.pawn.split("void ALatebrakeCarPawn::MenuMousePressed()",1)[1]
        click = click.split("\nvoid ALatebrakeCarPawn::",1)[0]
        for name, layer, method in (
            ("GaragePaint","Paint","GarageCyclePaint"),
            ("GarageStripe","Stripe","GarageCycleStripe"),
            ("GarageWheels","Wheels","GarageCycleWheels"),
        ):
            self.assertIn("case latebrake::DesktopMenuMouseAction::"+name+":\n"
                          "        if (!Hud->ToggleGaragePalette(latebrake::GarageLookLayer::"+layer+"))\n"
                          "            Mode->"+method+"();",click)
        self.assertIn("Hud->ApplyGaragePaletteChoice(Action)",click)
        self.assertIn("Hud->CloseGaragePalette()",click)
        self.assertNotIn("SaveRecords(",click)

    def test_layout_and_rendered_captions_gate_hits(self):
        hit = self.hud.split("ALatebrakeRaceHUD::HitTestMenuMouse(",1)[1]
        hit = hit.split("void ALatebrakeRaceHUD::DrawHUD()",1)[0]
        for marker in ("GaragePalette.Matches(", "Mode->CanCustomizeGarage()",
                       "CalculateGaragePaletteLayout(", "HitTestGaragePalette(",
                       "CanClickPresentedGaragePalette(", "permission, paletteClaim"):
            self.assertIn(marker,hit)
        for marker in ("PresentedGaragePalette = {};", "PresentedGaragePalette.Layout = Palette;",
                       "PresentedGaragePalette.CloseVisible =",
                       "PresentedGaragePalette.ChoiceVisible[static_cast<std::size_t>(i)] = bTitle && bTag && bPreviewDrawn;",
                       "DescribeGaragePaletteChoice(", "Choice.Swatch.R", "Choice.Selected",
                       'TEXT("ACTIVE")', 'TEXT("FACTORY")'):
            self.assertIn(marker,self.hud)
        self.assertIn("before.Action==action&&now.Action==action",self.policy)
        self.assertIn("a.X==b.X&&a.Y==b.Y&&a.Width==b.Width&&a.Height==b.Height",self.policy)

    def test_direct_choice_uses_existing_save_rollback(self):
        entry = self.mode.split("void ALatebrakeRaceGameMode::GarageChooseAppearance(",1)[1]
        entry = entry.split("void ALatebrakeRaceGameMode::GarageCyclePaint()",1)[0]
        self.assertIn("GarageCycleAppearance(Request.Layer, false, &Request);",entry)
        self.assertNotIn("SaveRecords(",entry)
        transaction = self.mode.split("void ALatebrakeRaceGameMode::GarageCycleAppearance(",1)[1]
        transaction = transaction.split("void ALatebrakeRaceGameMode::GarageChooseAppearance(",1)[0]
        for marker in ("RejectGarageMutationIfBlocked()", "CanStyleGalleryVehicle(Garage, GarageGallery)",
                       "PreviewGarageDirectAppearance(", "GarageDirectAppearanceStatus::Ready",
                       "GarageLooks.Choose(Viewed, layer, Direct->Choice)",
                       "GarageLooks.ResetVehicle(Viewed)", "GarageLooks.Cycle(Viewed, layer)",
                       "if (!bChanged)", "SaveRecords();", "GarageLooks = Before;",
                       "SyncGarageShowroomPreview();"):
            self.assertIn(marker,transaction)
        self.assertEqual(transaction.count("SaveRecords();"),1)
        self.assertLess(transaction.index("CanStyleGalleryVehicle("),
                        transaction.index("GarageLooks.Choose("))
        self.assertLess(transaction.index("PreviewGarageDirectAppearance("),
                        transaction.index("GarageLooks.Choose("))
        self.assertLess(transaction.index("if (!bChanged)"),transaction.index("SaveRecords();"))
        for forbidden in ("Garage.Unlock(", "Garage.Select(", "Garage.Upgrade("):
            self.assertNotIn(forbidden,transaction)

    def test_focus_and_navigation_invalidate_immediately(self):
        for method in ("PauseForAppInterruption","CloseMenuForRace","ToggleMenu","MenuNext",
                       "MenuPrevious","MenuActivate","GarageBrowseNext","GarageBrowsePrevious",
                       "GarageBrowseVehicle","GarageSelectVehicle"):
            section = self.mode.split("void ALatebrakeRaceGameMode::"+method+"(",1)[1]
            section = section.split("\nvoid ALatebrakeRaceGameMode::",1)[0]
            self.assertIn("DismissGaragePalette();",section[:650],method)
        focus = self.mode.split("void ALatebrakeRaceGameMode::PauseForAppInterruption()",1)[1]
        self.assertLess(focus.index("DismissGaragePalette();"),focus.index("AppFocusGuard.OnFocusLost("))
        cmake = source("Tests/Standalone/CMakeLists.txt")
        self.assertIn("add_latebrake_portable_research_test(GarageAppearancePalette)",cmake)


if __name__ == "__main__":
    unittest.main()
