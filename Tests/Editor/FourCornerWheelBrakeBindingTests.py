"""Actual shared-pawn wiring, not a licensed UE build or real wheel ABS test."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]


class FourCornerWheelBrakeBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp=(ROOT/'Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp').read_text()
        cls.header=(ROOT/'Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h').read_text()
        cls.tick=cls.cpp.split('void ALatebrakeCarPawn::Tick(',1)[1].split(
            'void ALatebrakeCarPawn::ApplySignalLights(',1)[0]

    def test_model_uses_accepted_real_points_and_existing_force_ceiling(self):
        for marker in ('RoadBrakeNormalZ[static_cast<unsigned>(Index)] = NormalZ',
                       'WheelBrakes.Propose({','RoadContactForwardSpeeds, RoadBrakeNormalZ, RoadBrakeForwardScales',
                       'BrakeProfileForVehicle(GarageVehicleId, bArcade)',
                       'GroundContact, SpringResponse, ContactBraking)',
                       'ContactBraking = WheelBrakePlan.Brakes;'):
            self.assertIn(marker,self.tick)
        self.assertLess(self.tick.index('CalculateSprungContactBraking({'),self.tick.index('WheelBrakes.Propose({'))
        self.assertLess(self.tick.index('ContactBraking = WheelBrakePlan.Brakes;'),
                        self.tick.index('CalculateLateralGripForce({'))

    def test_delivery_receipt_is_committed_only_after_actual_commands(self):
        block=self.tick.split('if (bEffectiveBraking && GroundContact.HasDriveContact)',1)[1].split(
            '// Neutral throttle COASTS',1)[0]
        self.assertIn('Chassis->AddForceAtLocation(',block)
        self.assertLess(block.index('Chassis->AddForceAtLocation('),block.index('WheelBrakes.CommitDeliveredFrame('))
        self.assertIn('WheelBrakes.CommitDeliveredFrame(WheelBrakePlan.Receipt)',block)
        self.assertIn('WheelBrakes.RejectUndeliveredFrame(WheelBrakePlan.Receipt)',block)
        self.assertLess(self.tick.index('if (!WheelBrakePlan.Valid)'),self.tick.index('Chassis->AddForce(Forward * DriveForce'))

    def test_lifecycle_clears_modeled_state_and_readout_is_committed_only(self):
        clear=self.cpp.split('void ALatebrakeCarPawn::ClearDriverInputs(',1)[1].split(
            'void ALatebrakeCarPawn::RejectOptionalDriveAfterSpring(',1)[0]
        self.assertIn('WheelBrakes.Reset();',clear)
        reset=self.header.split('void ResetDrivingStats()',1)[1].split('GetDriftMeter()',1)[0]
        self.assertIn('WheelBrakes.Reset();',reset)
        self.assertIn('return WheelBrakes.LastCommitted();',self.header)
        self.assertNotIn('WheelBrakes.Propose(',self.header)

    def test_visual_wheels_consume_delivered_integrated_rotation(self):
        visual=self.cpp.split('void ALatebrakeCarPawn::UpdateWheelVisuals(',1)[1].split(
            'bool ALatebrakeCarPawn::IsRacePhysicsReady()',1)[0]
        self.assertIn('WheelBrakes.LastCommitted()',visual)
        self.assertIn('ModeledRotationRadians',visual)
        self.assertIn('WheelMotion.ApplyCommittedBrakeRotation(BrakeFrame.Receipt, mask, rotation)',visual)
        self.assertNotIn('Chassis->AddForce(',visual)
        self.assertNotIn('SaveGame(',visual)

    def test_diagnostics_are_explicitly_modeled_and_rate_bounded(self):
        diagnostic=self.tick.split('// Real Development-build evidence',1)[1]
        self.assertIn('if (bReportSuspensionDriveEvidence)',diagnostic)
        self.assertIn('LATEBRAKE_WHEEL_BRAKES v=1 driver=%s source=modeled',diagnostic)
        for field in ('road_fl_cmps=','omega_fl=','slip_fl=','pressure_fl=',
                      'normal_est_fl_n=','command_fl_n='):
            self.assertIn(field,diagnostic)
        self.assertIn('WheelBrakes.LastCommitted()',diagnostic)

    def test_editor_ab_switch_retains_legacy_bound_without_claiming_abs_sensor(self):
        self.assertIn('bool bUseWheelBrakeModel = true;',self.header)
        self.assertIn('if (bUseWheelBrakeModel)',self.tick)
        self.assertIn('WheelBrakePlan.Brakes = ContactBraking;',self.tick)
        self.assertIn('MODELED', (ROOT/'Source/Latebrake/Public/Vehicles/FourCornerWheelBrakes.h').read_text())


if __name__=='__main__':
    unittest.main()
