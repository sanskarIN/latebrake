"""Source integration contracts; not an Unreal 5.8 gameplay test."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CAR_H = (ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h").read_text(encoding="utf-8")
CAR_CPP = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
HUD = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")


class ArcadeBrakeHeatBindingTests(unittest.TestCase):
    def test_shared_player_rival_pawn_owns_thermal_state(self):
        self.assertIn('#include "Vehicles/ArcadeBrakeHeat.h"', CAR_H)
        self.assertIn("latebrake::ArcadeBrakeHeat BrakeHeat;", CAR_H)
        self.assertIn("GetBrakeHeat()", CAR_H)
        self.assertIn("BrakeHeat.Reset();", CAR_H)
        self.assertIn("BrakeHeat.Suspend();", CAR_CPP)

    def test_physically_observed_braking_affects_service_resistance(self):
        self.assertIn("const bool bServiceBraking = bBraking || TouchDrive.BrakeActive();", CAR_CPP)
        self.assertIn("BrakeHeat.Observe({", CAR_CPP)
        self.assertIn("static_cast<double>(LongitudinalSpeed), bServiceBraking, true", CAR_CPP)
        self.assertIn("BrakeProfile.HandbrakeActive ? 4.2 : 0.0", CAR_CPP)
        self.assertIn("BrakeRate * Thermal.ServiceBrakeMultiplier", CAR_CPP)
        self.assertIn("FrameStableDampingRate(EffectiveBrakeRate, DeltaSeconds)", CAR_CPP)

    def test_only_expanded_hud_shows_advisory_heat(self):
        self.assertIn("Car->GetBrakeHeat().HasValidSample()", HUD)
        self.assertIn("BRAKE HEAT", HUD)
        self.assertIn("!bCompact", HUD)


if __name__ == "__main__":
    unittest.main()
