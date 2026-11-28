"""Source-only contract for developer fleet lane planning."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class FleetLineAimBindingTests(unittest.TestCase):
    def test_experimental_aim_only_changes_route_target(self):
        rival=read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        tick=rival.split("void ALatebrakeRivalCar::Tick(",1)[1]
        markers=("const double Distance = FVector::Dist2D(Position, Target)",
                 "RivalGateTrace.Sample(", "Route.ObserveVerifiedGate(",
                 "const latebrake::CircuitPoint NextTarget = Mode->IsExperimentalFleetRequested()",
                 "PlanFleetLineAim({GridSlot,Route.GetNextWaypoint()",
                 "RivalGateRunThroughPoint(Route.GetNextWaypoint())",
                 "NextTarget.X, NextTarget.Y, 440.0, false",
                 "ApplyRivalCornerBrakeEnvelope(", "FleetRisk.Apply(",
                 "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)")
        offsets=[tick.index(x) for x in markers]
        self.assertEqual(offsets,sorted(offsets))
        self.assertEqual(tick.count("Route.ObserveVerifiedGate("),1)
        self.assertEqual(tick.count("RivalGateTrace.Sample("),1)
    def test_geometry_policy_has_no_race_authority(self):
        policy=read("Source/Latebrake/Public/AI/FleetLineAim.h")
        for marker in ("FleetLanePoseFitsSlab(sample.Actual,straight.Slab)",
                       "FleetLanePoseFitsSlab({target,sample.Actual.HeadingDegrees},straight.Slab)",
                       "FleetLaneMaxLateralChangeCm","GateLateralHalfWidthCm",
                       "RivalGateRunThroughPoint(sample.NextGate)"):
            self.assertIn(marker,policy)
        for forbidden in ("AddForce(", "PassCheckpoint(", "TryRecoverTo(",
                          "SaveRecords(", "SetAutonomousControls("):
            self.assertNotIn(forbidden,policy)
        self.assertIn("add_latebrake_portable_research_test(FleetLineAim)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
