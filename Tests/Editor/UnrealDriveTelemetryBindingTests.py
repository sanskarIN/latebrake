"""Read-only contract for real post-force Unreal Development diagnostics.

No source checks prove licensed UE5.8 compilation, cooked Win64 playability,
actual force response, or trustworthiness of user-supplied log text.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(
    encoding="utf-8")
SCRIPT = (ROOT / "scripts/AnalyzeDriveTelemetry.py").read_text(encoding="utf-8")
MANUAL = (ROOT / "docs/SPRUNG_CHASSIS_QA.md").read_text(encoding="utf-8")


class UnrealDriveTelemetryBindingTests(unittest.TestCase):
    def test_nonshipping_log_shares_existing_bounded_suspension_cadence(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertEqual(tick.count("SuspensionQaSchedule.ShouldReport("), 1)
        self.assertEqual(tick.count("LATEBRAKE_SUSPENSION v=1"), 1)
        self.assertEqual(tick.count("LATEBRAKE_DRIVE v=1"), 1)
        self.assertIn("#if !UE_BUILD_SHIPPING\n"
                      "    // Real Development-build evidence", tick)
        self.assertIn("if (bReportSuspensionDriveEvidence)", tick)
        # The Tick's INITIAL pre-green input-neutralization occurs
        # before spring sampling; this assertion targets the later
        # mutually exclusive GRID RETURN after the suspension log.
        grid_return = "if (bPreGreen)\\n    {\\n        // Keep the grid still"
        self.assertLess(tick.index("SuspensionQaSchedule.ShouldReport("),
                        tick.index(grid_return))
        self.assertLess(tick.index(grid_return),
                        tick.index("LATEBRAKE_DRIVE v=1"))
        self.assertLess(tick.index("Chassis->AddTorqueInRadians("),
                        tick.index("LATEBRAKE_DRIVE v=1"))
        self.assertLess(tick.index("if (SpeedGovernor.Active)"),
                        tick.index("LATEBRAKE_DRIVE v=1"))
        self.assertLess(tick.index("LATEBRAKE_DRIVE v=1"),
                        tick.index("const FVector PostCapVelocity"))
        diagnostic = tick.split("    // Real Development-build evidence", 1)[1].split(
            "\n#endif", 1)[0]
        for forbidden in ("Chassis->AddForce(", "Chassis->AddTorqueInRadians(",
                          "SaveGame(", "AwardCredits(", "PassCheckpoint(",
                          "LineTraceSingleByChannel(", "SpawnActor(", "WriteFile("):
            self.assertNotIn(forbidden, diagnostic)

    def test_force_dispatched_branches_are_mutually_exclusive_in_log(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        diagnostic = tick.split("    // Real Development-build evidence", 1)[1].split(
            "\n#endif", 1)[0]
        for required in (
            "bMotorSubmitted", "!bEffectiveBraking && !OpposingBraking.OpposingThrottle",
            "bOpposingSubmitted", "OpposingBraking.OpposingThrottle",
            "SubmittedMotorForce", "MotorTraction.ForwardForce",
            "SubmittedOpposingForce", "OpposingBraking.ForwardForce",
            "bEffectiveBraking && GroundContact.HasDriveContact",
            "PhysicalBraking.ForwardForce", "ContactTractionScale",
            "LateralForce.RightForce", "TurnTorque",
            "FrontSteeringTractionScale", "CurrentDraftAppliedFraction",
            "AngularVelocity.Z", "LongitudinalSpeed", "LateralSpeed",
        ):
            self.assertIn(required, diagnostic)
        self.assertIn("LATEBRAKE_DRIVE v=1 driver=%s mode=%s", diagnostic)
        for key in ("motor_force=", "opposing_force=", "service_force=",
                    "lateral_force=", "yaw_torque=", "draft_pct="):
            self.assertIn(key, diagnostic)
        self.assertIn("UNVERIFIED_OFFLINE_DRIVE_DIAGNOSTIC", SCRIPT)
        self.assertIn('"gameplay_verified": False', SCRIPT)
        self.assertIn('"ready_for_release": False', SCRIPT)
        self.assertIn("AnalyzeDriveTelemetry.py", MANUAL)


if __name__ == "__main__":
    unittest.main()
