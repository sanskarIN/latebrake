"""Check the actual AI control-stage integration, not native physics execution."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class FleetDecisionJournalBindingTests(unittest.TestCase):
    def test_real_pipeline_records_all_stages_once_in_order(self):
        rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        markers = [
            "FleetDecisionStage::Route,Command",
            "FleetDecisionStage::Corner,Command",
            "FleetDecisionStage::Difficulty,Command",
            "FleetDecisionStage::Grip,Command",
            "FleetDecisionStage::SurfaceDamage,Command",
            "FleetDecisionStage::EngineHeat,Command",
            "FleetDecisionStage::HumanFollowing,FollowingCommand",
            "FleetDecisionStage::Passing,PassingLane.Command",
            "FleetDecisionStage::ImmediateTraffic,Immediate.Command",
            "FleetDecisionStage::PredictiveTraffic,FleetCommand",
            "FleetDecisionStage::Yaw,SafeTrafficCommand",
            "FleetDecisionStage::Spin,SafeSpinCommand",
            "Mode->ObserveFleetRivalDecision(this,Decision)",
            "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)",
        ]
        positions = [rival.index(marker) for marker in markers]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("Decision.TrafficObserved=Observed", rival)
        self.assertIn("Decision.Risk=Supervised.Forecast.Kind", rival)
        self.assertIn("Decision.Queue=FleetQueue.Readout().Status", rival)
        self.assertIn("Decision.Passing=FleetPassing.Readout().Phase", rival)

    def test_actual_clock_admission_and_roster(self):
        mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        self.assertLess(mode.index("AdvancePlayerRaceClockIfReady("),
                        mode.index("BeginFleetDecisionFrame(LiveDelta,bActualClockAdvanced)"))
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        block = runtime.split("void ALatebrakeRaceGameMode::BeginFleetDecisionFrame(", 1)[1].split(
            "void ALatebrakeRaceGameMode::BeginFleetReplayFrame(", 1)[0]
        for marker in ("!bClockAdvanced", "FleetDecisions.Suspend()", "FleetDecisions.BeginFrame(",
                       "IsRegisteredRaceRival(Actor)", "Actor->IsRacePhysicsReady()",
                       "FleetBoundActors[static_cast<std::size_t>(Slot)].Get()!=Actor",
                       "FleetAttempt.BoundIdentity()", "Ordinal!=Sample.GateOrdinal",
                       "Sample.Frame=FleetFrameToken", "GFrameCounter",
                       "Sample.TeleportRevision=Actor->GetTeleportRevision()",
                       "FleetDecisions.Observe(Sample)"):
            self.assertIn(marker, block)
        self.assertIn("FleetDecisions.Reset()", runtime)
        self.assertIn("FleetDecisions.Configure(FleetAttemptToken", runtime)

    def test_review_requires_independent_route_evidence(self):
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        self.assertIn("Row.Slot==Slot && Row.BodyObserved && Row.RouteEvidenceValid", runtime)
        self.assertIn("Review.Decisions=FleetDecisions.Read(", runtime)
        self.assertIn("if (Race.IsFinished()) return FleetFinishReview;", runtime)
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("AI COMMAND OBSERVATIONS | SAMPLED, NOT PHYSICAL FORCE", hud)
        self.assertIn("FleetReview.Decisions.CurrentRivals", hud)
        self.assertIn("H.NoDriveContexts.Dominant()", hud)
        self.assertIn("Main sampled no-drive context", hud)
        self.assertIn("FleetNoDriveContextName(Context)", hud)
        self.assertIn("Decisions.Coverage.Valid", hud)
        self.assertIn("Coverage.Fraction()", hud)
        self.assertIn("Coverage.DurationSlots", hud)
        self.assertIn("Race.GetCheckpointCount()*Race.GetRequiredLaps(),Race.GetCheckpointCount()", runtime)
        self.assertIn("Entry.Sectors", hud)
        self.assertIn("gate transitions unassigned", hud)
        self.assertIn("Most sampled no-drive: next gate", hud)
        self.assertNotIn("FleetDecisions.Observe(", hud)
        self.assertIn("add_latebrake_portable_research_test(FleetDecisionJournal)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
