"""Source-only contract: finite frame hitches must not hard-freeze Chaos.

A licensed UE 5.8 Editor/Win64 driving QA run is REQUIRED before this can be
called a validated smooth hitch recovery in a playable game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
SPRINGS = (ROOT / "Source/Latebrake/Public/Vehicles/SprungChassisDynamics.h").read_text(encoding="utf-8")
TESTS = (ROOT / "Tests/Standalone/SprungChassisDynamicsTests.cpp").read_text(encoding="utf-8")


class UnrealFiniteHitchPhysicsBindingTests(unittest.TestCase):
    def test_valid_chaos_hitch_quarantines_forces_before_ground_sampling(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1
        )[0]
        marker = ("if (FMath::IsFinite(DeltaSeconds) &&\n"
                  "        static_cast<double>(DeltaSeconds) >\n"
                  "            latebrake::SuspensionMaximumFrameSeconds)")
        self.assertEqual(tick.count(marker), 1)
        guard_start = tick.index(marker)
        following = tick.index(
            "if (!Chassis->IsSimulatingPhysics() || !FMath::IsFinite(DeltaSeconds)",
            guard_start
        )
        hitch = tick[guard_start:following]
        # In the countdown, zero horizontal/yaw grid momentum BEFORE
        # dropping untrusted long-frame force observations.
        self.assertLess(tick.index("FVector(0.f, 0.f, GridVelocity.Z)"),
                        guard_start)
        self.assertLess(tick.index("FVector(GridAngular.X, GridAngular.Y, 0.f)"),
                        guard_start)
        self.assertLess(following, tick.index("SmoothThrottle("))
        self.assertLess(following, tick.index("World->LineTraceSingleByChannel("))
        self.assertLess(following, tick.index("GripReattachment.Advance("))
        for required in (
            "ClearDriverInputs();",
            "ApplySignalLights(false, false, 0.0);",
            "WheelMotion.Suspend();",
            "ApplyWheelVisualPose();",
            "DrivingStats.Suspend();",
            "DriftMeter.Suspend();",
            "GForceMeter.Suspend();",
            "SlipAngleMeter.Suspend();",
            "return;",
        ):
            with self.subTest(required=required):
                self.assertIn(required, hitch)
        for forbidden in (
            "NeutralizeMotionWhenDrivingBlocked(",
            "Chassis->SetPhysicsLinearVelocity(",
            "Chassis->SetPhysicsAngularVelocity",
            "Chassis->AddForce(",
            "Chassis->AddTorque",
            "Chassis->AddForceAtLocation(",
            "PassCheckpoint(",
            "AwardCredits(",
            "SaveGame(",
            "LineTraceSingleByChannel(",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, hitch)

    def test_real_pause_and_invalid_body_still_neutralize_motion(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1
        )[0]
        self.assertLess(tick.index("if (!Mode ||"),
                        tick.index("if (!IsRacePhysicsReady())"))
        self.assertLess(tick.index("if (!IsRacePhysicsReady())"),
                        tick.index("if (FMath::IsFinite(DeltaSeconds) &&"))
        neutralize = PAWN.split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1
        )[1].split("void ALatebrakeCarPawn::ThrottleInput(", 1)[0]
        self.assertIn("Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);",
                      neutralize)
        self.assertIn("Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);",
                      neutralize)
        # This test does not weaken existing pause/race integrity safeguards.
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", tick)

    def test_short_frame_spring_limits_remain_strict(self):
        self.assertIn("SuspensionMaximumFrameSeconds = 0.10", SPRINGS)
        self.assertIn("s.DeltaSeconds = SuspensionMaximumFrameSeconds;", TESTS)
        self.assertIn(
            "s.DeltaSeconds = SuspensionMaximumFrameSeconds + 0.000001;",
            TESTS
        )
        self.assertIn("s.DeltaSeconds = 0.11;", TESTS)


if __name__ == "__main__":
    unittest.main()
