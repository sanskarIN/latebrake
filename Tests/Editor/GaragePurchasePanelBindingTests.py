"""Verify that desktop purchase offers share the existing consent and save path."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GaragePurchasePanelBindingTests(unittest.TestCase):
    def test_display_and_hit_regions_share_one_layout(self):
        hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        hit = hud.split("ALatebrakeRaceHUD::HitTestMenuMouse(", 1)[1].split(
            "void ALatebrakeRaceHUD::DrawHUD()", 1)[0]
        for marker in ("CalculateGaragePurchaseMouseLayout(",
                       "Mode->GetGaragePurchasePanel()",
                       "CanClickPresentedGarageOffer(",
                       "PresentedGarageOffers[0]",
                       "offers[0].Enabled()", "offers[1].Enabled()", "offers[2].Enabled()",
                       "CollectGaragePointerClaims(buttons, style, quick, orbit, purchase,"):
            self.assertIn(marker, hit)
        self.assertIn("const auto PurchaseButtons = latebrake::CalculateGaragePurchaseMouseLayout(", hud)
        self.assertIn("const auto Offers = Mode->GetGaragePurchasePanel();", hud)
        self.assertIn("HitTestMenuMouse(OfferX, OfferY)", hud)
        self.assertIn("PresentedGarageOffers = {};", hud)
        self.assertIn("PresentedGarageOffers[i].Visible = bTitleDrawn && bStatusDrawn;", hud)
        self.assertIn("DrawButtonCaption(Title,FString(),Upper", hud)
        self.assertIn('TEXT("CONFIRM %.1fs")', hud)
        self.assertIn('TEXT("NEED %d")', hud)

    def test_view_is_read_only_and_context_gated(self):
        mode = source("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        body = mode.split("ALatebrakeRaceGameMode::GetGaragePurchasePanel() const", 1)[1]
        body = body.split("FString ALatebrakeRaceGameMode::GetGaragePurchaseDisplayFeedback()",1)[0]
        for marker in ("Garage.Snapshot()", "GetGarageMutationUiState()",
                       "GaragePurchaseConfirm.Preview(Snapshot, FPlatformTime::Seconds())",
                       "BuildGaragePurchasePanel("):
            self.assertIn(marker, body)
        for forbidden in ("ConfirmOrStage(", "Garage.Unlock(", "Garage.Upgrade(",
                          "SaveRecords(", "ApplyGarageToPlayer("):
            self.assertNotIn(forbidden, body)

    def test_mouse_routes_to_authoritative_existing_handlers(self):
        pawn = source("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        click = pawn.split("void ALatebrakeCarPawn::MenuMousePressed()",1)[1]
        click = click.split("\nvoid ALatebrakeCarPawn::",1)[0]
        for action, handler in (("GarageUnlockOffer","GarageUnlockNext"),
                                ("GarageEngineOffer","GarageUpgradeEngine"),
                                ("GarageGripOffer","GarageUpgradeGrip")):
            self.assertIn("case latebrake::DesktopMenuMouseAction::"+action+":\n"
                          "        Mode->"+handler+"(); break;", click)
        self.assertNotIn("ConfirmOrStage(", click)
        self.assertNotIn("SaveRecords(", click)
        cmake = source("Tests/Standalone/CMakeLists.txt")
        self.assertIn("add_latebrake_portable_research_test(GaragePurchasePanel)",cmake)


if __name__ == "__main__":
    unittest.main()
