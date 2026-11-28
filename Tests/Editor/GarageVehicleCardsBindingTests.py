"""Garage vehicle grid source contracts; not an installed Unreal render test."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageVehicleCardsBindingTests(unittest.TestCase):
    def setUp(self):
        self.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.render = read("Source/Latebrake/Private/UI/LatebrakeGarageVehicleCardsHUD.cpp")
        self.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        self.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        self.policy = read("Source/Latebrake/Public/UI/GarageVehicleCards.h")

    def test_live_snapshot_and_cumulative_unlock_authority(self):
        block = self.mode.split("ALatebrakeRaceGameMode::GetGarageVehicleCardsSnapshot()", 1)[1]
        block = block.split("bool ALatebrakeRaceGameMode::BrowseGarageVehicleCard", 1)[0]
        for marker in ("Controller->IsLocalController()", "Controller->GetPawn()==Player",
                       "AppFocusGuard.IsSuspended()", "GarageGallery.Viewed()",
                       "SettingsSelection.Revision()", "Garage.Snapshot(),GarageLooks"):
            self.assertIn(marker, block)
        for marker in ("BuildGarageCatalogView(", "c.RouteCreditsShort", "c.IsNextUnlock",
                       "PaintForGarageLook(id,look).Body", "appearance.Load(s.Looks.data()"):
            self.assertIn(marker, self.policy)

    def test_card_click_is_only_preview(self):
        block = self.mode.split("bool ALatebrakeRaceGameMode::BrowseGarageVehicleCard(", 1)[1]
        block = block.split("bool ALatebrakeRaceGameMode::CanBrowseGarage()", 1)[0]
        for marker in ("Current!=Expected", "Current.CanBrowse()", "GarageBrowseVehicle(Id)"):
            self.assertIn(marker, block)
        for marker in ("Garage.Select(", "SaveRecords(", "Garage.Unlock(", "RestartRace("):
            self.assertNotIn(marker, block)
            self.assertNotIn(marker, self.render)
        self.assertIn("shown.Snapshot!=current", self.policy)
        self.assertIn("paletteOpen || !current.CanBrowse()", self.policy)

    def test_geometry_and_modal_receipts(self):
        self.assertIn("DrawnMenuRows+=VehicleCards.ReservedRows", self.hud)
        self.assertIn("Y=static_cast<float>(VehicleCards.NextDetailY)", self.hud)
        self.assertIn("PresentedGarageCards={};", self.hud)
        self.assertIn("vertical.OverflowNoticeY-1.25*step", self.policy)
        self.assertIn("Canvas->SizeX,Canvas->SizeY,X,Y,GaragePalette.Open", self.render)
        self.assertIn("Mode->BrowseGarageVehicleCard(Hit.Vehicle,Current)", self.render)
        self.assertLess(self.render.index("if(P.Name.Scale<=0 || P.State.Scale<=0 || P.Cost.Scale<=0)"),
                        self.render.index("Rect(Layout.Panel,Surface)"))
        self.assertLess(self.render.index("Presentation.Rendered[I]=true;"),
                        self.render.index("PresentedGarageCards=Presentation;"))
        self.assertLess(self.pawn.index("Hud->TryHandleGarageVehicleCardsPointer(MouseX,MouseY)"),
                        self.pawn.index("Hud->HitTestMenuMouse(MouseX, MouseY)"))
        self.assertIn("add_latebrake_portable_research_test(GarageVehicleCards)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
