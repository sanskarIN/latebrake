"""Source checks for experimental fleet slipstream, not native physics evidence."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class FleetSlipstreamDraftBindingTests(unittest.TestCase):
    def test_fresh_human_and_all_five_actual_physical_rivals(self):
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        block = runtime.split("bool ALatebrakeRaceGameMode::TryGetFleetDraftFrame(", 1)[1].split(
            "void ALatebrakeRaceGameMode::ObserveFleetPlayerTraffic(", 1)[0]
        for text in (
            "Out={}", "IsExperimentalFleetRequested()", "IsValid(Observer)",
            "Observer!=FleetBoundActors[0].Get()", "Observer->IsRacePhysicsReady()",
            "HasLocalPlayerDriver()", "Race.IsFinished()", "IsExperimentalFleetDriveReady()",
            "CollectFleetFrame(Physical,0.0,false)", "MatchLiveFleetIdentity(",
            "FleetAttemptToken", "FleetFrameToken", "GFrameCounter", "CheckpointSession.Current()",
            "FleetRivalAt(Slot)", "IsRegisteredRaceRival(Rival)",
            "Rival!=FleetBoundActors[static_cast<std::size_t>(Slot)].Get()",
            "Rival->IsRacePhysicsReady()", "Rival->GetActorLocation()",
            "Rival->GetActorForwardVector()", "Rival->GetVelocity()",
            "Rival->HasQualifiedDraftRoadSupport()", "Rival->HasCompletedRoute()",
            "Rival->GetTeleportRevision()", "Observer->GetTeleportRevision()",
            "ValidateFleetDraftFrame(CandidateFrame)", "Out=CandidateFrame",
        ):
            self.assertIn(text, block)
        self.assertIn("Slot<latebrake::StartGridCapacity", block)
        self.assertLess(block.index("ValidateFleetDraftFrame(CandidateFrame)"),
                        block.index("Out=CandidateFrame"))

    def test_real_wake_trace_ignores_exact_endpoints_only(self):
        runtime = read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        block = runtime.split("bool ALatebrakeRaceGameMode::TryGetFleetDraftFrame(", 1)[1].split(
            "void ALatebrakeRaceGameMode::ObserveFleetPlayerTraffic(", 1)[0]
        self.assertIn("Params.AddIgnoredActor(Observer)", block)
        self.assertIn("Params.AddIgnoredActor(Rival)", block)
        self.assertEqual(block.count("Params.AddIgnoredActor("), 2)
        self.assertIn("ECC_Visibility,Params", block)
        self.assertIn("Entry.ClearPhysicalPath=!World->LineTraceSingleByChannel(", block)
        self.assertIn("!Entry.RouteComplete", block)
        self.assertIn("Entry.GroundSupported", block)

    def test_separate_experimental_meter_shared_existing_force_budget(self):
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        tick = pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        begin = tick.index("if (Mode->IsExperimentalFleetRequested() && IsLocallyControlled()")
        finish = tick.index("latebrake::AllocateArcadeBoost(", begin)
        block = tick[begin:finish]
        for text in (
            "Cast<APlayerController>(GetController()) != nullptr", "Mode->HasLocalPlayerDriver()",
            "static_cast<double>(LongitudinalSpeed)", "static_cast<double>(LateralSpeed)",
            "static_cast<double>(EffectiveThrottle)", "bEffectiveBraking",
            "HasQualifiedDraftGroundSupport(GroundContact)", "Mode->TryGetFleetDraftFrame(",
            "DraftExtraForce=FleetSlipstreamBoost.Observe(DraftFrame).ExtraForce",
            "FleetSlipstreamBoost.Suspend()",
        ):
            self.assertIn(text, block)
        self.assertEqual(tick.count("latebrake::AllocateArcadeBoost("), 1)
        self.assertLess(tick.index("SlipstreamBoost.Observe({"), begin)
        self.assertLess(finish, tick.index("CurrentDraftAppliedFraction", finish))
        self.assertIn("CombinedBoost.DraftExtra", tick[finish:])

    def test_interruptions_and_read_only_hud(self):
        pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        clear = pawn.split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        self.assertIn("FleetSlipstreamBoost.Suspend()", clear)
        self.assertIn("FleetSlipstreamBoost.Reset()", read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"))
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("GetFleetSlipstreamReadout()", hud)
        self.assertIn("SLIPSTREAM AI %d", hud)
        self.assertIn("GetAppliedDraftBoostFraction()", hud)
        self.assertNotIn("FleetSlipstreamBoost.Observe(", hud)
        self.assertIn("add_latebrake_portable_research_test(FleetSlipstreamDraft)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
