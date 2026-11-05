"""Production source contract: optional invalid drive cannot freeze real springs.

This inspects Unreal C++ syntax/ordering; it does NOT replace licensed UE5.8
compile, Chaos simulation, packaged playtests or actual fault injection.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN_H = (ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h").read_text(encoding="utf-8")
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
DOC = (ROOT / "docs/SPRUNG_CHASSIS_QA.md").read_text(encoding="utf-8")


class UnrealOptionalDriveRecoveryBindingTests(unittest.TestCase):
    def test_pre_motor_rejections_keep_physical_momentum_after_springs(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1)[0]
        spring = tick.index("ApplySprungForces(SpringResponse, WheelWorldAnchors);",
                            tick.index("if (bPreGreen)", tick.index("LATEBRAKE_SUSPENSION v=1")))
        # Skip the countdown's exclusive spring call and find the live one.
        spring = tick.index("ApplySprungForces(SpringResponse, WheelWorldAnchors);",
                            spring + 1)
        motor = tick.index("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle")
        yaw = tick.index("Chassis->AddTorqueInRadians(")
        candidates = (
            "Surface", "RoadLoad", "PhysicalBraking", "LateralForce",
            "PlannedYaw", "TransmissionFrame",
        )
        for policy in candidates:
            with self.subTest(policy=policy):
                expected = (
                    "if (!" + policy + ".Valid)\\n"
                    "    {\\n"
                    "        RejectOptionalDriveAfterSpring(SpringResponse);\\n"
                    "        return;\\n"
                    "    }"
                )
                # Only one guarded return; no accidental hard freeze.
                self.assertEqual(tick.count(expected.replace("\\n", "\n")), 1)
                position = tick.index(expected.replace("\\n", "\n"))
                self.assertLess(spring, position)
                self.assertLess(position, motor)
                self.assertLess(position, yaw)
        self.assertIn(
            "RejectOptionalDriveAfterSpring(\n"
            "    const latebrake::SuspensionResponse& AcceptedSprings)",
            PAWN
        )
        self.assertIn("void RejectOptionalDriveAfterSpring(", PAWN_H)

    def test_helper_revokes_hud_and_drive_without_false_chaos_stop(self):
        helper = PAWN.split("void ALatebrakeCarPawn::RejectOptionalDriveAfterSpring(", 1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        for marker in (
            "ClearDriverInputs();", "ApplySignalLights(false, false, 0.0);",
            "WheelMotion.Suspend();", "LastSuspension = AcceptedSprings;",
            "ApplyWheelVisualPose();", "DrivingStats.Suspend();",
            "DriftMeter.Suspend();", "GForceMeter.Suspend();",
            "SlipAngleMeter.Suspend();",
        ):
            self.assertIn(marker, helper)
        self.assertLess(helper.index("ClearDriverInputs();"),
                        helper.index("LastSuspension = AcceptedSprings;"))
        for forbidden in (
            "Chassis->SetPhysicsLinearVelocity", "Chassis->SetPhysicsAngularVelocity",
            "NeutralizeMotionWhenDrivingBlocked(", "Chassis->AddForce(",
            "Chassis->AddTorque", "AddForceAtLocation", "AwardCredits(",
            "PassCheckpoint(", "SetActorLocation", "SaveGame(", "LineTraceSingle",
        ):
            self.assertNotIn(forbidden, helper)
        self.assertIn("LastGroundContact = {};", PAWN.split(
            "void ALatebrakeCarPawn::ClearDriverInputs(", 1)[1].split(
            "void ALatebrakeCarPawn::RejectOptionalDriveAfterSpring(", 1
        )[0])
        self.assertIn("## Failed optional drive policy", DOC)

    def test_hard_invalid_chaos_and_pause_guards_stay_intact(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1)[0]
        hard = tick.index("if (!Mode ||")
        physics = tick.index("if (!IsRacePhysicsReady())")
        optional = tick.index("if (!Surface.Valid)")
        self.assertLess(hard, physics)
        self.assertLess(physics, optional)
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", tick[:physics+140])
        late = tick.split("if (!ReverseLimit.Valid)", 1)[1][:160]
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", late)
        neutral = PAWN.split("void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[1].split(
            "void ALatebrakeCarPawn::ThrottleInput(", 1)[0]
        self.assertIn("SetPhysicsLinearVelocity(FVector::ZeroVector)", neutral)
        self.assertIn("SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector)", neutral)


if __name__ == "__main__":
    unittest.main()
