"""Source-only guard for real rival Chaos-motion spin policy integration.

This checks text wiring, not UnrealHeaderTool or installed gameplay.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
RIVAL = (ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "Source/Latebrake/Public/AI/LatebrakeRivalCar.h").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")


class RivalSpinThrottleBindingTests(unittest.TestCase):
    def test_real_physics_and_final_policy_order(self):
        self.assertIn('#include "AI/RivalSpinThrottleGuard.h"', HEADER)
        self.assertIn('latebrake::RivalSpinThrottleGuard SpinThrottleGuard;', HEADER)
        markers = (
            'GetPhysicsAngularVelocityInRadians()',
            'latebrake::ModerateRivalYawRate(',
            'SpinThrottleGuard.Apply(',
            'SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)',
            'Super::Tick(DeltaSeconds);',
        )
        start = RIVAL.index('const FVector ActualAngularVelocity =')
        positions = [RIVAL.find(fragment, start) for fragment in markers]
        self.assertTrue(all(position >= start for position in positions))
        self.assertEqual(positions, sorted(positions))
        self.assertIn('Velocity, GetActorRightVector()', RIVAL)
        self.assertIn('static_cast<double>(ActualAngularVelocity.Z)', RIVAL)

    def test_all_session_interruptions_clear_spin_hold(self):
        self.assertGreaterEqual(RIVAL.count('SpinThrottleGuard.Reset();'), 6)
        for marker in ('void ALatebrakeRivalCar::ResetPilot()',
                       'void ALatebrakeRivalCar::ConfigurePilot(',
                       'if (Observation.NeedsRecovery)',
                       'if (Observation.Completed)'):
            self.assertIn(marker, RIVAL)

    def test_portable_cross_platform_strict_target(self):
        self.assertIn('add_executable(latebrake_rival_spin_throttle_tests RivalSpinThrottleGuardTests.cpp)', CMAKE)
        self.assertIn('target_compile_features(latebrake_rival_spin_throttle_tests PRIVATE cxx_std_20)', CMAKE)
        self.assertIn('add_test(NAME RivalSpinThrottleGuard COMMAND latebrake_rival_spin_throttle_tests)', CMAKE)


if __name__ == '__main__':
    unittest.main()
