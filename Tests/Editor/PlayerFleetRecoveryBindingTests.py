"""Bind actual paid player recovery to complete physical six-car occupancy.

Source inspection only. The checker cannot run UE5.8, real Chaos bodies,
a cooked map or verify an installed Win64 executable.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class PlayerFleetRecoveryBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.header = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.policy = read("Source/Latebrake/Public/AI/PlayerFleetRecoveryClearance.h")
        cls.tests = read("Tests/Standalone/PlayerFleetRecoveryClearanceTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_legacy_paid_teleport_runs_only_after_traffic_preflight(self):
        recover = self.mode.split(
            "void ALatebrakeRaceGameMode::RecoverPlayer(", 1
        )[1].split("void ALatebrakeRaceGameMode::LoadRecords(", 1)[0]
        for marker in (
            "Car != UGameplayStatics::GetPlayerPawn(this, 0)",
            "!StartLights.IsGreen()",
            "const latebrake::CircuitPose Pose =",
            "bExperimentalSixCarGrid && !bCareerRace &&",
            "SelectedEvent == latebrake::EventKind::Circuit",
            "TryCollectPlayerRecoveryTraffic(",
            "EvaluatePlayerFleetRecoveryClearance(",
            "if (!Clearance.Safe())",
            "PLAYER_FLEET_RECOVER_BLOCKED",
            "TryChargePlayerRecoveryAfterVerifiedTeleport(",
            "HasLocalPlayerDriver()",
            "Car->TryRecoverTo(Target, Facing)",
        ):
            self.assertIn(marker, recover)
        self.assertLess(
            recover.index("EvaluatePlayerFleetRecoveryClearance("),
            recover.index("TryChargePlayerRecoveryAfterVerifiedTeleport("))
        self.assertLess(
            recover.index("if (!Clearance.Safe())"),
            recover.index("const FVector Target("))
        self.assertIn(
            "return;", recover.split("if (!Clearance.Safe())", 1)[1].split(
                "const FVector Target(", 1)[0]
        )
        self.assertIn("bool bExperimentalSixCarGrid = false;", self.header)
        self.assertNotIn("Race.PassCheckpoint(", recover)
        self.assertNotIn("SaveRecords(", recover)

    def test_five_actual_actor_tokens_are_collected_without_fakes(self):
        collect = self.mode.split(
            "bool ALatebrakeRaceGameMode::TryCollectPlayerRecoveryTraffic(", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::ObserveAdditionalRivalVerifiedGate(", 1
        )[0]
        for marker in (
            "Out.fill({});",
            "OutCount = 0;",
            "!IsExperimentalSixCarGridEnabled()",
            "Player != UGameplayStatics::GetPlayerPawn(this, 0)",
            "!HasLocalPlayerDriver()",
            "!IsRegisteredRivalActor(RivalCar)",
            "Slot < latebrake::StartGridCapacity",
            "ExperimentalRivals[Slot - 2].Get()",
            "!IsValid(Actor)",
            "Actor->GetGridSlot() != Slot",
            "!IsRegisteredRivalActor(Actor)",
            "!Actor->IsRacePhysicsReady()",
            "Actor->GetActorLocation()",
            "Actor->GetVelocity()",
            "Actor->GetActorRotation().Yaw",
            "Actor->GetUniqueID()",
            "OutCount == latebrake::PlayerFleetRecoveryRequiredRivals",
        ):
            self.assertIn(marker, collect)
        self.assertIn("bool TryCollectPlayerRecoveryTraffic(", self.header)
        self.assertIn("OutCount) const;", self.header)

    def test_portable_full_occupancy_and_no_authored_fee(self):
        for marker in (
            "PlayerFleetRecoveryRequiredRivals =",
            "StartGridCapacity - 1",
            "observedCount != PlayerFleetRecoveryRequiredRivals",
            "other.Slot != i + PrimaryRivalGridSlot",
            "other.ActorToken == 0",
            "EvaluateRivalRecoveryClearance(",
            "RecoveryClearanceReason::UnknownOccupancy",
        ):
            self.assertIn(marker, self.policy)
        for forbidden in (
            "SaveRecords(", "SpawnActor(", "TryRecoverTo(",
            "AddTimePenalty(", "EarnCredits(", "UCLASS(", "GetWorld(",
            "SetActorLocation(", "Race.PassCheckpoint(",
        ):
            self.assertNotIn(forbidden, self.policy)
        for marker in (
            "TryChargePlayerRecoveryAfterVerifiedTeleport(",
            "RecoveryPenaltySeconds",
            "recoveryCount == 0",
            "recoveryCount == 1",
            "PlayerRecovery(gate)",
        ):
            self.assertIn(marker, self.tests)
        self.assertIn("PlayerFleetRecoveryClearanceTests.cpp", self.cmake)
        self.assertIn("NAME PlayerFleetRecoveryClearance", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)


if __name__ == "__main__":
    unittest.main()
