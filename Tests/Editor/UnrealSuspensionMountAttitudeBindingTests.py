"""Ensure actual Chaos chassis-up orientation gates physical spring force.

Source-binding verification ONLY: this does not compile Unreal Engine,
validate collision meshes, or prove a playable car can right itself.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "Source/Latebrake/Public/Vehicles/SprungChassisDynamics.h").read_text(encoding="utf-8")
TESTS = (ROOT / "Tests/Standalone/SuspensionMountAttitudeTests.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
DOCS = (ROOT / "docs/SPRUNG_CHASSIS_QA.md").read_text(encoding="utf-8")


class UnrealSuspensionMountAttitudeBindingTests(unittest.TestCase):
    def test_orientation_is_sampled_from_simulated_chassis_not_hud(self):
        tick = PAWN.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySprungForces(", 1)[0]
        origin = tick.index("const FVector GroundOrigin = Chassis->GetComponentLocation();")
        measured = tick.index("static_cast<double>(Chassis->GetUpVector().Z);")
        solved = tick.index("latebrake::CalculateSprungChassis(SpringSample)")
        dispatched = tick.index("ApplySprungForces(SpringResponse, WheelWorldAnchors);")
        self.assertLess(measured, origin)
        self.assertLess(origin, solved)
        self.assertLess(solved, dispatched)
        sample = tick.split("SpringSample.ChassisUpZ =", 1)[1].split(
            "const FVector GroundOrigin", 1
        )[0]
        self.assertIn("Chassis->GetUpVector().Z", sample)
        for forbidden in ("Camera", "GetControlRotation", "GroundContact", "World->GetGravity",
                          "GetActorUpVector", "FVector::UpVector"):
            self.assertNotIn(forbidden, sample)

    def test_no_downward_wheel_support_on_side_or_roof(self):
        for required in (
            "double ChassisUpZ = 1.0;", "SuspensionFullMountUpZ = 0.50;",
            "SuspensionNoMountUpZ = 0.25;",
            "!std::isfinite(frame.ChassisUpZ)",
            "frame.ChassisUpZ < -1.0 || frame.ChassisUpZ > 1.0",
            "SuspensionMountLoadScale(frame.ChassisUpZ)",
            "if (mountScale <= 0.0)",
            "const double spring = unscaledSpring * mountScale;",
            "std::sqrt(unscaledSpring * quarterMass) * mountScale;",
            "SuspensionMaxPerCornerWeightFactor * mountScale;",
            "SuspensionAntiRollMaxWeightFractionPerAxle *",
        ):
            self.assertIn(required, SOURCE)
        self.assertLess(SOURCE.index("if (mountScale <= 0.0)"),
                        SOURCE.index("for (unsigned i = 0; i < result.Corners.size()"))
        self.assertIn("return result;", SOURCE.split("if (mountScale <= 0.0)", 1)[1][:130])
        for forbidden in ("UCLASS(", "Chassis->SetPhysicsLinearVelocity",
                          "SaveGame(", "PassCheckpoint(", "AddForce("):
            self.assertNotIn(forbidden, SOURCE)

    def test_standalone_matrix_and_qa_track_real_roll_edge_cases(self):
        self.assertIn("NAME SuspensionMountAttitude", CMAKE)
        self.assertIn("SuspensionMountAttitudeTests.cpp", CMAKE)
        self.assertIn("120000", TESTS)
        self.assertIn("!deniedAxle.RearDriveReady", TESTS)
        self.assertIn("!deniedAxle.FrontSteeringReady", TESTS)
        self.assertIn("unevenHalf.AntiRollTransfer[0]", TESTS)
        self.assertIn("## Roll and roof contact", DOCS)
        self.assertIn("real Chaos", DOCS)


if __name__ == "__main__":
    unittest.main()
