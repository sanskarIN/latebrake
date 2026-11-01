"""Unreal career source-binding regression: all six garage car IDs are valid.

These checks are source-only. C++ portable tests validate the actual
eligibility, medal credits and persistence policies; installed UE playtests
remain mandatory.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealCareerSixCarBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.challenge = read("Source/Latebrake/Public/Career/CareerChallenges.h")
        cls.garage = read("Source/Latebrake/Public/Garage/GarageState.h")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_validation_uses_real_immutable_six_car_range(self):
        self.assertIn('#include "Garage/GarageState.h"', self.challenge)
        self.assertIn("!GarageState::IsVehicle(result.VehicleId)", self.challenge)
        self.assertNotIn("result.VehicleId > 2", self.challenge)
        self.assertIn("static_assert(VehicleCount == 6", self.garage)
        self.assertIn("return id >= 0 && id < VehicleCount;", self.garage)
        self.assertIn("event.RequiredVehicle == result.VehicleId", self.challenge)

    def test_unreal_career_finish_only_accepts_selected_starting_car(self):
        finish = self.mode.split(
            "const latebrake::CareerFinish Finish{", 1
        )[1].split("LastCareerAward = Career.CommitFinish", 1)[0]
        self.assertIn("Race.GetElapsedSeconds()", finish)
        self.assertIn("Race.GetCompletedLaps()", finish)
        self.assertIn("Garage.SelectedVehicle() == CareerStartedVehicleId", finish)
        self.assertIn("? CareerStartedVehicleId : -1", finish)
        self.assertIn("RecoveriesThisRace", finish)
        self.assertIn("true", finish)
        self.assertNotIn("EarnCredits(", finish)

    def test_eligible_six_car_test_target_remains_registered(self):
        self.assertIn("CareerSixCarEligibilityTests.cpp", self.cmake)
        self.assertIn("add_test(NAME CareerSixCarEligibility", self.cmake)


if __name__ == "__main__":
    unittest.main()
