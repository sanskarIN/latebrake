"""Shared-pawn advisory frame wiring; these checks never execute Unreal."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class RivalBrakeCapabilityBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = (ROOT/'Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp').read_text()
        cls.header = (ROOT/'Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h').read_text()
        cls.rival = (ROOT/'Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp').read_text()
        cls.tick = cls.pawn.split('void ALatebrakeCarPawn::Tick(', 1)[1].split(
            'void ALatebrakeCarPawn::ApplySignalLights(', 1)[0]

    def test_estimate_uses_same_accepted_spring_tangent_and_profile_as_physical_braking(self):
        for marker in ('EstimateServiceBrakeCapability({', 'RoadContactForwardSpeeds,',
                       'RoadBrakeNormalZ, RoadBrakeForwardScales}',
                       'GroundContact, SpringResponse, WheelBrakeSetup)',
                       'bArcade, bUseWheelBrakeModel, bRoadBrakeSamplesValid',
                       'Thermal.Valid ? Thermal.ServiceBrakeMultiplier : 1.0'):
            self.assertIn(marker, self.tick)
        self.assertLess(self.tick.index('ConstrainGroundContactToSuspension('),
                        self.tick.index('EstimateServiceBrakeCapability({'))

    def test_advisory_frame_publishes_after_actual_drive_commands(self):
        publish = self.tick.index('BrakeCapability.Publish(')
        for marker in ('Chassis->AddForceAtLocation(', 'EngineHeat.SettleNitroDelivery(',
                       'Chassis->AddForce(FVector('):
            self.assertLess(self.tick.rindex(marker), publish)
        self.assertIn('static_cast<double>(DeltaSeconds), TeleportRevision)', self.tick[publish:])
        self.assertIn('return BrakeCapability.Read();', self.header)

    def test_invalid_recovery_pause_and_garage_changes_invalidate_advisory_data(self):
        for begin, end in [('void ALatebrakeCarPawn::ClearDriverInputs(',
                           'void ALatebrakeCarPawn::RejectOptionalDriveAfterSpring('),
                          ('void ALatebrakeCarPawn::SetGarageTuning(',
                           'void ALatebrakeCarPawn::ClearDriverInputs('),
                          ('bool ALatebrakeCarPawn::TryRecoverTo(',
                           'void ALatebrakeCarPawn::RecoverTo(')]:
            block = self.pawn.split(begin, 1)[1].split(end, 1)[0]
            self.assertIn('BrakeCapability.Invalidate();', block)
        self.assertIn('BrakeCapability.Invalidate();', self.header.split('ResetDrivingStats()', 1)[1])

    def test_real_rival_consumes_capacity_before_corner_forecast_and_all_traffic_guards(self):
        start = self.rival.index('const auto CapacityPace =')
        block = self.rival[start:]
        for marker in ('BrakeCapacityPace.Apply(Command, {',
                       'GetTeleportRevision()', 'GetBrakeCapabilityEstimate()',
                       'bCornerPhysicsQualified && CapacityPace.Valid',
                       'true, CapacityPace.DecelerationCeilingCmPerSecondSquared',
                       '0.20 + static_cast<double>(DeltaSeconds)'):
            self.assertIn(marker, block)
        for marker in ('ApplyRivalCornerBrakeEnvelope(', 'ApplyRivalDifficulty(',
                       'YieldRivalToPlayer(', 'SetAutonomousControls('):
            self.assertLess(block.index('BrakeCapacityPace.Apply('), block.index(marker))

    def test_capacity_policy_does_not_create_engine_force_or_race_authority(self):
        policy = (ROOT/'Source/Latebrake/Public/AI/RivalBrakeCapacityPace.h').read_text()
        for forbidden in ('AddForce(', 'PassCheckpoint(', 'SaveGame', 'AwardCredits(',
                          'SetActorLocation(', 'SetPhysicsLinearVelocity('):
            self.assertNotIn(forbidden, policy)
        self.assertIn('c.Sequence <= LastSequence', policy)
        self.assertIn('c.TeleportRevision != s.TeleportRevision', policy)


if __name__ == '__main__':
    unittest.main()
