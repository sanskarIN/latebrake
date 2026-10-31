"""Production source checks for the real rival's continuous run-through target.

Not a licensed UE 5.8/UHT game compile or evidence of a playable build.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RivalGateRunThroughBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.route = read("Source/Latebrake/Public/AI/RivalRouteProgress.h")
        cls.pilot = read("Source/Latebrake/Public/AI/RivalPilot.h")
        cls.target = read("Source/Latebrake/Public/AI/RivalGateRunThrough.h")
        cls.geometry = read("Source/Latebrake/Public/Track/CircuitLayout.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.stress = read("Tests/Standalone/RivalGateRunThroughTests.cpp")

    def test_real_physics_keeps_approaching_until_plane_is_verified(self):
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        markers = (
            "Mode->IsRaceStartGreen()",
            "IsRacePhysicsReady()",
            "const int32 Next = Route.GetNextWaypoint();",
            "if (!latebrake::IsCircuitGate(Next))",
            "const latebrake::CircuitPoint Waypoint = latebrake::CircuitGate(Next);",
            "RivalGateTrace.Sample(",
            "Route.ObserveVerifiedGate(",
            "if (Observation.Advanced)",
            "if (Observation.NeedsRecovery)",
            "latebrake::RivalGateRunThroughPoint(Route.GetNextWaypoint())",
            "latebrake::PlanRivalTurn(",
            "latebrake::AnticipateRivalCorner(",
            "GripPace.Apply(",
            "latebrake::YieldRivalToPlayer(",
            "SetAutonomousControls(static_cast<float>(SafeTrafficCommand.Throttle)",
        )
        # The anticipation wrapper is syntactically outside the nested
        # planner invocation, but remains the returned command's authority.
        self.assertIn("latebrake::AnticipateRivalCorner(\n        latebrake::PlanRivalTurn(",
                      tick)
        for token in markers:
            self.assertIn(token, tick)
        self.assertLess(tick.index("RivalGateTrace.Sample("),
                        tick.index("Route.ObserveVerifiedGate("))
        self.assertLess(tick.index("Route.ObserveVerifiedGate("),
                        tick.index("latebrake::RivalGateRunThroughPoint("))
        self.assertLess(tick.index("latebrake::RivalGateRunThroughPoint("),
                        tick.index("latebrake::PlanRivalTurn("))
        self.assertIn("NextTarget.X, NextTarget.Y, 440.0, false)", tick)
        self.assertNotIn("Route.Observe(", tick)

    def test_real_gate_invalid_index_brakes_and_never_aims_at_origin(self):
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        block = tick.split("if (!latebrake::IsCircuitGate(Next))", 1)[1].split(
            "const latebrake::CircuitPoint Waypoint", 1
        )[0]
        for required in ("RivalGateTrace.Reset();", "FollowingBrakeHold.Reset();",
                         "GripPace.Reset();", "SurfaceDamagePace.Reset();",
                         "SetAutonomousControls(0.f, 0.f, true);",
                         "Super::Tick(DeltaSeconds);", "return;"):
            self.assertIn(required, block)

    def test_legacy_proximity_callers_are_preserved_but_rival_opt_out(self):
        self.assertIn("bool allowProximityArrival = true", self.pilot)
        self.assertIn("if (allowProximityArrival && distance <= arrivalRadiusCm)",
                      self.pilot)
        self.assertIn("arrivalRadiusCm > 0.0", self.pilot)
        self.assertIn("RouteObservation ObserveVerifiedGate(", self.route)
        self.assertIn("requiresPlaneCrossing && forwardPlaneCrossed", self.route)
        self.assertIn("++NextWaypoint;", self.route)
        self.assertIn("RivalGateRunThroughDistanceCm = 550.0", self.target)
        self.assertIn("if (!IsCircuitGate(nextGate))", self.target)
        self.assertIn("const CircuitPoint gate = CircuitGate(nextGate);",
                      self.target)
        for gate in ("case 0:", "case 1:", "case 2:", "case 3:"):
            self.assertIn(gate, self.target)
        for forbidden in ("AddForce(", "AddTorque(", "AwardCredits(",
                          "PassCheckpoint(", "RaceProgress", "UCLASS("):
            self.assertNotIn(forbidden, self.target)

    def test_portable_stress_and_qa_are_registered(self):
        self.assertIn("RivalGateRunThroughTests.cpp", self.cmake)
        self.assertIn("NAME RivalGateRunThrough", self.cmake)
        self.assertIn("latebrake_rival_gate_run_through_tests PRIVATE /W4 /WX",
                      self.cmake)
        self.assertIn("200000", self.stress)
        self.assertIn("CheckpointMotionTrace", self.stress)
        self.assertIn("RivalGateRunThroughPoint", self.stress)


if __name__ == "__main__":
    unittest.main()
