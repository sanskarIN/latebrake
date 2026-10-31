"""Source bindings for real rival physical 3D checkpoint crossings.

This suite checks the actual Unreal call sequence, not an Unreal Engine
runtime build or licensed device playtest.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class RivalDirectionalGateBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rival = load("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.rival_header = load("Source/Latebrake/Public/AI/LatebrakeRivalCar.h")
        cls.route = load("Source/Latebrake/Public/AI/RivalRouteProgress.h")
        cls.trace = load("Source/Latebrake/Public/Racing/CheckpointMotionTrace.h")
        cls.plane = load("Source/Latebrake/Public/Racing/CircuitGateCrossing.h")
        cls.cmake = load("Tests/Standalone/CMakeLists.txt")
        cls.stress = load("Tests/Standalone/RivalDirectionalGateTests.cpp")

    def test_real_chassis_gate_trace_precedes_route_and_recovery(self):
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        required = (
            "Mode->IsRaceStartGreen()",
            "IsRacePhysicsReady()",
            "const FVector Position = GetActorLocation();",
            "const FVector Velocity = GetVelocity();",
            "const double FlatSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();",
            "const int32 Next = Route.GetNextWaypoint();",
            "const double Distance = FVector::Dist2D(Position, Target);",
            "RivalGateTrace.Sample(",
            "Route.ObserveVerifiedGate(",
            "if (Observation.Advanced)",
            "if (Observation.NeedsRecovery)",
            "const bool bRecovered = TryRecoverTo(",
            "Route.ReportRecoveryResult(bRecovered)",
        )
        offsets = [tick.index(token) for token in required]
        self.assertEqual(offsets, sorted(offsets))
        self.assertEqual(tick.count("RivalGateTrace.Sample("), 1)
        self.assertEqual(tick.count("Route.ObserveVerifiedGate("), 1)
        self.assertNotIn("Route.Observe(", tick)
        self.assertIn("Next, 0, {{Position.X, Position.Y}, Position.Z}", tick)
        self.assertIn("Velocity.X, Velocity.Y", tick)
        self.assertIn("bVerifiedGateCrossed);", tick)
        self.assertIn("RivalGateTrace.Reset();", tick)

    def test_route_requires_event_for_real_rival_but_old_contract_remains(self):
        self.assertIn("RouteObservation ObserveVerifiedGate(", self.route)
        self.assertIn("bool forwardPlaneCrossed", self.route)
        self.assertIn("requiresPlaneCrossing && forwardPlaneCrossed", self.route)
        self.assertIn("!requiresPlaneCrossing && distanceCm <= 440.0", self.route)
        self.assertIn("if (RecoveryPending || IsFinished()", self.route)
        self.assertIn("++NextWaypoint;", self.route)
        self.assertIn("RecoveryPending = true;", self.route)
        self.assertIn("ReportRecoveryResult(bool enginePlacementVerified)", self.route)
        self.assertIn("RouteObservation Observe(double distanceCm", self.route)
        for forbidden in ("AddTimePenalty(", "EarnCredits(", "SaveGame",
                          "UCLASS(", "SetActorLocation(", "AddTorque("):
            self.assertNotIn(forbidden, self.route)

    def test_shared_player_gate_geometry_requires_motion_and_corridor(self):
        self.assertIn('#include "Racing/CheckpointMotionTrace.h"',
                      self.rival_header)
        self.assertIn("latebrake::CheckpointMotionTrace RivalGateTrace;",
                      self.rival_header)
        self.assertIn("EvaluateCircuitGatePlane(", self.trace)
        self.assertIn("PreviousGeneration != generation", self.trace)
        self.assertIn("CanPassCircuitGate(", self.plane)
        self.assertIn("GateMaxFrameTravelCm = 1200.0", self.plane)
        self.assertIn("GateLateralHalfWidthCm = 500.0", self.plane)
        self.assertIn("GateVerticalHalfHeightCm = 380.0", self.plane)
        self.assertIn("previousAlong < 0.0 && currentAlong >= 0.0",
                      self.plane)

    def test_no_stale_trace_from_pauses_teleport_or_new_waypoints(self):
        for method in ("ResetPilot", "ConfigurePilot"):
            code = self.rival.split(
                "void ALatebrakeRivalCar::" + method + "(", 1
            )[1].split("void ALatebrakeRivalCar::", 1)[0]
            self.assertIn("RivalGateTrace.Reset();", code)
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        self.assertGreaterEqual(tick.count("RivalGateTrace.Reset();"), 4)
        for outcome in ("if (Observation.Advanced)", "if (Observation.NeedsRecovery)",
                        "if (Observation.Completed)"):
            self.assertIn(outcome, tick)

    def test_cross_platform_stress_target_is_compiled(self):
        self.assertIn("RivalDirectionalGateTests.cpp", self.cmake)
        self.assertIn("NAME RivalDirectionalGate", self.cmake)
        self.assertIn("latebrake_rival_directional_gate_tests PRIVATE /W4 /WX",
                      self.cmake)
        self.assertIn("200000", self.stress)
        self.assertIn("ObserveVerifiedGate(", self.stress)


if __name__ == "__main__":
    unittest.main()
