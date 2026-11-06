"""Source-only Unreal binding of smooth physical planar overspeed force.

Does not run Unreal 5.8, inspect real Chaos contacts or qualify a game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class PlanarSpeedGovernorBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.policy = read("Source/Latebrake/Public/Vehicles/PlanarSpeedGovernor.h")
        cls.tests = read("Tests/Standalone/PlanarSpeedGovernorTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.docs = read("docs/PROGRESSIVE_ROAD_LOAD_QA.md")
        cls.tick = cls.pawn.split(
            "void ALatebrakeCarPawn::Tick(", 1
        )[1].split("void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]

    def test_true_chaos_planar_speed_is_checked_before_thrust(self):
        tick = self.tick
        for term in (
            '#include "Vehicles/PlanarSpeedGovernor.h"',
            "latebrake::PlanarSpeedGovernorOutput SpeedGovernor",
            "latebrake::CalculatePlanarSpeedGovernor({",
            "static_cast<double>(Velocity.X)",
            "static_cast<double>(Velocity.Y)",
            "static_cast<double>(Mass)",
            "static_cast<double>(ActiveSpeedCap), bArcade",
            "if (!SpeedGovernor.Valid)",
            "RejectOptionalDriveAfterSpring(SpringResponse);",
            "if (SpeedGovernor.Active)",
            "static_cast<float>(SpeedGovernor.ForceX)",
            "static_cast<float>(SpeedGovernor.ForceY), 0.f",
        ):
            self.assertIn(term, self.pawn if term.startswith("#include") else tick)
        self.assertEqual(tick.count("CalculatePlanarSpeedGovernor({"), 1)
        stage = (
            "ApplySprungForces(SpringResponse, WheelWorldAnchors);",
            "NitroBoost.Step(",
            "const float ActiveSpeedCap =",
            "latebrake::CalculatePlanarSpeedGovernor({",
            "if (!SpeedGovernor.Valid)",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
            "if (SpeedGovernor.Active)",
            "if (ReverseGovernor.Active)",
        )
        self.assertEqual([tick.index(x) for x in stage],
                         sorted(tick.index(x) for x in stage))

    def test_airborne_or_unloaded_suspension_cannot_add_cap_braking(self):
        tick = self.tick
        governor = tick.split(
            "latebrake::CalculatePlanarSpeedGovernor({", 1
        )[1].split("if (!SpeedGovernor.Valid)", 1)[0]
        self.assertIn(
            "static_cast<double>(ActiveSpeedCap), bArcade,\n"
            "            GroundContact.ContactTractionScale", governor
        )
        self.assertLess(tick.index("CalculateSprungChassis(SpringSample)"),
                        tick.index("ConstrainGroundContactToSuspension("))
        self.assertLess(tick.index("ConstrainGroundContactToSuspension("),
                        tick.index("CalculatePlanarSpeedGovernor({"))
        self.assertIn("double ContactTractionScale = 1.0;", self.policy)
        self.assertIn("!std::isfinite(sample.ContactTractionScale)",
                      self.policy)
        self.assertIn("sample.ContactTractionScale <= 1.0e-6", self.policy)
        self.assertIn("}) * sample.ContactTractionScale;", self.policy)
        for forbidden in (
            "LineTraceSingleByChannel(", "GetActorLocation(",
            "PassCheckpoint(", "SaveGame("
        ):
            self.assertNotIn(forbidden, governor)
        self.assertIn("airborne overspeed cannot manufacture", self.tests)
        self.assertIn("120000", self.tests)
        self.assertIn("mid-air", self.docs.lower())

    def test_no_global_planar_velocity_teleport_at_nitro_cap(self):
        tick = self.tick
        for banned in (
            "FlatVelocity.GetClampedToMaxSize(ActiveSpeedCap)",
            "Chassis->SetPhysicsLinearVelocity(FVector(Limited.X",
            "if (Speed > ActiveSpeedCap)",
        ):
            self.assertNotIn(banned, tick)
        # Reverse overspeed now ALSO applies finite braking force, never
        # a direct XY velocity snap, and has its own rear-contact budget.
        self.assertIn("latebrake::CalculateReverseSpeedGovernor({", tick)
        self.assertIn("if (ReverseGovernor.Active)", tick)
        self.assertNotIn("latebrake::LimitPlanarReverseSpeed(", tick)
        self.assertNotIn("const FVector PostCapVelocity", tick)
        governor_force = tick.split(
            "if (SpeedGovernor.Active)", 1
        )[1].split("#if !UE_BUILD_SHIPPING", 1)[0]
        self.assertIn("Chassis->AddForce(FVector(", governor_force)
        self.assertNotIn("SetPhysicsLinearVelocity(", governor_force)
        self.assertNotIn("SetPhysicsAngularVelocity", governor_force)

    def test_policy_strictly_bounded_without_race_authority(self):
        for term in (
            "PlanarGovernorMaximumFrameSeconds = 0.10",
            "PlanarGovernorMaximumSpeedCmPerSecond = 15000.0",
            "PlanarGovernorResponsePerSecond = 3.0",
            "PlanarGovernorArcadeMaxDeceleration = 650.0",
            "PlanarGovernorSimulationMaxDeceleration = 500.0",
            "std::hypot(sample.VelocityX, sample.VelocityY)",
            "excess / sample.DeltaSeconds",
            "sample.VelocityX * scale",
            "sample.VelocityY * scale",
        ):
            self.assertIn(term, self.policy)
        for banned in (
            "GetWorld(", "AddForce(", "SetPhysicsLinearVelocity(",
            "SaveGame(", "PassCheckpoint(", "AwardCredits(", "UCLASS(",
        ):
            self.assertNotIn(banned, self.policy)
        self.assertIn("120000", self.tests)
        self.assertIn("NAME PlanarSpeedGovernor", self.cmake)
        self.assertIn("PlanarSpeedGovernorTests.cpp", self.cmake)
        self.assertIn("Win64", self.docs)
        self.assertIn("not physically calibrated", self.docs)


if __name__ == "__main__":
    unittest.main()
