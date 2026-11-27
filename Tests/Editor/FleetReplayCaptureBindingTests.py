"""Native source assertions; not installed Unreal gameplay evidence."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class FleetReplayCaptureBindingTests(unittest.TestCase):
    def test_clock_and_finish_authority(self):
        mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        self.assertLess(mode.index("AdvancePlayerRaceClockIfReady("),
                        mode.index("BeginFleetReplayFrame(LiveDelta,bActualClockAdvanced)"))
        finish = mode.index("FleetFinishReview=FinishTiming")
        freeze = mode.index("FleetReplay.Freeze(", finish)
        self.assertLess(freeze, mode.index("FleetAttempt.Finish(", freeze))
        self.assertLess(freeze, mode.index("FleetFrameAuthority.Revoke()", freeze))

    def test_real_identity_and_sample_clock(self):
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        block = runtime.split("void ALatebrakeRaceGameMode::BeginFleetReplayFrame(", 1)[1].split(
            "void ALatebrakeRaceGameMode::ObserveFleetPlayerTraffic(", 1)[0]
        for marker in ("!bClockAdvanced", "IsExperimentalFleetDriveReady()",
                       "FleetAttemptToken,FleetFrameToken", "GFrameCounter",
                       "CheckpointSession.Current()", "FleetAttempt.BoundIdentity()",
                       "FleetBoundActors[static_cast<std::size_t>(Index)].Get()==Car",
                       "HasLocalPlayerDriver()", "IsRegisteredRaceRival(",
                       "Car->GetTeleportRevision()", "FleetReplay.Submit("):
            self.assertIn(marker, block)
        self.assertIn("FleetReplay.Reset()", runtime)

    def test_same_tick_road_and_controls(self):
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        submit = pawn.index("Mode->ObserveFleetReplayCar(this,Sample)")
        start = pawn.rindex("if (Mode->IsExperimentalFleetRequested())", 0, submit)
        block = pawn[start:submit]
        for marker in ("SkidFrame.Contacts", "Contact.Supported", "Contact.PavedRoad",
                       "SkidContactEvidence::ArcadeGroundProbe", "GroundContact",
                       "GetActorLocation()", "Velocity.X", "Velocity.Y", "Velocity.Z",
                       "AssistedSteering", "EffectiveThrottle", "bEffectiveBraking",
                       "IsRacePhysicsReady()"):
            self.assertIn(marker, block)
        self.assertLess(pawn.index("LastGroundContact = GroundContact"), submit)

    def test_results_read_only(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("RECENT SIX-CAR REPLAY OBSERVATIONS", hud)
        self.assertIn("GetFleetReplayReview()", hud)
        self.assertNotIn("FleetReplay.Submit(", hud)
        self.assertIn("add_latebrake_portable_research_test(FleetReplayCapture)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
