"""Static binding guards for rival grip/traffic policy composition.

This is source text inspection only, not Unreal 5.8 compilation or a
claim that physical Chaos collisions or AI driving have been validated.
"""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
RIVAL = (ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp").read_text(
    encoding="utf-8"
)
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(
    encoding="utf-8"
)
STRESS = (ROOT / "Tests/Standalone/RivalRacecraftCompositionTests.cpp").read_text(
    encoding="utf-8"
)


class RivalRacecraftCompositionBindingTests(unittest.TestCase):
    def test_actual_rival_composes_grip_then_traffic_then_brake_hold(self):
        markers = (
            "AnticipateRivalCorner(",
            "Command = GripPace.Apply(",
            "YieldRivalToPlayer(Command, {",
            "FollowingBrakeHold.Apply(",
            "SetAutonomousControls(static_cast<float>(SafeTrafficCommand.Throttle)",
            "Super::Tick(DeltaSeconds);",
        )
        positions = [RIVAL.find(value, RIVAL.index("const latebrake::CircuitPoint NextTarget")) for value in markers]
        self.assertTrue(all(position >= 0 for position in positions))
        self.assertEqual(positions, sorted(positions))

    def test_real_chassis_measurement_and_traffic_cannot_replace_steering(self):
        for statement in (
            "GetTireCondition().GripMultiplier()",
            "GetTireCondition().HasValidSample()",
            "Velocity, GetActorRightVector()",
            "Player->GetVelocity()",
            "TrafficCommand.Brake && !Command.Brake",
            "SafeTrafficCommand.Steering",
        ):
            self.assertIn(statement, RIVAL)

    def test_synthetic_stress_test_is_compiled_strictly_all_platforms(self):
        for fragment in (
            "add_executable(latebrake_rival_racecraft_composition_tests",
            "RivalRacecraftCompositionTests.cpp",
            "target_compile_features(latebrake_rival_racecraft_composition_tests PRIVATE cxx_std_20)",
            "/W4 /WX",
            "-Wall -Wextra -Wpedantic -Werror",
            "add_test(NAME RivalRacecraftComposition COMMAND latebrake_rival_racecraft_composition_tests)",
        ):
            self.assertIn(fragment, CMAKE)
        for phrase in (
            "RivalGripPace.h",
            "RivalTrafficAwareness.h",
            "RivalFollowingBrakeHold.h",
            "1000; ++session",
            "100; ++frame",
            "pipeline.Reset()",
            "checkpoint arrival must not create traffic or grip braking",
            "composed rival policies must not amplify motor or steer",
        ):
            self.assertIn(phrase, STRESS)

    def test_interruptions_reset_both_independent_hysteresis_policies(self):
        for branch in ("if (!Mode || !Mode->IsRaceStartGreen()", "if (Observation.NeedsRecovery)",
                       "if (Observation.Completed)"):
            self.assertIn(branch, RIVAL)
        self.assertGreaterEqual(RIVAL.count("FollowingBrakeHold.Reset();"), 3)
        self.assertGreaterEqual(RIVAL.count("GripPace.Reset();"), 3)


if __name__ == "__main__":
    unittest.main()
