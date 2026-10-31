"""Protect real AI surface and accepted-impact pace integration.

Source-level checks only: no UE 5.8 compilation or installed racing QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RivalSurfaceDamagePaceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rival = load("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.header = load("Source/Latebrake/Public/AI/LatebrakeRivalCar.h")
        cls.policy = load("Source/Latebrake/Public/AI/RivalSurfaceDamagePace.h")
        cls.cmake = load("Tests/Standalone/CMakeLists.txt")

    def test_only_real_rival_reads_previously_measured_chassis_surface(self):
        self.assertIn('#include "AI/RivalSurfaceDamagePace.h"', self.header)
        self.assertIn("latebrake::RivalSurfaceDamagePace SurfaceDamagePace;", self.header)
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        markers = (
            "Mode->IsRaceStartGreen()",
            "IsRacePhysicsReady()",
            "latebrake::CanDriveRival(",
            "const FVector Position = GetActorLocation();",
            "latebrake::AnticipateRivalCorner(",
            "GripPace.Apply(Command, {",
            "GetSurfaceTraction().IsOffPavement()",
            "SurfaceDamagePace.Apply(Command, {",
            "GetCollisionDamage().LateralGripMultiplier()",
            "GetSurfaceTraction().HasValidSample()",
            "GetCollisionDamage().HasEligibleFrame()",
            "latebrake::YieldRivalToPlayer(Command, {",
            "FollowingBrakeHold.Apply(",
            "SetAutonomousControls(static_cast<float>(SafeTrafficCommand.Throttle)",
        )
        offsets = [tick.index(m) for m in markers]
        self.assertEqual(offsets, sorted(offsets))
        self.assertIn("Velocity, GetActorRightVector()", tick)
        self.assertIn("ArcadeSurfaceTraction::OffPavementGripMultiplier", tick)

    def test_race_and_teleport_clear_any_old_brake_state(self):
        for name in ("ResetPilot", "ConfigurePilot"):
            block = self.rival.split(
                "void ALatebrakeRivalCar::" + name + "(", 1
            )[1].split("void ALatebrakeRivalCar::", 1)[0]
            self.assertIn("SurfaceDamagePace.Reset();", block)
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        self.assertGreaterEqual(tick.count("SurfaceDamagePace.Reset();"), 3)
        self.assertIn("if (Observation.NeedsRecovery)", tick)
        self.assertIn("if (Observation.Completed)", tick)

    def test_policy_can_only_slow_brake_and_leave_steering_untouched(self):
        for required in (
            "std::isfinite(", "MaxFrameSeconds = 0.10",
            "ArcadeSurfaceTraction::OffPavementGripMultiplier",
            "ArcadeCollisionDamage::MinLateralGripMultiplier",
            "!sample.SurfaceReady || !sample.ImpactFrameReady",
            "command.Throttle = std::min(command.Throttle, permitted)",
            "command.Throttle = 0.0",
            "command.Brake = true", "return command;", "return {};",
            "speed >= 650.0", "RemainingBrakeSeconds",
        ):
            self.assertIn(required, self.policy)
        for forbidden in (
            "GetWorld(", "UCLASS(", "AddForce(", "AddTorque(",
            "SaveGame", "RaceProgress", "AwardCredits(",
            "PassCheckpoint(", "SetAutonomousControls(", "SetActorLocation",
        ):
            self.assertNotIn(forbidden, self.policy)

    def test_stress_suite_is_platform_portable(self):
        self.assertIn("RivalSurfaceDamagePaceTests.cpp", self.cmake)
        self.assertIn("NAME RivalSurfaceDamagePace", self.cmake)
        self.assertIn("latebrake_rival_surface_damage_pace_tests PRIVATE /W4 /WX",
                      self.cmake)
        self.assertIn("200000", load("Tests/Standalone/RivalSurfaceDamagePaceTests.cpp"))


if __name__ == "__main__":
    unittest.main()
