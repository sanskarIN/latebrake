"""Exact Unreal callsite binding for opt-in live physical six-car GO checks.

This is source-only inspection: not a licensed UE5.8 Editor/UHT compile,
real Chaos simulation, cooked owned map or installed Win64 race.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class LiveSixCarStartingGridBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.mode_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.policy = read("Source/Latebrake/Public/AI/LiveSixCarStartingGrid.h")
        cls.tests = read("Tests/Standalone/LiveSixCarStartingGridTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.docs = read("docs/LIVE_SIX_CAR_GRID_QA.md")
        cls.nav = read("mkdocs.yml")

    def test_go_readiness_uses_six_actual_physics_snapshots_each_frame(self):
        qualify = self.mode.split(
            "bool ALatebrakeRaceGameMode::IsExperimentalFleetReadyForStart() const", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::RefreshExperimentalRivals()", 1
        )[0]
        for marker in (
            "IsExperimentalSixCarGridEnabled()",
            "HasLocalPlayerDriver()",
            "!IsRegisteredRivalActor(RivalCar)",
            "!RivalCar->IsRacePhysicsReady()",
            "UGameplayStatics::GetPlayerPawn(this, 0)",
            "!Human->IsRacePhysicsReady()",
            "latebrake::StartGridCapacity> Observed{}",
            "Car->IsRacePhysicsReady()",
            "Car->GetActorLocation()",
            "Car->GetActorRotation().Yaw",
            "Car->GetVelocity()",
            "Car->GetUniqueID()",
            "latebrake::LiveGridVerticalSample",
            "Height.ChassisWorldZCm",
            "World->GetDeltaSeconds()",
            "Height.Ground.ChassisVerticalSpeedCmPerSecond",
            "LineTraceSingleByChannel(",
            "SouthStraightCollisionActor.IsValid()",
            "Asphalt->GetCollisionEnabled()",
            "Asphalt->GetCollisionObjectType()",
            "Hit.GetActor() == SouthStraight",
            "ECC_WorldStatic",
            "Height.Ground.Probes[static_cast<std::size_t>(Index)]",
            "Observed, Vertical, latebrake::StartGridCapacity",
            "CollectLive(Human, latebrake::HumanGridSlot)",
            "CollectLive(RivalCar.Get(), latebrake::PrimaryRivalGridSlot)",
            "Actor->GetGridSlot() != Index + 2",
            "!IsRegisteredRivalActor(Actor)",
            "!Actor->IsRacePhysicsReady()",
            "!CollectLive(Actor, Index + 2)",
            "latebrake::EvaluateLiveSixCarStartingGrid(",
            "return LiveGrid.Safe();",
        ):
            self.assertIn(marker, qualify)
        self.assertLess(
            qualify.index("GetPlayerPawn(this, 0)"),
            qualify.index("latebrake::EvaluateLiveSixCarStartingGrid(")
        )
        self.assertLess(qualify.index("LineTraceSingleByChannel("),
                        qualify.index("latebrake::EvaluateLiveSixCarStartingGrid("))
        self.assertIn("Query.AddIgnoredActor(Car)", qualify)
        self.assertIn(
            "TWeakObjectPtr<AStaticMeshActor> SouthStraightCollisionActor;",
            self.mode_h)
        build = self.mode.split(
            "void ALatebrakeRaceGameMode::BuildCircuit()", 1)[1].split(
            "void ALatebrakeRaceGameMode::", 1)[0]
        self.assertIn("SouthStraightCollisionActor.Reset()", build)
        self.assertIn("SouthStraightCollisionActor = PhysicalBlock;", build)
        self.assertIn("if (Index == 1)", build)
        self.assertLess(qualify.index("SouthStraightCollisionActor.IsValid()"),
                        qualify.index("LineTraceSingleByChannel("))
        self.assertIn("Index < 4", qualify)
        self.assertNotIn("GetGroundContactReadout()", qualify)
        self.assertEqual(qualify.count("return LiveGrid.Safe();"), 1)
        self.assertIn("bool IsExperimentalFleetReadyForStart() const;", self.mode_h)

    def test_physical_readiness_still_precedes_real_startlights(self):
        tick = self.mode.split(
            "void ALatebrakeRaceGameMode::Tick(float DeltaSeconds)", 1
        )[1].split("void ALatebrakeRaceGameMode::", 1)[0]
        self.assertIn("DecideExperimentalFleetStart({", tick)
        self.assertIn("IsExperimentalFleetReadyForStart()", tick)
        self.assertIn("FleetStart.PermitCountdown", tick)
        self.assertIn("const bool bStartQualified =", tick)
        self.assertLess(tick.index("IsExperimentalFleetReadyForStart()"),
                        tick.index("StartLights.Advance("))
        self.assertIn("bFleetStartReady", tick)
        self.assertIn("bool bExperimentalSixCarGrid = false;", self.mode_h)
        self.assertIn("#if !UE_BUILD_SHIPPING", self.mode)
        self.assertIn('TEXT("LatebrakeExperimentalSixCarGrid")', self.mode)

    def test_policy_has_no_gameplay_or_unreal_mutation_authority(self):
        for forbidden in (
            "UCLASS(", "UPROPERTY(", "GetWorld(", "SpawnActor(",
            "SaveRecords(", "Race.PassCheckpoint(", "SetActorLocation(",
            "AddForce(", "AddTimePenalty(", "TryRecoverTo(",
        ):
            self.assertNotIn(forbidden, self.policy)
        for marker in (
            "LiveGridMaxPoseErrorCm", "LiveGridMaxHeadingErrorDegrees",
            "LiveGridMaxPlanarSpeedCmS",
            "LiveGridExpectedChassisCenterZCm",
            "LiveGridMaxVerticalDisplacementCm",
            "LiveGridMaxVerticalSpeedCmS",
            "LiveGridMinimumGroundProbes",
            "EvaluateChassisGroundContact(",
            "LiveGridFailure::UnsafeVerticalPosition",
            "LiveGridFailure::InsufficientGroundSupport",
            "observedCount != StartGridCapacity",
            "actor.Slot != slot", "actor.ActorToken == 0",
            "std::isfinite(actor.VelocityX)",
            "std::hypot(", "std::remainder(",
            "StartGridPoses[static_cast<std::size_t>(slot)]",
            "StartGridFootprintOnPavement(",
            "StartGridFootprintTouchesBarrier(",
            "StartGridFootprintsOverlap(",
            "LiveGridFailure::DuplicateActorIdentity",
            "LiveGridFailure::MovingBeforeGo",
            "LiveGridFailure::DisplacedFromAssignedSlot",
            "LiveGridFailure::CarOverlap",
        ):
            self.assertIn(marker, self.policy)
        self.assertIn("LiveSixCarStartingGridTests.cpp", self.cmake)
        self.assertIn("NAME LiveSixCarStartingGrid", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("trial < 4096", self.tests)
        self.assertIn("GoodVertical()", self.tests)
        self.assertIn("vChanged[5].Ground.Probes[0].Hit = false", self.tests)
        self.assertIn("ChassisContactMaximumSupportedHeightCm", self.tests)
        self.assertIn("std::numeric_limits<double>::infinity()", self.tests)
        self.assertIn("LIVE_SIX_CAR_GRID_QA.md", self.nav)
        self.assertIn("UE5.8", self.docs)


if __name__ == "__main__":
    unittest.main()
