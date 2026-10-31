"""Verify the real physical rival reads Chaos yaw for bounded steering.

Source binding is not UE5.8/UHT compilation or installed-game handling QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class RivalYawRateSteeringBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.pawn = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy = read("Source/Latebrake/Public/AI/RivalYawRateSteering.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.regression = read("Tests/Standalone/RivalYawRateSteeringTests.cpp")

    def test_uses_real_physics_only_after_race_and_traffic_gates(self):
        tick = self.rival.split("void ALatebrakeRivalCar::Tick(", 1)[1]
        markers = (
            "Mode->IsRaceStartGreen()",
            "IsRacePhysicsReady()",
            "latebrake::AnticipateRivalCorner(",
            "latebrake::PlanRivalTurn(",
            "GripPace.Apply(Command, {",
            "SurfaceDamagePace.Apply(Command, {",
            "latebrake::YieldRivalToPlayer(Command, {",
            "FollowingBrakeHold.Apply(",
            "Chassis->GetPhysicsAngularVelocityInRadians()",
            "latebrake::ModerateRivalYawRate(FollowingCommand, {",
            "SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)",
        )
        self.assertEqual(
            [tick.index(marker) for marker in markers],
            sorted(tick.index(marker) for marker in markers),
        )
        self.assertEqual(tick.count("ModerateRivalYawRate("), 1)
        # An earlier guarded branch also invokes Super::Tick; assert the
        # final, applied command precedes the final physical chassis tick.
        self.assertLess(
            tick.index("SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle)"),
            tick.rindex("Super::Tick(DeltaSeconds);"),
        )
        self.assertIn('#include "AI/RivalYawRateSteering.h"', self.rival)
        self.assertIn('#include "Components/StaticMeshComponent.h"', self.rival)
        self.assertIn("static_cast<double>(ActualAngularVelocity.Z)", tick)
        self.assertIn("FlatSpeed,", tick)
        self.assertIn("TObjectPtr<UStaticMeshComponent> Chassis;", self.pawn)

    def test_never_adds_rival_physics_or_touches_player(self):
        for forbidden in (
            "AddForce(", "AddTorque(", "SetPhysics", "RaceProgress",
            "AwardCredits", "SaveGame", "UCLASS(", "GetWorld(",
            "SetAutonomousControls(", "PassCheckpoint(",
        ):
            self.assertNotIn(forbidden, self.policy)
        for required in (
            "yawRate * command.Steering <= 0.0",
            "command.Steering *= factor;",
            "command.Arrived || command.Steering == 0.0",
            "0.65 * yawBlend * speedBlend",
            "if (!sample.PhysicsReady ||",
            "std::isfinite(sample.WorldYawRadiansPerSecond)",
            "return {};",
            "return command;",
        ):
            self.assertIn(required, self.policy)

    def test_regression_is_registered_with_warnings_as_errors(self):
        self.assertIn("RivalYawRateSteeringTests.cpp", self.cmake)
        self.assertIn("NAME RivalYawRateSteering", self.cmake)
        self.assertIn(
            "latebrake_rival_yaw_rate_steering_tests PRIVATE /W4 /WX",
            self.cmake,
        )
        self.assertIn("200000", self.regression)
        self.assertIn("opposing spin preserves full correcting", self.regression)


if __name__ == "__main__":
    unittest.main()
