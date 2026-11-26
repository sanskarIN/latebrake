"""Source contracts for real arcade skid marks, not native rendered pixels."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]

def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class ArcadeSkidTrailBindingTests(unittest.TestCase):
    def test_actual_existing_probes_supply_only_measured_evidence(self):
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        tick=pawn.split("void ALatebrakeCarPawn::Tick(",1)[1].split(
            "void ALatebrakeCarPawn::SubmitSkidTrailFrame(",1)[0]
        markers=("LineTraceSingleByChannel(", "TryGetSkidRoadSurface(Hit.GetComponent(),AsphaltBlock)",
                 "GetPhysicsLinearVelocityAtPoint(Hit.ImpactPoint)",
                 "MakeArcadeSkidProbeContact({", "EvaluateChassisGroundContact(GroundSample)",
                 "if (!ReverseLimit.Valid)", "SubmitSkidTrailFrame(SkidFrame);")
        offsets=[tick.index(marker) for marker in markers]
        self.assertEqual(offsets,sorted(offsets))
        self.assertEqual(tick.count("LineTraceSingleByChannel("),1)
        self.assertIn("SkidFrame.SessionRevision=Mode->GetCheckpointSessionRevision()",tick)
        self.assertIn("SkidFrame.TeleportRevision=GetTeleportRevision()",tick)
        self.assertNotIn("NormalLoadNewtons=",tick)
        self.assertNotIn("ModeledBrakingSlip=",tick)
        self.assertIn("if (!GroundContact.HasDriveContact) SkidFrame.Contacts={};",tick)

    def test_component_identity_not_tag_or_actor_name(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        lookup=mode.split("bool ALatebrakeRaceGameMode::TryGetSkidRoadSurface(",1)[1].split(
            "bool ALatebrakeRaceGameMode::HasRival()",1)[0]
        for marker in ("Road!=HitComponent","IsArcadeSkidAsphaltBlock(Index)",
                       "GetComponentLocation().Equals","GetComponentScale().Equals",
                       "GetComponentQuat().Equals","ECC_WorldStatic"):
            self.assertIn(marker,lookup)
        self.assertIn("SkidRoadComponents[Index]=BlockActor->GetStaticMeshComponent()",mode)
        self.assertIn("SkidRoadComponents.Reset();",mode)

    def test_optional_world_space_renderer_has_no_physics_authority(self):
        renderer=read("Source/Latebrake/Private/Vehicles/LatebrakeSkidTrailActor.cpp")
        for marker in ("ECollisionEnabled::NoCollision","SetGenerateOverlapEvents(false)",
                       "SetCanEverAffectNavigation(false)","SetSimulatePhysics(false)",
                       "SetActorEnableCollision(false)","GetAllVectorParameterInfo",
                       "EMaterialParameterAssociation::GlobalParameter",
                       "Pool.IsCurrent(Slot)","Pool.IsActive(Slot)",
                       "IsRenderableSkidStrip(Strip)","UpdateInstanceTransform(",
                       "AddInstance(Pose,true)","DisableRendering()"):
            self.assertIn(marker,renderer)
        for forbidden in ("AddForce(","AddTimePenalty(","SaveRecords(","AttachToActor("):
            self.assertNotIn(forbidden,renderer)
        self.assertNotIn("->RemoveInstance(",renderer)

    def test_pause_recovery_restart_and_endplay_lifecycle(self):
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        header=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        self.assertIn("void ALatebrakeCarPawn::ClearDriverInputs()\n{\n    BreakSkidTrailContinuity();",pawn)
        self.assertIn("ResetSkidTrails(); }",header)
        self.assertIn("SkidTrailActor->Destroy();",pawn)
        self.assertIn("bSkidRendererAttempted=true;",pawn)
        recover=pawn.split("bool ALatebrakeCarPawn::TryRecoverTo(",1)[1].split(
            "void ALatebrakeCarPawn::RecoverTo(",1)[0]
        self.assertIn("ClearDriverInputs();",recover)
        self.assertIn("SurfaceSkidTrailTests.cpp",read("Tests/Standalone/CMakeLists.txt"))

if __name__=="__main__":
    unittest.main()
