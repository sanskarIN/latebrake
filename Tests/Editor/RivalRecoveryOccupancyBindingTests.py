"""Source-only verification of moving-human clearance for rival teleports.

Not an Unreal physics, Chaos collision, installed-game or full-grid test.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RivalRecoveryOccupancyBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.header = read("Source/Latebrake/Public/AI/RivalRecoveryOccupancy.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.tests = read("Tests/Standalone/RivalRecoveryOccupancyTests.cpp")

    def test_recovery_uses_verified_player_chassis_before_teleport(self):
        recovery = self.rival.split("if (Observation.NeedsRecovery)", 1)[1].split(
            "if (Observation.Completed)", 1)[0]
        markers = (
            "const latebrake::CircuitPose Pose = latebrake::RivalRecovery(",
            "UGameplayStatics::GetPlayerPawn(this, 0)",
            "Human->IsRacePhysicsReady()",
            "Human->GetActorLocation()",
            "Human->GetVelocity()",
            "Human->GetActorRotation().Yaw",
            "Human->GetUniqueID()",
            "latebrake::EvaluateRivalRecoveryClearance(",
            "if (!Clearance.Safe())",
            "Route.ReportRecoveryResult(false)",
            "const bool bRecovered = TryRecoverTo(",
            "Route.ReportRecoveryResult(bRecovered)",
        )
        positions = [recovery.index(v) for v in markers]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("Blockers, bHumanObserved ? 1 : 0", recovery)
        self.assertIn("Mode->ObserveRivalRecoveryResult(this, false)", recovery)
        self.assertIn("SetAutonomousControls(0.f, 0.f, true);", recovery)
        self.assertIn("SetAutonomousControls(0.f, 0.f, !bRecovered);", recovery)

    def test_safe_geometry_and_full_interval_sweep_are_source_policy(self):
        for term in (
            "RecoveryTrafficForecastSeconds = 0.4",
            "RecoveryChassisFitsPhysicalAsphalt(",
            "CollisionBlockStyle::Asphalt",
            "StartGridProjectionRadius(",
            "StartGridFootprintTouchesBarrier(",
            "EvaluateCircuitSpawnPose(",
            "PredictedRecoveryFootprintsOverlap(",
            "enter = std::max(",
            "leave = std::min(",
            "if (enter > leave)",
            "observedCount < 1",
            "std::hypot(other.VelocityX, other.VelocityY)",
            "prior.ActorToken == other.ActorToken",
            "RecoveryClearanceReason::TrafficCollision",
        ):
            self.assertIn(term, self.header)
        for forbidden in ("UCLASS(", "SpawnActor<", "SaveRecords(",
                          "RegisterCheckpoint(", "SetActorLocation(",
                          "PassCheckpoint(", "AddForce(", "Race.Advance("):
            self.assertNotIn(forbidden, self.header)
        self.assertIn("RivalRecoveryOccupancyTests.cpp", self.cmake)
        self.assertIn("NAME RivalRecoveryOccupancy", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("for (int degree = 0; degree < 3600", self.tests)


if __name__ == "__main__":
    unittest.main()
