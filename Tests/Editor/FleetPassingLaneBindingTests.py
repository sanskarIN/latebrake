"""Source integration checks; not native engine testing."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class FleetPassingLaneBindingTests(unittest.TestCase):
    def test_live_roster_and_existing_brakes(self):
        text = (ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp").read_text()
        start = text.index("const bool Observed=Mode->TryGetFreshFleetTraffic(")
        end = text.index("const FVector ActualAngularVelocity", start)
        block = text[start:end]
        for marker in ("FleetPassing.Apply(FollowingCommand", "bCornerPhysicsQualified && !bOffPavement",
                       "Route.GetCompletedLaps()*Route.GetWaypointCount()",
                       "ApplyRivalFleetTraffic(PassingLane.Command,Snapshot)",
                       "FleetRisk.Apply(Immediate.Command,Snapshot", "FleetPassing.AbortTraffic()",
                       "FleetCommand.Steering=FollowingCommand.Steering",
                       "FleetPassing.Suspend()"):
            self.assertIn(marker, block)
        self.assertLess(block.index("FleetPassing.Apply"), block.index("FleetRisk.Apply"))
        self.assertLess(block.index("FleetRisk.Apply"), block.index("FleetPassing.AbortTraffic"))
        self.assertIn("ModerateRivalYawRate(FleetCommand", text[end:])
        self.assertIn("SpinThrottleGuard.Apply(SafeTrafficCommand", text[end:])

    def test_reset_and_interruptions(self):
        text = (ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp").read_text()
        self.assertEqual(text.count("FleetPassing.Reset()"), 2)
        self.assertIn("FleetPassing.Suspend();\n        LastQualifiedSessionRevision", text)
        self.assertIn("FleetPassing.Suspend();\n        const int32 RecoveryIndex", text)
        self.assertIn("FleetPassing.Suspend();\n        SetAutonomousControls", text)

    def test_registration(self):
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text()
        self.assertIn("add_latebrake_portable_research_test(FleetPassingLane)", cmake)


if __name__ == "__main__":
    unittest.main()
