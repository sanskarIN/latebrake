"""Actual Unreal pawn binding for suspension contact-loss and grip-return safety.

Source evidence ONLY; does not claim UE5.8 Editor/Chaos gameplay qualification.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class UnrealSprungGripReattachmentBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (
            ROOT / "Source/Latebrake/Public/Vehicles/SprungGripReattachment.h"
        ).read_text(encoding="utf-8")
        cls.pawn_header = (
            ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
        ).read_text(encoding="utf-8")
        cls.pawn = (
            ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
        ).read_text(encoding="utf-8")
        cls.tick = cls.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        cls.cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(
            encoding="utf-8"
        )

    def test_real_measured_spring_and_axle_observation_precedes_filter(self):
        tick = self.tick
        stages = (
            "latebrake::CalculateSprungChassis(SpringSample)",
            "latebrake::ConstrainGroundContactToSuspension(",
            "latebrake::CalculateAxleTractionBudget(GroundContact, SpringResponse)",
            "GripReattachment.Advance(",
            "GroundContact = GripFrame.Ground;",
            "const latebrake::AxleTractionBudget AxleBudget = GripFrame.Axles;",
            "ApplySprungForces(SpringResponse, WheelWorldAnchors);",
            "const float RearDriveTractionScale =",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
            "Chassis->AddTorqueInRadians(",
        )
        for stage in stages:
            self.assertIn(stage, tick)
        for earlier, later in zip(stages, stages[1:]):
            self.assertLess(tick.index(earlier), tick.index(later))

    def test_invalid_samples_and_gameplay_boundaries_reset_old_grip(self):
        self.assertIn(
            '#include "Vehicles/SprungGripReattachment.h"', self.pawn_header
        )
        self.assertIn("latebrake::SprungGripReattachment GripReattachment;",
                      self.pawn_header)
        self.assertIn("LastSuspension = {}; GripReattachment.Reset();",
                      self.pawn_header)
        for method in (
            "NeutralizeMotionWhenDrivingBlocked()",
            "TogglePause()",
        ):
            body = self.pawn.split(
                "void ALatebrakeCarPawn::" + method + "\n{", 1
            )[1][:480]
            self.assertIn("GripReattachment.Reset();", body)
        self.assertIn(
            "LastSuspension = {};\n        GripReattachment.Reset();",
            self.tick
        )
        self.assertIn(
            "if (!GripFrame.Valid)\n    {\n"
            "        NeutralizeMotionWhenDrivingBlocked();",
            self.tick
        )
        self.assertGreaterEqual(self.pawn.count("GripReattachment.Reset();"), 5)

    def test_grid_keeps_measured_traction_warm_without_starting_drive(self):
        # The live 3-2-1 countdown runs ClearDriverInputs every Tick.
        # It must NOT reset the ramp on every frame, or GO will begin
        # with an artificial grip deficit despite a settled chassis.
        self.assertIn(
            "void ClearDriverInputs(bool bResetGripAuthority = true);",
            self.pawn_header
        )
        self.assertIn(
            "void ALatebrakeCarPawn::ClearDriverInputs(bool bResetGripAuthority)",
            self.pawn
        )
        inputs = self.pawn.split(
            "void ALatebrakeCarPawn::ClearDriverInputs(", 1
        )[1].split("void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked(", 1)[0]
        self.assertIn("if (bResetGripAuthority)", inputs)
        self.assertIn("GripReattachment.Reset();", inputs)
        self.assertIn(
            "if (bPreGreen)\n    {\n"
            "        // Every countdown frame neutralizes driving input",
            self.tick
        )
        self.assertIn("ClearDriverInputs(false);", self.tick)
        self.assertLess(
            self.tick.index("ClearDriverInputs(false);"),
            self.tick.index("GripReattachment.Advance(")
        )
        grid_dispatch = self.tick.split("if (bPreGreen)\n    {\n"
                                        "        // Keep the grid still in XY/yaw", 1)[1]
        grid_dispatch = grid_dispatch.split("return;", 1)[0]
        self.assertIn("ApplySprungForces(SpringResponse, WheelWorldAnchors);",
                      grid_dispatch)
        self.assertNotIn("Chassis->AddForce(Forward * DriveForce", grid_dispatch)
        self.assertNotIn("Chassis->AddTorqueInRadians(", grid_dispatch)

    def test_loss_is_immediate_and_regain_is_bounded_without_score_mutation(self):
        header = self.header
        for required in (
            "GripReattachmentRisePerSecond = 6.0",
            "GripReattachmentMaxFrameSeconds = 0.10",
            "!ValidScale(axles.FrontAxleLoadRatio)",
            "!ValidScale(axles.RearAxleLoadRatio)",
            "ground.SupportingProbes == 3 ? 0.75",
            "ground.SupportingProbes == 2 ? 0.45",
            "axles.RearAxleLoadRatio *",
            "axles.FrontAxleLoadRatio *",
            "OneDrivenWheelGripFraction",
            "OneSteeringWheelGripFraction",
            "std::min(axles.RearDriveTractionScale, RearGrip + rise)",
            "std::min(axles.FrontSteeringTractionScale, FrontGrip + rise)",
            "result.Ground = ground;",
            "result.Axles.RearDriveReady =",
            "result.Axles.FrontSteeringReady =",
            "void Reset() noexcept",
            "Reset();\n            return {};",
        ):
            self.assertIn(required, header)
        for banned in (
            "GetWorld(", "AddForce(", "SetPhysicsLinearVelocity(",
            "SaveGame(", "PassCheckpoint(", "AwardCredits(",
            "Http", "UCLASS(", "UFUNCTION(",
        ):
            self.assertNotIn(banned, header)
        self.assertIn("SprungGripReattachmentTests.cpp", self.cmake)
        self.assertIn("NAME SprungGripReattachment", self.cmake)
        self.assertIn("100000", (
            ROOT / "Tests/Standalone/SprungGripReattachmentTests.cpp"
        ).read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
