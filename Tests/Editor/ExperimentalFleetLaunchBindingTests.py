"""Nonshipping six-actor launch gates and verified post-spawn physics.

Only checks exact source structure. Does not run Unreal, UHT or Win64.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class ExperimentalFleetLaunchBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.header = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")

    def test_actual_go_gate_refuses_unverified_extras(self):
        tick = self.mode.split(
            "void ALatebrakeRaceGameMode::Tick(float DeltaSeconds)", 1
        )[1].split("void ALatebrakeRaceGameMode::", 1)[0]
        for expected in (
            "bExperimentalSixCarGrid && bCircuitReady && bLocalPlayerReady",
            "!bInactive && !Race.IsFinished() && !StartLights.IsGreen()",
            "RefreshExperimentalRivals();",
            "const bool bFleetStartReady = !bExperimentalSixCarGrid ||",
            "IsExperimentalFleetReadyForStart()",
            "IsRestartPlacementReady() &&",
            "bFleetStartReady",
            "StartLights.Advance(",
        ):
            self.assertIn(expected, tick)
        self.assertLess(tick.index("RefreshExperimentalRivals();"),
                        tick.index("StartLights.Advance("))
        self.assertLess(tick.index("bFleetStartReady"),
                        tick.index("const bool bStartQualified"))
        self.assertLess(tick.index("const bool bStartQualified"),
                        tick.index("StartLights.Advance("))
        assert "IsExperimentalFleetReadyForStart() const;" in self.header
        qualify = self.mode.split(
            "bool ALatebrakeRaceGameMode::IsExperimentalFleetReadyForStart() const", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::RefreshExperimentalRivals()", 1
        )[0]
        for marker in (
            "IsExperimentalSixCarGridEnabled()",
            "!IsValid(RivalCar)", "!IsRegisteredRivalActor(RivalCar)",
            "!RivalCar->IsRacePhysicsReady()",
            "for (int32 Index = 0; Index < ExperimentalRivals.Num(); ++Index)",
            "Actor->GetGridSlot() != Index + 2",
            "!IsRegisteredRivalActor(Actor)",
            "!Actor->IsRacePhysicsReady()",
        ):
            self.assertIn(marker, qualify)

    def test_spawn_is_deferred_until_human_and_primary_real_physics(self):
        spawn = self.mode.split(
            "void ALatebrakeRaceGameMode::RefreshExperimentalRivals()", 1
        )[1].split(
            "bool ALatebrakeRaceGameMode::IsRegisteredRivalActor(", 1
        )[0]
        for expected in (
            "#if UE_BUILD_SHIPPING",
            "!HasLocalPlayerDriver()",
            "!IsRegisteredRivalActor(RivalCar)",
            "!RivalCar->IsRacePhysicsReady()",
            "ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding",
            "GetWorld()->SpawnActor<ALatebrakeRivalCar>(",
            "Actor->TryRecoverTo(",
            "Actor->IsRacePhysicsReady()",
            "Actor->AssignGridSlot(Slot)",
            "GridRivals.Bind(Slot,",
            "DestroyExperimentalRivals();",
        ):
            self.assertIn(expected, spawn)
        self.assertLess(spawn.index("!HasLocalPlayerDriver()"),
                        spawn.index("GetWorld()->SpawnActor<ALatebrakeRivalCar>("))
        self.assertLess(spawn.index("Actor->TryRecoverTo("),
                        spawn.index("GridRivals.Bind(Slot,"))
        self.assertIn("if (!bVerifiedPlacement", spawn)
        self.assertIn("bExperimentalSixCarGrid = false;", spawn)

    def test_missing_primary_is_never_ignored_by_traffic(self):
        collect = self.mode.split(
            "bool ALatebrakeRaceGameMode::TryCollectPhysicalTraffic(", 1
        )[1].split(
            "bool ALatebrakeRaceGameMode::TryCollectPlayerRecoveryTraffic(", 1
        )[0]
        for marker in (
            "RivalCar.Get() != Subject",
            "!IsValid(RivalCar)",
            "!IsRegisteredRivalActor(RivalCar)",
            "!Add(RivalCar.Get(), latebrake::PrimaryRivalGridSlot)",
            "if (!IsValid(Actor) ||",
            "!IsRegisteredRivalActor(Actor)",
        ):
            self.assertIn(marker, collect)
        self.assertIn("return false; // Missing registered physical AI", collect)


if __name__ == "__main__":
    unittest.main()
