"""Source contracts for the read-only factory schematic, not Unreal QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class GarageFactorySchematicBindingTests(unittest.TestCase):
    def test_draw_is_factory_only_and_desktop_only(self):
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")
        body = hud.split("// Read-only factory schematic for the equipped car.", 1)[1]
        body = body.split("// SAVE failures must stay visible", 1)[0]
        for marker in ("MenuPage::Garage", "MouseMenu.Valid",
                       "GetSelectedVehicleId()", "FactoryGarageLook(Vehicle)",
                       "CalculateGarageSchematic(", "Schematic.Valid",
                       "PaintForGarageLook(Vehicle, Look)", "FACTORY SCHEMATIC",
                       "GetTextSize(", "Schematic.Panel.Width",
                       "DrawBounds(Piece.Bounds, Ink)", "#endif"):
            self.assertIn(marker, body)
        for marker in ("SaveRecords(", "Garage.Unlock(", "Garage.Select(",
                       "HitTestGarageStyleMouse(", "HitTestGarageQuickView(",
                       "MenuActivate(", "GarageUpgrade"):
            self.assertNotIn(marker, body)
        self.assertIn("#if !PLATFORM_ANDROID\n        // Read-only factory schematic", hud)

    def test_new_cosmetic_targets_are_not_dispatched_by_current_runtime(self):
        pawn = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
        click = pawn.split("void ALatebrakeCarPawn::MenuMousePressed()", 1)[1]
        click = click.split("\nvoid ALatebrakeCarPawn::", 1)[0]
        for action in ("GaragePaint", "GarageStripe", "GarageWheels",
                       "GarageReset", "GarageQuickView", "GarageOrbit"):
            self.assertNotIn(action, click)

if __name__ == "__main__":
    unittest.main()
