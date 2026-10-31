"""Source-only proof that the actual rival uses audited waypoint stall recovery.

These checks do not compile or run Unreal Engine 5.8, Chaos or installed QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class RivalNoApproachBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.route = load("Source/Latebrake/Public/AI/RivalRouteProgress.h")
        cls.rival = load("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.cmake = load("Tests/Standalone/CMakeLists.txt")
        cls.tests = load("Tests/Standalone/RivalNoApproachRecoveryTests.cpp")

    def test_real_rival_uses_measured_target_distance_and_chassis_speed(self):
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        markers = (
            "Mode->IsRaceStartGreen()",
            "IsRacePhysicsReady()",
            "const FVector Position = GetActorLocation();",
            "const FVector Velocity = GetVelocity();",
            "const double FlatSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();",
            "const latebrake::CircuitPoint Waypoint = latebrake::CircuitGate(Next);",
            "const double Distance = FVector::Dist2D(Position, Target);",
            "Route.Observe(",
            "if (Observation.NeedsRecovery)",
            "const bool bRecovered = TryRecoverTo(",
            "Route.ReportRecoveryResult(bRecovered)",
        )
        self.assertEqual(
            [tick.index(marker) for marker in markers],
            sorted(tick.index(marker) for marker in markers),
        )
        self.assertIn(
            "Distance, FlatSpeed, static_cast<double>(DeltaSeconds)", tick
        )

    def test_no_approach_is_real_distance_not_free_speed_or_route_advancement(self):
        for required in (
            "MeaningfulApproachCm = 260.0",
            "NoApproachRecoverSeconds = 8.0",
            "NoApproachMinSpeedCmPerSecond = 450.0",
            "NoApproachMaxFrameSeconds = 0.10",
            "MaxQualifiedWaypointDistanceCm = 50000.0",
            "distanceCm <= ApproachReferenceDistanceCm - MeaningfulApproachCm",
            "StationarySeconds >= 8.0 || noApproachStall",
            "ResetApproachWindow();",
            "if (RecoveryPending || IsFinished()",
        ):
            self.assertIn(required, self.route)
        self.assertIn("bool ReportRecoveryResult(bool enginePlacementVerified)",
                      self.route)
        self.assertIn("++Recoveries;", self.route)
        self.assertIn("RecoveryPending = true;", self.route)
        self.assertLess(self.route.index("++Recoveries;"),
                        self.route.index("RecoveryPending = true;"))
        self.assertNotIn("SetActorLocation", self.route)
        self.assertNotIn("AwardCredits", self.route)
        self.assertNotIn("GetRace(", self.route)

    def test_resets_and_invalid_samples_cannot_carry_teleport_authority(self):
        for marker in (
            "void Reset() noexcept", "ResetApproachWindow();",
            "if (deltaSeconds <= NoApproachMaxFrameSeconds)",
        ):
            if marker == "if (deltaSeconds <= NoApproachMaxFrameSeconds)":
                # Frame eligibility is combined with other physical gates.
                self.assertIn(
                    "deltaSeconds <= NoApproachMaxFrameSeconds",
                    self.route,
                )
            else:
                self.assertIn(marker, self.route)
        self.assertIn("if (Recoveries >= MaxRecoveries)", self.route)
        self.assertIn("if (!RecoveryPending)", self.route)
        self.assertIn("RecoveryPending = false;", self.route)

    def test_stress_suite_is_in_strict_cxx_matrix(self):
        self.assertIn("RivalNoApproachRecoveryTests.cpp", self.cmake)
        self.assertIn("NAME RivalNoApproachRecovery", self.cmake)
        self.assertIn(
            "latebrake_rival_no_approach_recovery_tests PRIVATE /W4 /WX",
            self.cmake,
        )
        self.assertIn("200000", self.tests)
        self.assertIn("ReportRecoveryResult(", self.tests)
        self.assertIn("Meaningful", self.route)


if __name__ == "__main__":
    unittest.main()
