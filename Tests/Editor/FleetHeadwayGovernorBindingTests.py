"""Source integration contracts for experimental fleet headway pacing.

These checks establish wiring, not a successful Unreal build or driving test.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp"
HEADER = ROOT / "Source/Latebrake/Public/AI/LatebrakeRivalCar.h"
PORTABLE = ROOT / "Tests/Standalone/CMakeLists.txt"


class FleetHeadwayGovernorBindingTests(unittest.TestCase):
    def test_actor_owns_separate_governor(self):
        header = HEADER.read_text(encoding="utf-8")
        self.assertIn('#include "AI/FleetHeadwayGovernor.h"', header)
        self.assertIn("latebrake::FleetHeadwayGovernor FleetHeadway;", header)
        self.assertLess(header.index('#include "AI/FleetHeadwayGovernor.h"'),
                        header.index('#include "LatebrakeRivalCar.generated.h"'))

    def test_headway_restricts_only_supervised_experimental_command(self):
        source = SOURCE.read_text(encoding="utf-8")
        begin = source.index("latebrake::PilotCommand FleetCommand=FollowingCommand;")
        end = source.index("const FVector ActualAngularVelocity", begin)
        block = source[begin:end]
        markers = (
            "if (Mode->IsExperimentalFleetRequested())",
            "Mode->TryGetFreshFleetTraffic",
            "FleetPassing.Apply(FollowingCommand",
            "ApplyRivalFleetTraffic(PassingLane.Command,Snapshot)",
            "FleetRisk.Apply(Immediate.Command,Snapshot",
            "FleetCommand=Supervised.Command;",
            "if (Supervised.Qualified)",
            "Headway=FleetHeadway.Apply(FleetCommand,",
            "FleetCommand=Headway.Command;",
            "FleetPassing.AbortTraffic()",
            "Decision.Set(latebrake::FleetDecisionStage::PredictiveTraffic,FleetCommand)",
        )
        indices = [block.index(marker) for marker in markers]
        self.assertEqual(indices, sorted(indices))
        self.assertIn("ModerateRivalYawRate(FleetCommand", source[end:])
        self.assertIn("SpinThrottleGuard.Apply(SafeTrafficCommand", source[end:])

    def test_headway_uses_same_fresh_physical_snapshot(self):
        source = SOURCE.read_text(encoding="utf-8")
        begin = source.index("Headway=FleetHeadway.Apply(FleetCommand,")
        end = source.index("FleetCommand=Headway.Command;", begin)
        block = source[begin:end]
        for marker in (
            "Ordinal,Route.GetNextWaypoint()",
            "static_cast<double>(DeltaSeconds)",
            "bCornerPhysicsQualified && !bOffPavement",
            "Snapshot,PassingStamp",
        ):
            self.assertIn(marker, block)
        self.assertIn("FleetFrame.AttemptToken", source)
        self.assertIn("CurrentSessionRevision,GetTeleportRevision()", source)

    def test_headway_has_same_reset_and_interrupt_lifecycle(self):
        source = SOURCE.read_text(encoding="utf-8")
        self.assertEqual(source.count("FleetHeadway.Reset();"), 2)
        self.assertEqual(source.count("FleetPassing.Reset();"), 2)
        for marker in (
            "FleetPassing.Reset();",
            "FleetPassing.Suspend();",
        ):
            headway = marker.replace("FleetPassing", "FleetHeadway")
            for line, value in enumerate(source.splitlines()):
                if value.strip() == marker:
                    self.assertEqual(source.splitlines()[line - 1].strip(), headway)
        self.assertIn("else FleetHeadway.Suspend();", source)

    def test_diagnostic_slot_does_not_replace_existing_threat(self):
        source = SOURCE.read_text(encoding="utf-8")
        begin = source.index("if (Decision.LimitingSlot<0 && Headway.Restricted")
        end = source.index("if (FleetCommand.Brake", begin)
        self.assertIn("Headway.Assessment.Leader.Valid", source[begin:end])
        self.assertIn("Headway.Assessment.TargetCeiling<1.0", source[begin:end])
        self.assertIn("Decision.LimitingSlot=Headway.Assessment.Leader.Slot;",
                      source[begin:end])

    def test_portable_registration(self):
        self.assertIn("add_latebrake_portable_research_test(FleetHeadwayGovernor)",
                      PORTABLE.read_text(encoding="utf-8"))
        self.assertTrue((ROOT / "Tests/Standalone/FleetHeadwayGovernorTests.cpp").is_file())


if __name__ == "__main__":
    unittest.main()
