"""Native showroom source contracts after reconciliation; no Unreal QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

class GarageRuntimeSchematicBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.arbitration = read("Source/Latebrake/Public/UI/GaragePointerArbitration.h")

    def test_geometry_and_pointer_admission_share_one_dispatch(self):
        hit = self.hud.split("ALatebrakeRaceHUD::HitTestMenuMouse(", 1)[1]
        hit = hit.split("void ALatebrakeRaceHUD::DrawHUD()", 1)[0]
        for marker in ("CalculateDesktopMenuMouseLayout(",
                       "CalculateGarageStyleMouseLayout(",
                       "CalculateGarageShowroomLane(",
                       "CalculateGarageOrbitButtons(",
                       "CalculateGarageSchematic(",
                       "CalculateGarageQuickViewLayout(",
                       "CollectGaragePointerClaims(buttons, style, quick, orbit,",
                       "ResolveGaragePointerClaims(",
                       "Mode->CanBrowseGarage()", "Mode->CanCustomizeGarage()",
                       "car && car->CanSelectGarageCameraOrbit()"):
            self.assertIn(marker, hit)
        for marker in ("HitTestMenuMouse(CursorX, CursorY)",
                       "HitTestMenuMouse(PickX, PickY)",
                       "HitTestMenuMouse(OrbitMouseX, OrbitMouseY)"):
            self.assertIn(marker, self.hud)
        for marker in ("HitTestDesktopMenuMouse(navigation, x, y,",
                       "HitTestGarageStyleMouse(style, x, y, true, true)",
                       "HitTestGarageQuickView(quick, x, y, true)",
                       "HitTestGarageOrbitButtons(orbit, x, y, true)",
                       "ValidGarageGroupClaim(", "claim.Authorized",
                       "if (action != DesktopMenuMouseAction::None)"):
            self.assertIn(marker, self.arbitration)

    def test_all_mutation_entries_use_shared_gate(self):
        for method in ("GarageNextVehicle", "GarageSelectVehicle",
                       "GarageSelectViewedVehicle", "GarageUnlockNext",
                       "GarageUpgradeEngine", "GarageUpgradeGrip",
                       "GarageCycleAppearance"):
            with self.subTest(method=method):
                body = self.mode.split("void ALatebrakeRaceGameMode::" + method + "(", 1)[1]
                body = body.split("{", 1)[1].lstrip()
                self.assertTrue(body.startswith("if (RejectGarageMutationIfBlocked()) return;"))
        gate = self.mode.split("ALatebrakeRaceGameMode::GetGarageMutationUiState() const", 1)[1]
        gate = gate.split("bool ALatebrakeRaceGameMode::RejectGarageMutationIfBlocked()", 1)[0]
        for marker in ("EvaluateGarageMutationUi(", "Menu.IsOpen()", "Menu.Current()",
                       "IsFocusLossGuardActive()", "IsManualRacePauseActive()",
                       "bFutureSaveSchema", "CanEditGarageDuringRace("):
            self.assertIn(marker, gate)
        rejection = self.mode.split("ALatebrakeRaceGameMode::RejectGarageMutationIfBlocked()", 1)[1]
        rejection = rejection.split("FString ALatebrakeRaceGameMode::GetGaragePurchaseDisplayFeedback()", 1)[0]
        self.assertIn("GaragePurchaseConfirm.Reset();", rejection)

    def test_view_only_actions_never_spend_or_equip(self):
        view = self.mode.split("void ALatebrakeRaceGameMode::GarageBrowseVehicle(", 1)[1]
        view = view.split("void ALatebrakeRaceGameMode::GarageBrowsePrevious()", 1)[0]
        self.assertIn("GarageGallery.Browse(VehicleId)", view)
        self.assertIn("SyncGarageShowroomPreview();", view)
        for forbidden in ("SaveRecords(", "Garage.Unlock(", "Garage.Select(",
                          "CommitGarageVehicleSelection(", "Garage.Upgrade("):
            self.assertNotIn(forbidden, view)
        click = self.pawn.split("void ALatebrakeCarPawn::MenuMousePressed()", 1)[1]
        click = click.split("\nvoid ALatebrakeCarPawn::", 1)[0]
        self.assertIn("Mode->GarageBrowseVehicle(", click)
        self.assertIn("GarageQuickViewIndex(Action)", click)
        self.assertNotIn("SaveRecords(", click)

if __name__ == "__main__":
    unittest.main()
