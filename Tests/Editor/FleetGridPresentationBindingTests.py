"""Actual grid settling/visual plan wiring, not installed Unreal evidence."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class FleetGridPresentationBindingTests(unittest.TestCase):
    def test_settling_preserves_only_measured_vertical(self):
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        tick=pawn.split("void ALatebrakeCarPawn::Tick(",1)[1].split(
            "// The authoritative clock also requires",1)[0]
        markers=("ReadExperimentalGridSettlingVelocity(DeltaSeconds)",
                 "NeutralizeMotionWhenDrivingBlocked();",
                 "SettlingZ<0.0",
                 "SetPhysicsLinearVelocity(FVector(0.0,0.0,SettlingZ))")
        offsets=[tick.index(x) for x in markers]
        self.assertEqual(offsets,sorted(offsets))
        helper=pawn.split("double ALatebrakeCarPawn::ReadExperimentalGridSettlingVelocity(",1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()",1)[0]
        markers=("TryGetExperimentalGridSettlingSlot(this,Slot)",
                 "!IsRacePhysicsReady()", "Chassis->GetPhysicsLinearVelocity()",
                 "PlanFleetGridSettling(", "Settling.PreserveDownward")
        offsets=[helper.index(x) for x in markers]
        self.assertEqual(offsets,sorted(offsets))
        self.assertNotIn("SetPhysicsLinearVelocity(",helper)
        self.assertNotIn("AddForce(",tick+helper)
        mode=read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        settle=mode.split("bool ALatebrakeRaceGameMode::TryGetExperimentalGridSettlingSlot(",1)[1]
        for token in ("StartLights.IsGreen()", "Menu.IsOpen()",
                      "IsGamePaused(this)", "AppFocusGuard.IsSuspended()",
                      "ManualPause.IsSuspended()", "IsRestartPlacementReady()",
                      "FleetBoundActors[0].Get()!=Actor"):
            self.assertIn(token,settle)
    def test_visual_plan_is_one_time_and_neutral(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        spawn=mode.split("void ALatebrakeRaceGameMode::SpawnExtraFleet()",1)[1].split(
            "bool ALatebrakeRaceGameMode::CollectFleetFrame(",1)[0]
        self.assertIn("BuildFleetVisualPlan(GetSelectedVehicleId(),GetSelectedGarageLook())",spawn)
        self.assertIn("SetGarageTuning(1.f,1.f,1.f,Entry.VehicleId,Entry.Look)",spawn)
        self.assertIn("!ExtraFleet.IsEmpty()",spawn)
        for token in ("SaveRecords(", "Garage.EarnCredits(", "Garage.Unlock"):
            self.assertNotIn(token,spawn)
        self.assertIn("SetGarageTuning(1.f,1.f,1.f,0,latebrake::FactoryGarageLook(0))",mode)
if __name__=="__main__":unittest.main()
