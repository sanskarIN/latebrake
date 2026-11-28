"""Actual route deferral uses fresh fleet evidence and preserves gate credit."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class FleetQueueDeferralBindingTests(unittest.TestCase):
    def test_fresh_queue_before_route_observation(self):
        rival=read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        tick=rival.split("void ALatebrakeRivalCar::Tick(",1)[1]
        markers=("RivalGateTrace.Sample(", "Mode->IsExperimentalFleetRequested() && !bVerifiedGateCrossed",
                 "TryGetFreshFleetTraffic(this,false,0.0,QueueSample)",
                 "FleetQueue.Observe(Ordinal,", "Route.ObserveVerifiedGate(",
                 "bVerifiedGateCrossed,bVerifiedTrafficWait", "if (Observation.NeedsRecovery)")
        offsets=[tick.index(x) for x in markers]
        self.assertEqual(offsets,sorted(offsets))
        self.assertEqual(tick.count("Route.ObserveVerifiedGate("),1)
        self.assertIn("FleetQueue.Suspend();",tick)
        self.assertIn("FleetQueueFrameStamp QueueStamp{FleetFrame.AttemptToken",tick)
        self.assertIn("CurrentSessionRevision,GetTeleportRevision()",tick)
        self.assertIn("QueueSample,QueueStamp).DeferStallRecovery",tick)
        self.assertNotIn("FleetQueue.Reset();",tick)
    def test_real_gate_precedes_stationary_deferral(self):
        policy=read("Source/Latebrake/Public/AI/RivalRouteProgress.h")
        gate=policy.index("++NextWaypoint;")
        defer=policy.index("if (requiresPlaneCrossing && deferStationaryForVerifiedTraffic")
        stall=policy.index("bool noApproachStall = false")
        self.assertLess(gate,defer);self.assertLess(defer,stall)
        self.assertIn("speedCmPerSecond < 80.0",policy[defer:stall])
        self.assertIn("SuspendObservation();",policy[defer:stall])
        self.assertIn("add_latebrake_portable_research_test(FleetQueueDeferral)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
