"""Cosmetic native wiring checks, not an Unreal render/cook qualification."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class SurfaceSkidBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = (ROOT/'Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp').read_text()
        cls.renderer = (ROOT/'Source/Latebrake/Private/Vehicles/LatebrakeSkidTrailActor.cpp').read_text()
        cls.mode = (ROOT/'Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp').read_text()
        cls.tick = cls.pawn.split('void ALatebrakeCarPawn::Tick(', 1)[1].split(
            'void ALatebrakeCarPawn::ApplySignalLights(', 1)[0]

    def test_only_actual_accepted_tagged_road_contacts_supply_skid_data(self):
        self.assertIn('Block.Style == latebrake::CollisionBlockStyle::Asphalt', self.mode)
        self.assertIn('PhysicalBlock->Tags.AddUnique(FName(TEXT("LatebrakePavedRoad")))', self.mode)
        for marker in ('Hit.GetActor()->ActorHasTag(PavedRoadTag)',
                       'Hit.GetComponent()->GetUniqueID()', 'SkidRoadNormals[CornerIndex]',
                       'FVector::DotProduct(RoadVelocity, Right)',
                       'Contact.Position = {WheelRoadContacts[Corner].X,',
                       'Contact.NormalLoadNewtons = SpringResponse.Corners[Corner].UpForce *'):
            self.assertIn(marker, self.tick)
        self.assertEqual(self.tick.count('LineTraceSingleByChannel('), 1)

    def test_renderer_consumes_committed_slip_after_the_actual_force_path(self):
        submit = self.tick.index('SkidTrails->Submit(SkidFrame)')
        for marker in ('WheelBrakes.CommitDeliveredFrame(', 'BrakeCapability.Publish(',
                       'Chassis->AddForce(', 'Chassis->AddTorqueInRadians('):
            self.assertLess(self.tick.rindex(marker), submit)
        block = self.tick[self.tick.index('latebrake::SurfaceSkidFrame SkidFrame;'):submit]
        self.assertIn('WheelBrakes.LastCommitted()', block)
        self.assertIn('SkidFrame.TeleportRevision = TeleportRevision;', block)
        self.assertIn('SkidFrame.DrivingAllowed = !bPreGreen;', block)
        self.assertIn('Contact.ModeledSlipAvailable = DeliveredWheels.Valid', block)

    def test_render_holder_is_world_fixed_bounded_and_non_colliding(self):
        for marker in ('PrimaryActorTick.bCanEverTick = false', 'SetReplicates(false)',
                       'SetCollisionEnabled(ECollisionEnabled::NoCollision)',
                       'SetCanEverAffectNavigation(false)', 'SetGenerateOverlapEvents(false)',
                       'Pool.MaximumCount()', 'Pool.IsCurrent(Id)',
                       'Instances->AddInstance(Pose, true)',
                       'Instances->UpdateInstanceTransform(Index, Pose, true, false, true)',
                       'Instances->MarkRenderStateDirty()'):
            self.assertIn(marker, self.renderer)
        for forbidden in ('RemoveInstance(', 'SetupAttachment(', 'AttachToActor(', 'AddForce(',
                          'SetPhysicsLinearVelocity(', 'PassCheckpoint(', 'SaveGame', 'AwardCredits('):
            self.assertNotIn(forbidden, self.renderer)

    def test_missing_real_material_disables_visuals_without_a_white_fallback(self):
        self.assertIn('source_only_qa=required', self.renderer)
        self.assertIn('Instances->SetVisibility(bRenderingReady)', self.renderer)
        self.assertIn('/Game/Materials/M_LatebrakeTireRubber.M_LatebrakeTireRubber', self.renderer)
        self.assertIn('+DirectoriesToAlwaysCook=(Path="/Game/Materials")',
                      (ROOT/'Config/DefaultGame.ini').read_text())

    def test_interruption_restart_and_actor_teardown_release_cosmetic_state(self):
        self.assertIn('if (IsValid(SkidTrails)) SkidTrails->BreakContinuity();', self.pawn)
        self.assertIn('if (IsValid(SkidTrails)) SkidTrails->ResetTrails();', self.pawn)
        end = self.pawn.split('void ALatebrakeCarPawn::EndPlay(', 1)[1].split(
            'void ALatebrakeCarPawn::UnPossessed()', 1)[0]
        self.assertIn('SkidTrails->Destroy()', end)
        self.assertIn('SkidTrails = nullptr;', end)


if __name__ == '__main__':
    unittest.main()
