"""Native fleet source wiring, not Unreal runtime evidence."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(name): return (ROOT/name).read_text(encoding="utf-8")
class ExperimentalFleetRuntimeBindingTests(unittest.TestCase):
    def test_shipping_and_gameplay_isolation(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        self.assertIn("#if UE_BUILD_SHIPPING\n    return false;",mode)
        self.assertIn("!bCareerRace",mode)
        self.assertIn("DontSpawnIfColliding",mode)
        self.assertIn("FleetAttempt.ReportSpawnFailure",mode)
        self.assertIn("!StartLights.IsGreen()",mode)
        self.assertIn("FleetObservedEpoch!=CheckpointSession.Current()",mode)
        self.assertIn("FleetBoundActors[0].Get()!=Human",mode)
        self.assertIn("FleetBoundActors[static_cast<std::size_t>(Slot)].Get()!=Rival",mode)
        self.assertIn("IsArcadeSkidRoadPoint(Road,",mode)
        self.assertNotIn("&& Road==1",mode)
    def test_no_experimental_finish_save(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        gate=mode.split("void ALatebrakeRaceGameMode::RegisterCheckpoint(",1)[1].split(
            "\n}\n",1)[0]
        a=gate.index("FleetAttempt.Finish(FleetAttemptToken)")
        b=gate.index("latebrake::ProfileWriteCheckpoint SavedProfile")
        self.assertLess(a,b)
        self.assertIn("return;",gate[a:b])
    def test_fresh_recovery_and_traffic(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        for marker in ("CollectFleetFrame(Frame,0.0,false)","MakeFreshFleetTraffic",
                       "SelectFleetRivalRecovery","TryGetSkidRoadSurface(Hit.GetComponent(),Road)"):
            self.assertIn(marker,mode)
        rival=read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        self.assertIn("Mode->IsRegisteredRaceRival(this)",rival)
        self.assertIn("FleetRisk.Apply(Immediate.Command",rival)
        self.assertIn("bFleetMode?FleetSelection.Clear:Selection.Safe()",rival)
if __name__=="__main__": unittest.main()
