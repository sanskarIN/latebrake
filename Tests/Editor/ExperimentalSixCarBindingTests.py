"""Source-only guardrails for opt-in six-physical-car test builds.

The test proves branch source structure, not UE5.8/UHT/Chaos, cooked-map,
race performance, matchmaking, scoring or installed-device playability.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class ExperimentalSixCarBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.mode_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.rival_h = read("Source/Latebrake/Public/AI/LatebrakeRivalCar.h")
        cls.traffic = read("Source/Latebrake/Public/AI/RivalFleetTraffic.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.tests = read("Tests/Standalone/RivalFleetTrafficTests.cpp")
        cls.docs = read("docs/EXPERIMENTAL_SIX_CAR_QA.md")
        cls.nav = read("mkdocs.yml")

    def test_never_enable_more_than_one_rival_without_explicit_local_flag(self):
        self.assertIn("bool bExperimentalSixCarGrid = false;", self.mode_h)
        self.assertIn("#if !UE_BUILD_SHIPPING", self.mode)
        self.assertIn("FParse::Param(", self.mode)
        self.assertIn("FCommandLine::Get()", self.mode)
        self.assertIn('TEXT("LatebrakeExperimentalSixCarGrid")', self.mode)
        source = self.mode.split(
            "void ALatebrakeRaceGameMode::RefreshExperimentalRivals()", 1
        )[1].split(
            "bool ALatebrakeRaceGameMode::IsRegisteredRivalActor(", 1
        )[0]
        for marker in (
            "#if UE_BUILD_SHIPPING",
            "DestroyExperimentalRivals();",
            "!bExperimentalSixCarGrid",
            "!bCircuitReady",
            "bCareerRace",
            "SelectedEvent != latebrake::EventKind::Circuit",
            "!IsPlannedStartGridSafe()",
            "latebrake::StartGridCapacity - 2",
            "ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding",
            "GetWorld()->SpawnActor<ALatebrakeRivalCar>(",
            "Actor->AssignGridSlot(Slot)",
            "GridRivals.Bind(Slot",
            "bExperimentalSixCarGrid = false",
        ):
            self.assertIn(marker, source)
        self.assertIn(
            "TArray<TObjectPtr<ALatebrakeRivalCar>> ExperimentalRivals;",
            self.mode_h)
        self.assertIn("RefreshExperimentalRivals();", self.mode)
        self.assertIn("DestroyExperimentalRivals();", self.mode)

    def test_retained_primary_rival_is_only_one_for_official_split(self):
        section = self.mode.split(
            "void ALatebrakeRaceGameMode::ObserveAdditionalRivalVerifiedGate(", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::ObserveAdditionalRivalRecoveryResult(", 1
        )[0]
        for forbidden in (
            "RivalSplits.RecordRival(", "RivalFinishClocks.RecordRivalAcceptedGate(",
            "Race.PassCheckpoint(", "EarnCredits(", "SaveRecords(",
            "Career.CommitFinish(", "SetActorLocation(", "SpawnActor(",
        ):
            self.assertNotIn(forbidden, section)
        self.assertIn("GridRivals.RecordAcceptedGate(", section)
        self.assertIn("IsPrimaryRivalActor(Actor)", section)
        self.assertIn("IsRegisteredRivalActor(Actor)", section)
        gate = self.rival.split("if (Observation.Advanced)", 1)[1].split(
            "if (Observation.NeedsRecovery)", 1
        )[0]
        self.assertIn("Mode->ObserveRivalVerifiedGate();", gate)
        self.assertIn("Mode->ObserveAdditionalRivalVerifiedGate(this);", gate)
        self.assertIn("if (Mode->IsPrimaryRivalActor(this))", gate)
        self.assertIn("bool AssignGridSlot(int32 NewSlot);", self.rival_h)
        self.assertIn("int32 GridSlot = 1;", self.rival_h)

    def test_actual_obstacles_are_required_for_every_physical_rival(self):
        collector = self.mode.split(
            "bool ALatebrakeRaceGameMode::TryCollectPhysicalTraffic(", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::ObserveAdditionalRivalVerifiedGate(", 1
        )[0]
        for marker in (
            "IsRegisteredRivalActor(Subject)", "HasLocalPlayerDriver()",
            "UGameplayStatics::GetPlayerPawn(this, 0)",
            "Human->IsRacePhysicsReady()",
            "Actor->IsRacePhysicsReady()",
            "Actor->GetActorLocation()",
            "Actor->GetActorRotation().Yaw",
            "Actor->GetVelocity()",
            "Actor->GetUniqueID()",
            "RivalCar.Get() != Subject",
            "for (int32 Index = 0; Index < ExperimentalRivals.Num(); ++Index)",
            "if (!IsValid(Actor) ||",
            "!IsRegisteredRivalActor(Actor)",
            "if (Actor != Subject && !Add(Actor, Index + 2))",
        ):
            self.assertIn(marker, collector)
        self.assertIn("bool ALatebrakeRaceGameMode::IsRegisteredRivalActor(", self.mode)
        self.assertIn("GridRivals.Matches(Slot, Identity)", self.mode)
        self.assertIn("bUseFleetTraffic ? FleetTraffic.Command : TrafficCommand",
                      self.rival)
        self.assertIn("Mode->IsExperimentalSixCarGridEnabled()", self.rival)
        self.assertIn("Mode->TryCollectPhysicalTraffic(", self.rival)
        self.assertIn("latebrake::ApplyRivalFleetTraffic(TrafficCommand, {",
                      self.rival)

    def test_recovery_and_traffic_remain_non_authoritative(self):
        for forbidden in (
            "Race.PassCheckpoint(", "SaveRecords(", "EarnCredits(",
            "SetActorLocation(", "AddForce(", "UCLASS(", "GetWorld(",
            "SpawnActor(", "RegisterCheckpoint(",
        ):
            self.assertNotIn(forbidden, self.traffic)
        self.assertIn("PredictedRecoveryFootprintsOverlap(", self.traffic)
        self.assertIn("FleetTrafficRearSkipCm", self.traffic)
        self.assertIn("return {};", self.traffic)
        self.assertIn("RivalFleetTrafficTests.cpp", self.cmake)
        self.assertIn("NAME RivalFleetTraffic", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("for (int i = 0; i < 5; ++i)", self.tests)
        self.assertIn("EXPERIMENTAL_SIX_CAR_QA.md", self.nav)
        self.assertIn("opt-in", self.docs.lower())


if __name__ == "__main__":
    unittest.main()
