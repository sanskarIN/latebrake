"""Unreal-source contract for real per-corner Chaos suspension integration.

The portable policy runs on the hosted C++20 matrix; these checks inspect
actual source wiring. Neither is a substitute for a licensed UE 5.8 build,
a cooked map or installed Windows/Android driving tests.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class UnrealSprungChassisBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy = read("Source/Latebrake/Public/Vehicles/SprungChassisDynamics.h")
        cls.tests = read("Tests/Standalone/SprungChassisDynamicsTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_chaos_pitch_and_roll_are_unlocked_for_real_springs(self):
        ctor = self.pawn.split("ALatebrakeCarPawn::ALatebrakeCarPawn()", 1)[1].split(
            "void ALatebrakeCarPawn::BeginPlay()", 1)[0]
        self.assertIn("Chassis->BodyInstance.bLockXRotation = false;", ctor)
        self.assertIn("Chassis->BodyInstance.bLockYRotation = false;", ctor)
        self.assertIn("Chassis->SetSimulatePhysics(true);", ctor)
        self.assertIn("Chassis->SetEnableGravity(true);", ctor)
        self.assertIn("Chassis->SetMassOverrideInKg(NAME_None, 1100.f);", ctor)
        self.assertIn("FVector(122.f, -94.f, -29.f)", ctor)
        self.assertIn("CameraBoom->bInheritPitch = false;", ctor)
        self.assertIn("CameraBoom->bInheritRoll = false;", ctor)
        self.assertIn("CameraBoom->bInheritYaw = true;", ctor)
        self.assertIn("CameraBoom->bUsePawnControlRotation = false;", ctor)

    def test_real_world_gravity_and_corner_point_speed_sample(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1)[0]
        for marker in (
            "latebrake::SuspensionFrame SpringSample{};",
            "SpringSample.MassKg = static_cast<double>(Mass);",
            "World->GetGravityZ()", "GetPhysicsLinearVelocityAtPoint(Origin)",
            "WheelWorldAnchors[Index] = Origin;",
            "constexpr float ProbeSideCm = 94.f;",
            "ChassisForward * ProbeX[Index] + ChassisRight * ProbeY[Index]",
            "World->LineTraceSingleByChannel(",
            "ECC_WorldStatic, GroundQuery", "GroundQuery.AddIgnoredActor(this);",
            "Hit.GetComponent()->GetCollisionObjectType() == ECC_WorldStatic",
            "latebrake::EvaluateChassisGroundContact(GroundSample)",
            "latebrake::CalculateSprungChassis(SpringSample)",
            "if (!SpringResponse.Valid)",
            "latebrake::ConstrainGroundContactToSuspension(",
            "LastSuspension = SpringResponse;",
            "ApplySprungForces(SpringResponse, WheelWorldAnchors);",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, tick)
        self.assertLess(tick.index("GetPhysicsLinearVelocityAtPoint(Origin)"),
                        tick.index("CalculateSprungChassis(SpringSample)"))
        self.assertLess(tick.index("CalculateSprungChassis(SpringSample)"),
                        tick.index("ApplySprungForces(SpringResponse, WheelWorldAnchors);"))

    def test_planar_drivetrain_cannot_lift_tilted_body(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1)[0]
        for required in (
            "const FVector ChassisForward = GetActorForwardVector();",
            "const FVector ChassisRight = GetActorRightVector();",
            "FVector(ChassisForward.X, ChassisForward.Y, 0.f).GetSafeNormal()",
            "const FVector Right(-Forward.Y, Forward.X, 0.f);",
            "if (Forward.IsNearlyZero())",
            "ChassisForward * ProbeX[Index] + ChassisRight * ProbeY[Index]",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
            "Chassis->AddForce(Right *",
        ):
            self.assertIn(required, tick)
        self.assertLess(tick.index("if (Forward.IsNearlyZero())"),
                        tick.index("Chassis->AddForce(Forward * DriveForce"))
        self.assertLess(tick.index("ChassisForward * ProbeX[Index]"),
                        tick.index("CalculateSprungChassis(SpringSample)"))

    def test_point_forces_are_applied_to_the_real_physics_body_only(self):
        segment = self.pawn.split("void ALatebrakeCarPawn::ApplySprungForces(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplyWheelVisualPose()", 1)[0]
        for required in (
            "Response.Valid", "Chassis->IsSimulatingPhysics()",
            "Index < 4", "Lift > 0.0", "FMath::IsFinite(Lift)",
            "Chassis->AddForceAtLocation(", "WorldAnchors[Index]",
        ):
            self.assertIn(required, segment)
        for forbidden in (
            "SetActorLocation", "PassCheckpoint(", "SaveRecords(",
            "AwardCredits(", "SetPhysicsLinearVelocity(",
            "AddTimePenalty(", "AddTorqueInRadians(",
        ):
            self.assertNotIn(forbidden, segment)

    def test_red_lights_allow_only_spring_gravity_to_settle(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1)[0]
        self.assertIn("const bool bPreGreen = Mode && !Mode->IsRaceStartGreen();",
                      tick)
        self.assertIn("Chassis->SetPhysicsLinearVelocity(", tick)
        self.assertIn("FVector(0.f, 0.f, GridVelocity.Z)", tick)
        self.assertIn("FVector(GridAngular.X, GridAngular.Y, 0.f)", tick)
        self.assertIn("ClearDriverInputs();", tick)
        end = tick.split("LastSuspension = SpringResponse;", 1)[1]
        early = end.split("// One accepted physics frame only", 1)[0]
        self.assertIn("if (bPreGreen)", early)
        self.assertIn("ApplySprungForces(SpringResponse, WheelWorldAnchors);",
                      early)
        self.assertIn("return;", early)
        self.assertLess(tick.index("LastSuspension = SpringResponse;"),
                        tick.index("if (AxleBudget.RearDriveReady &&"))
        self.assertLess(tick.index("ApplySprungForces(SpringResponse, WheelWorldAnchors);"),
                        tick.index("Chassis->AddForce("))

    def test_suspension_support_precedes_optional_drive_rejections(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1)[0]
        force_call = "ApplySprungForces(SpringResponse, WheelWorldAnchors);"
        # Exactly one call per path: one for the pre-green countdown, one
        # for accepted live driving. Normal racing never runs both.
        self.assertEqual(tick.count(force_call), 2)
        grid_branch = tick.index(
            "if (bPreGreen)\n    {\n"
            "        // Keep the grid still in XY/yaw")
        grid_call = tick.index(force_call, grid_branch)
        grid_return = tick.index("return;", grid_call)
        live_call = tick.index(force_call, grid_return)
        self.assertLess(grid_branch, grid_call)
        self.assertLess(grid_call, grid_return)
        self.assertLess(grid_return, live_call)
        # These independent policy functions can reject a driving tick.
        # Their early returns must not switch off physical suspension.
        for later in (
            "const latebrake::ArcadeSurfaceOutput Surface =",
            "const latebrake::ProgressiveRoadLoadOutput RoadLoad =",
            "const latebrake::GripLimitedBrakingOutput PhysicalBraking =",
            "const latebrake::LateralGripOutput LateralForce =",
            "const latebrake::SpeedAwareYawOutput PlannedYaw =",
            "const latebrake::AutomaticTransmissionState TransmissionFrame =",
            "Chassis->AddForce(Forward * DriveForce",
            "Chassis->AddTorqueInRadians(",
        ):
            with self.subTest(later=later):
                self.assertLess(live_call, tick.index(later))
        # No duplicate physical support may be inserted after gearbox
        # selection, even when the rear axle has valid traction.
        self.assertNotIn(
            force_call,
            tick.split("const float GearPowerFactor =", 1)[1]
        )

    def test_noncolliding_visual_wheels_ride_real_spring_travel(self):
        visual = self.pawn.split(
            "void ALatebrakeCarPawn::ApplyWheelVisualPose()", 1)[1].split(
            "void ALatebrakeCarPawn::UpdateWheelVisuals(", 1)[0]
        self.assertIn("LastSuspension.Corners[", visual)
        self.assertIn("SuspensionVisualMinTravelCm", visual)
        self.assertIn("SetRelativeLocation(", visual)
        self.assertIn("SetRelativeRotation(Visual.Rotator());", visual)
        self.assertNotIn("AddForce", visual)
        self.assertNotIn("PassCheckpoint", visual)
        self.assertIn("LastSuspension = {}; BrakeDistance.Reset();", self.pawn_h)
        self.assertIn("LastSuspension = {};", self.pawn)

    def test_real_nonshipping_telemetry_has_no_race_or_physics_side_effects(self):
        tick = self.pawn.split(
            "void ALatebrakeCarPawn::Tick(", 1
        )[1].split("void ALatebrakeCarPawn::ApplySprungForces(", 1)[0]
        debug = tick.split(
            "LastSuspension = SpringResponse;", 1
        )[1].split("if (bPreGreen)", 1)[0]
        for required in (
            "#if !UE_BUILD_SHIPPING",
            "SuspensionQaElapsedSeconds += static_cast<double>(DeltaSeconds);",
            "if (SuspensionQaElapsedSeconds >= 2.0)",
            "SuspensionQaElapsedSeconds = 0.0;",
            "LATEBRAKE_SUSPENSION v=1",
            "contacts=%d", "weight_ratio=%.3f",
            "traction=%.3f", "pitch_deg=%.1f",
            "roll_deg=%.1f", "fl_cm=%.1f", "rr_cm=%.1f",
            "arb_front_pct=%.2f arb_rear_pct=%.2f",
            "SpringSample.Corners[", "SpringResponse.TotalUpForce",
        ):
            self.assertIn(required, debug)
        for forbidden in (
            "AddForce(", "AddTorque", "SetPhysics", "SaveGame",
            "PassCheckpoint", "AwardCredits", "LineTraceSingleByChannel(",
            "WriteFile(", "Http", "SetActorLocation",
        ):
            self.assertNotIn(forbidden, debug)
        self.assertIn("#if !UE_BUILD_SHIPPING", self.pawn_h)
        self.assertIn("double SuspensionQaElapsedSeconds = 0.0;", self.pawn_h)
        neutral = self.pawn.split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1
        )[1].split("void ALatebrakeCarPawn::ThrottleInput(", 1)[0]
        self.assertIn("SuspensionQaElapsedSeconds = 0.0;", neutral)

    def test_policy_is_bounded_and_portable(self):
        for required in (
            "std::array<SuspensionCornerObservation, 4>", "PhysicsReady",
            "MassKg", "GravityCmPerSecondSquared",
            "std::isfinite(frame.GravityCmPerSecondSquared)",
            "SuspensionMaximumFrameSeconds = 0.10",
            "SuspensionRestDistanceCm = 82.0",
            "SuspensionTargetRideDistanceCm = 59.0",
            "SuspensionMaxPerCornerWeightFactor = 0.85",
            "SuspensionAntiRollSpringFraction = 0.20",
            "SuspensionAntiRollDamperFraction = 0.08",
            "SuspensionAntiRollMaxWeightFractionPerAxle = 0.08",
            "std::array<double, 2> AntiRollTransfer{}",
            "if (!l.Contact || !r.Contact",
            "l.UpForce += transfer;",
            "r.UpForce -= transfer;",
            "r.UpForce += transfer;",
            "l.UpForce -= transfer;",
            "SuspensionMinNormalZ = 0.68",
            "PointVerticalSpeedCmPerSecond",
            "ConstrainGroundContactToSuspension(",
            "sprung.ContactCount > coarse.SupportingProbes",
            "sprung.TotalUpForce / sprung.WeightForce",
            "coarse.ContactTractionScale =",
            "std::min(countBasedGrip, supportedLoadFraction)",
            "coarse.HasDriveContact = sprung.ContactCount >= 2",
            "std::clamp(requested, 0.0, maxCornerForce)",
        ):
            self.assertIn(required, self.policy)
        for forbidden in (
            "UCLASS(", "FVector", "GetWorld(", "SetPhysics",
            "AddForce(", "SaveRecords(", "PassCheckpoint(", "SpawnActor(",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("SprungChassisDynamicsTests.cpp", self.cmake)
        self.assertIn("NAME SprungChassisDynamics", self.cmake)
        self.assertIn("latebrake_sprung_chassis_dynamics_tests PRIVATE /W4 /WX", self.cmake)
        self.assertIn("120000", self.tests)
        self.assertIn("anti-roll cannot create net gravity-compensating", self.tests)
        self.assertIn("anti-roll cannot lever an airborne wheel", self.tests)
        self.assertIn("front bar conserves sum of existing spring loads", self.tests)
        self.assertIn("SprungChassisRideHeightTests.cpp", self.cmake)
        self.assertIn("NAME SprungChassisRideHeight", self.cmake)
        heave = read("Tests/Standalone/SprungChassisRideHeightTests.cpp")
        for token in (
            "for (const int fps : {30, 60, 120})",
            "for (const double height : {80.0, 180.0})",
            "SprungChassisRideHeight", "chassisHalfHeightCm",
            "spring equilibrium scales with actual Unreal world gravity",
            "run.MinimumHeight > chassisHalfHeightCm + 2.0",
        ):
            self.assertIn(token, heave)


if __name__ == "__main__":
    unittest.main()
