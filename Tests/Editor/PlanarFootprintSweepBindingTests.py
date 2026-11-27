"""Production-source binding checks; no native Chaos claim."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class PlanarFootprintSweepBindingTests(unittest.TestCase):
    def test_same_kernel_used_in_recovery_and_both_fleet_guards(self):
        recovery = read("Source/Latebrake/Public/AI/RivalRecoveryOccupancy.h")
        immediate = read("Source/Latebrake/Public/AI/RivalFleetTraffic.h")
        future = read("Source/Latebrake/Public/AI/RivalFleetCollisionForecast.h")
        self.assertIn("SweepPlanarFootprints({recoveryPose,blockerPose", recovery)
        self.assertIn("blockerVelocityX, blockerVelocityY) > 15000.0", recovery)
        self.assertIn("SweepPlanarFootprints({sample.Self.Pose,other.Pose", immediate)
        self.assertNotIn("PredictedRecoveryFootprintsOverlap(", immediate)
        self.assertIn("SweepPlanarFootprints({self.Pose,other.Pose", future)

    def test_real_qualified_commands_are_restricted_by_both_guards(self):
        source = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        observed = source.index("Mode->TryGetFreshFleetTraffic(this,bCornerPhysicsQualified")
        immediate = source.index("ApplyRivalFleetTraffic(PassingLane.Command,Snapshot)", observed)
        supervised = source.index("FleetRisk.Apply(Immediate.Command,Snapshot", immediate)
        self.assertLess(observed, immediate)
        self.assertLess(immediate, supervised)
        self.assertIn("FleetCommand=Supervised.Command;", source[supervised:])
        policy = read("Source/Latebrake/Public/AI/RivalFleetTraffic.h")
        self.assertIn("other.ActorToken == sample.Self.ActorToken", policy)
        self.assertIn("other.Slot<result.LimitingSlot", policy)
        self.assertIn("contact<result.ContactSeconds", policy)
        self.assertIn("if(!sweep.Valid)return {};", policy)
        self.assertNotIn("result.Command.Steering=", policy)

    def test_portable_suite_is_registered(self):
        self.assertIn("add_latebrake_portable_research_test(PlanarFootprintSweep)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
