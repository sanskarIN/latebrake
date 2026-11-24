"""Static runtime/HUD wiring for bounded nitro status; never a UE runtime claim."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class NitroHudRuntimeBindingTests(unittest.TestCase):
    def test_actual_unreal_hud_observes_nitro_without_changing_physics(self):
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn('#include "UI/NitroHudReadout.h"', hud)
        section = hud.split("const latebrake::NitroHudReadout BoostStatus =", 1)[1].split(
            "Line((Car->IsHandbrakeHeld()", 1
        )[0]
        for term in (
            "latebrake::ReadNitroHud(",
            "Car->GetNitroBoost()",
            "Car->GetForwardSpeedKmH()",
            "!Mode->IsMenuOpen()",
            "!Mode->IsManualRacePauseActive()",
            "!UGameplayStatics::IsGamePaused(this)",
            "Mode->IsCircuitReadyForGameplay()",
            "!Race.IsFinished()",
            "NitroHudPhase::Cooldown",
            "NitroHudPhase::Overheated",
            "NitroHudPhase::BuildSpeed",
            "NitroHudPhase::Stored",
            "RawBoostCharge",
            "DisplayBoostCharge",
        ):
            with self.subTest(term=term):
                self.assertIn(term, section)
        for forbidden in (
            "NitroBoost.Reset(", "NitroBoost.Step(", "SaveRecords(",
            "EarnCredits(", "SetGarageTuning(", "SetGamePaused(",
        ):
            self.assertNotIn(forbidden, section)

    def test_real_strict_portable_tests_are_registered(self):
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(
            encoding="utf-8"
        )
        for target in (
            "HudDetailTextFlow", "HudInstrumentDriveBadge",
            "HudMenuReadablePanel", "NitroHudReadout",
            "GarageEquipUi", "GarageInputFocus",
            "OriginalVehicleIndexedRender", "OriginalVehicleRenderCache",
            "ProfileSaveWriteGuard", "PreviewDrivingCoach",
            "PreviewGarageBottomAction", "PreviewGaragePurchaseConsent",
            "PreviewGarageSelection", "PreviewGarageStyle",
            "PreviewKeyboardFocusGuard", "PreviewRaceTimeText",
            "PreviewSettingsPersistence", "PreviewViewportMath",
            "BestLapGhostYawContinuity", "RaceSessionCrossSystemSafety",
            "RivalPassGateRetry", "HudIntegratedReadouts",
            "HudIntegratedViewportSafety",
        ):
            with self.subTest(target=target):
                self.assertIn(f"({target})", cmake)


if __name__ == "__main__":
    unittest.main()
