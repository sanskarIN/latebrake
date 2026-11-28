"""Source order contracts, not native Unreal scheduler execution."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class FleetFrameAuthorityBindingTests(unittest.TestCase):
    def test_engine_frame_is_observed_before_clock_authority(self):
        runtime=read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        observe=runtime.split("bool ALatebrakeRaceGameMode::ObserveExperimentalFleet(",1)[1].split(
            "bool ALatebrakeRaceGameMode::IsExperimentalFleetDriveReady()",1)[0]
        markers=("GFrameCounter","FleetFrameAuthority.BeginFrame(Stamp)",
                 "++FleetFrameToken","FleetAttempt.Observe(Frame)",
                 "FleetFrameAuthority.Commit(Stamp")
        offsets=[observe.index(x) for x in markers]
        self.assertEqual(offsets,sorted(offsets))
        self.assertIn("Stamp.SessionEpoch==CheckpointSession.Current()",observe)
        self.assertIn("!HasCurrentFleetFrameAuthority()",runtime)
        self.assertIn("FleetFrameAuthority.IsCurrent({FleetAttemptToken,FleetFrameToken",runtime)
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        tick=mode.split("void ALatebrakeRaceGameMode::Tick(",1)[1]
        self.assertLess(tick.index("ObserveExperimentalFleet("),tick.index("StartLights.Advance("))
        self.assertLess(tick.index("StartLights.Advance("),tick.index("AdvancePlayerRaceClockIfReady("))
    def test_tick_dependencies_only_opt_in_and_removed(self):
        runtime=read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        register=runtime.split("void ALatebrakeRaceGameMode::RegisterFleetTickDependent(",1)[1].split(
            "void ALatebrakeRaceGameMode::ClearFleetTickPrerequisites()",1)[0]
        self.assertIn("!IsExperimentalFleetRequested()",register)
        self.assertIn("Actor==this",register)
        self.assertIn("Actor->GetWorld()!=GetWorld()",register)
        self.assertIn("Existing.Get()==Actor",register)
        self.assertIn("Actor->AddTickPrerequisiteActor(this)",register)
        self.assertIn("Actor->RemoveTickPrerequisiteActor(this)",runtime)
        begin=runtime.split("void ALatebrakeRaceGameMode::BeginExperimentalFleetAttempt()",1)[1].split(
            "ALatebrakeRivalCar* ALatebrakeRaceGameMode::FleetRivalAt(",1)[0]
        self.assertLess(begin.index("ClearFleetTickPrerequisites()"),begin.index("DestroyExtraFleet()"))
        self.assertIn("RegisterFleetTickDependent(Gate.Get())",begin)
        self.assertIn("RegisterFleetTickDependent(Actor.Get())",runtime)
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        self.assertIn("FleetCheckpointActors.Add(Checkpoint)",mode)
        self.assertIn("FleetFrameAuthority.Revoke();\n    ClearFleetTickPrerequisites();",mode)
        self.assertIn("add_latebrake_portable_research_test(FleetFrameAuthority)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
