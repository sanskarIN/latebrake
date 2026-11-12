"""Source-only binding of original Unreal GameMode to proposed six-slot grid.

This does not build Unreal Engine, cook a map, or spawn additional opponents.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class CircuitStartGridBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.game = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.header = read("Source/Latebrake/Public/Track/CircuitStartGrid.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.tests = read("Tests/Standalone/CircuitStartGridTests.cpp")

    def test_existing_physical_player_and_primary_rival_share_grid(self):
        self.assertIn('#include "Track/CircuitStartGrid.h"', self.game)
        player = self.game.split(
            "FVector ALatebrakeRaceGameMode::GetStartPosition()", 1)[1].split(
            "AActor* ALatebrakeRaceGameMode::ChoosePlayerStart_Implementation", 1)[0]
        self.assertIn("StartGridPoses[latebrake::HumanGridSlot]", player)
        self.assertIn("StartGridPoses[latebrake::PrimaryRivalGridSlot]", self.game)
        restart = self.game.split(
            "void ALatebrakeRaceGameMode::RestartRace()", 1)[1].split(
            "void ALatebrakeRaceGameMode::", 1)[0]
        refresh = self.game.split(
            "void ALatebrakeRaceGameMode::RefreshRival()", 1)[1].split(
            "void ALatebrakeRaceGameMode::RestartRace()", 1)[0]
        self.assertIn("StartGridPoses[latebrake::PrimaryRivalGridSlot]", restart)
        self.assertIn("StartGridPoses[latebrake::PrimaryRivalGridSlot]", refresh)
        self.assertIn("SpawnActor<ALatebrakeRivalCar>", refresh)
        self.assertEqual(refresh.count("SpawnActor<ALatebrakeRivalCar>"), 1)
        self.assertIn("Car->TryRecoverTo(GetStartPosition(), GetStartRotation())", restart)

    def test_no_unqualified_six_car_gameplay(self):
        self.assertIn("StartGridCapacity = 6", self.header)
        self.assertIn("PlayerStart,", self.header)
        self.assertIn("RivalStart,", self.header)
        self.assertIn("std::array<CircuitPose, StartGridCapacity>", self.header)
        self.assertIn("StartGridFootprintsOverlap(", self.header)
        self.assertIn("StartGridFootprintTouchesBarrier(", self.header)
        self.assertIn("StartGridFootprintOnPavement(", self.header)
        for forbidden in ("SpawnActor<", "UGameplayStatics", "AddForce(",
                          "SaveGame", "Race.Advance(", "RegisterCheckpoint("):
            self.assertNotIn(forbidden, self.header)
        self.assertIn("CircuitStartGridTests.cpp", self.cmake)
        self.assertIn("NAME CircuitStartGrid", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("3600", self.tests)


if __name__ == "__main__":
    unittest.main()
