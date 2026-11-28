"""Source-level fleet player traffic contracts; no native engine claim."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class FleetPlayerTrafficBindingTests(unittest.TestCase):
    def test_attempt_reset_and_real_roster_configuration(self):
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        begin = runtime.split("void ALatebrakeRaceGameMode::BeginExperimentalFleetAttempt()", 1)[1].split(
            "ALatebrakeRivalCar* ALatebrakeRaceGameMode::FleetRivalAt(", 1)[0]
        self.assertLess(begin.index("FleetPlayerTraffic.Reset()"), begin.index("FleetAttempt.Begin("))
        self.assertIn("LedgerBound && !FleetPlayerTraffic.Configure(FleetAttemptToken,FleetAttempt.BoundIdentity())", runtime)

    def test_sample_once_after_actual_clock_admission(self):
        mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        tick = mode.split("void ALatebrakeRaceGameMode::Tick(", 1)[1].split(
            "void ALatebrakeRaceGameMode::SuspendBestLapGhost()", 1)[0]
        self.assertEqual(tick.count("ObserveFleetPlayerTraffic("), 1)
        self.assertLess(tick.index("AdvancePlayerRaceClockIfReady("),
                        tick.index("ObserveFleetPlayerTraffic(LiveDelta,bActualClockAdvanced)"))
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        observe = runtime.split("void ALatebrakeRaceGameMode::ObserveFleetPlayerTraffic(", 1)[1].split(
            "latebrake::FleetRaceReview ALatebrakeRaceGameMode::CollectFleetRaceReview()", 1)[0]
        for marker in ("!IsExperimentalFleetRequested()", "!bClockAdvanced",
                       "Race.IsFinished()", "!IsExperimentalFleetDriveReady()",
                       "CollectFleetFrame(Sample.Physical,0.0,false)",
                       "CheckpointSession.Current()", "GetTeleportRevision()",
                       "GFrameCounter", "Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds()",
                       "FleetPlayerTraffic.Suspend()", "FleetPlayerTraffic.Observe(Sample)"):
            self.assertIn(marker, observe)
        self.assertLess(observe.index("GetTeleportRevision()"), observe.index("FleetPlayerTraffic.Observe(Sample)"))

    def test_fresh_review_and_read_only_hud(self):
        review = read("Source/Latebrake/Public/AI/FleetRaceReview.h")
        self.assertIn("out.Traffic=MeasureFleetPlayerTraffic(live,bound)", review)
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        self.assertIn("Review.TrafficHistory=FleetPlayerTraffic.Readout()", runtime)
        self.assertIn("if (Race.IsFinished()) return FleetFinishReview;", runtime)
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("FleetReview.Traffic", hud)
        self.assertIn("Review.TrafficHistory", hud)
        self.assertIn("Traffic.Exposure.AccountsFor", hud)
        self.assertIn("Exposure.MultiRivalSeconds()", hud)
        self.assertIn("Exposure.LongestProjectedPath.Duration()", hud)
        self.assertIn("FleetTrafficDirectionName(Threat.Direction)", hud)
        self.assertNotIn("FleetPlayerTraffic.Observe(", hud)
        self.assertNotIn("ObserveFleetPlayerTraffic(", hud)
        self.assertIn("add_latebrake_portable_research_test(FleetPlayerTraffic)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
