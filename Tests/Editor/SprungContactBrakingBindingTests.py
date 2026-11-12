"""Real pawn source integration checks; these do not compile or run Unreal."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class SprungContactBrakingBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / 'Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp').read_text()
        cls.tick = cls.source.split('void ALatebrakeCarPawn::Tick(', 1)[1].split(
            'void ALatebrakeCarPawn::ApplySignalLights(', 1)[0]

    def test_road_points_and_velocities_come_from_existing_physics_hits(self):
        trace = self.tick.split('if (bGroundHit)', 1)[1].split(
            'latebrake::ChassisGroundContactOutput GroundContact', 1)[0]
        self.assertIn('Hit.ImpactPoint', trace)
        self.assertIn('Chassis->GetPhysicsLinearVelocityAtPoint(RoadPoint)', trace)
        self.assertIn('latebrake::MeasureBrakeRoadTangent({', trace)
        self.assertIn('RoadTangent.LongitudinalCmPerSecond', trace)
        self.assertIn('RoadTangent.PlanarForwardScale', trace)
        self.assertIn('bRoadBrakeSamplesValid = false;', trace)
        self.assertEqual(self.tick.count('LineTraceSingleByChannel('), 1)

    def test_validation_precedes_all_optional_motor_and_brake_dispatch(self):
        self.assertLess(self.tick.index('if (!ContactBraking.Valid)'),
                        self.tick.index('Chassis->AddForce(Forward * DriveForce'))
        self.assertIn('PhysicalBraking.ForwardForce * GroundContact.ContactTractionScale,', self.tick)
        self.assertIn('BrakeProfile.HandbrakeActive && !bServiceBraking,', self.tick)
        self.assertEqual(self.tick.count('ContactBraking.WholeBodyBudgetDeceleration +'), 2)

    def test_no_com_fallback_or_second_contact_multiplier(self):
        block = self.tick.split('if (bEffectiveBraking && GroundContact.HasDriveContact)', 1)[1].split(
            '// Neutral throttle COASTS', 1)[0]
        self.assertIn('ContactBraking.TangentForces[Corner]', block)
        self.assertIn('Chassis->AddForceAtLocation(', block)
        self.assertIn('WheelRoadContacts[Corner]', block)
        for forbidden in ('Chassis->AddForce(', '* ContactTractionScale',
                          'SetPhysicsLinearVelocity(', 'GetActorLocation('):
            self.assertNotIn(forbidden, block)
        self.assertIn('? ContactBraking.TotalForwardForce : 0.0;', self.tick)

    def test_full_3d_force_is_spent_and_actual_projection_is_logged(self):
        policy = (ROOT / 'Source/Latebrake/Public/Vehicles/SprungContactBraking.h').read_text()
        self.assertIn('out.TotalBrakeForceMagnitude / sample.MassKg /', policy)
        self.assertIn('out.TangentForces[i] * sample.PlanarForwardScales[i]', policy)
        dispatch = self.tick.split('if (bEffectiveBraking && GroundContact.HasDriveContact)', 1)[1].split(
            '// Neutral throttle COASTS', 1)[0]
        self.assertIn('RoadBrakeDirections[Corner] * static_cast<float>(BrakeForce)', dispatch)
        self.assertNotIn('Forward * static_cast<float>(BrakeForce)', dispatch)
        self.assertIn('? ContactBraking.TotalForwardForce : 0.0;', self.tick)

    def test_tangents_use_supported_real_hit_normals_not_visual_poses(self):
        trace = self.tick.split('if (bGroundHit)', 1)[1].split(
            'latebrake::ChassisGroundContactOutput GroundContact', 1)[0]
        for required in ('Distance <= latebrake::SuspensionRestDistanceCm',
                         'NormalZ >= latebrake::SuspensionMinNormalZ',
                         'static_cast<double>(Hit.ImpactNormal.X)',
                         'static_cast<double>(Hit.ImpactNormal.Y)',
                         'static_cast<double>(RoadVelocity.Z)',
                         'bRoadBrakeSamplesValid && RoadTangent.Valid'):
            self.assertIn(required, trace)
        for forbidden in ('GetComponentRotation(', 'WheelMotion.', 'AddForce('):
            self.assertNotIn(forbidden, trace)
        self.assertLess(self.tick.index('ApplySprungForces(SpringResponse'),
                        self.tick.index('if (!ContactBraking.Valid)'))

    def test_qa_and_strict_test_target_are_present(self):
        cmake = (ROOT / 'Tests/Standalone/CMakeLists.txt').read_text()
        self.assertIn('NAME SprungContactBraking', cmake)
        self.assertIn('latebrake_sprung_contact_braking_tests PRIVATE /W4 /WX', cmake)
        qa = (ROOT / 'docs/GRIP_LIMITED_BRAKING_QA.md').read_text()
        self.assertIn('SprungContactBraking.h', qa)
        self.assertIn('150,000', qa)


if __name__ == '__main__':
    unittest.main()
