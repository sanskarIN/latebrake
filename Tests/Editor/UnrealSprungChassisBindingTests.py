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

    def test_real_world_gravity_and_corner_point_speed_sample(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1)[0]
        for marker in (
            "latebrake::SuspensionFrame SpringSample{};",
            "SpringSample.MassKg = static_cast<double>(Mass);",
            "World->GetGravityZ()", "GetPhysicsLinearVelocityAtPoint(Origin)",
            "WheelWorldAnchors[Index] = Origin;",
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
                        tick.index("if (GroundContact.HasDriveContact &&"))
        self.assertLess(tick.index("ApplySprungForces(SpringResponse, WheelWorldAnchors);"),
                        tick.index("Chassis->AddForce("))

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

    def test_policy_is_bounded_and_portable(self):
        for required in (
            "std::array<SuspensionCornerObservation, 4>", "PhysicsReady",
            "MassKg", "GravityCmPerSecondSquared",
            "std::isfinite(frame.GravityCmPerSecondSquared)",
            "SuspensionMaximumFrameSeconds = 0.10",
            "SuspensionRestDistanceCm = 82.0",
            "SuspensionTargetRideDistanceCm = 59.0",
            "SuspensionMaxPerCornerWeightFactor = 0.70",
            "SuspensionMinNormalZ = 0.68",
            "PointVerticalSpeedCmPerSecond",
            "ConstrainGroundContactToSuspension(",
            "sprung.ContactCount > coarse.SupportingProbes",
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


if __name__ == "__main__":
    unittest.main()
