"""Source contract: qualified physical reverse overspeed force, no XY snap.

Real Unreal 5.8 compile/cook and installed Win64 driving remain mandatory.
The older instantaneous pure reference is retained only as a numerical
baseline; it must NEVER write physical Chaos velocity in normal driving.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


class ReverseSpeedBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.car = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.policy = read("Source/Latebrake/Public/Vehicles/ReverseSpeedGovernor.h")
        cls.reference = read("Source/Latebrake/Public/Vehicles/ArcadeReverseSpeedLimit.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.tests = read("Tests/Standalone/ReverseSpeedGovernorTests.cpp")
        cls.docs = read("docs/REVERSE_OVERSPEED_FORCE_QA.md")
        cls.tick = cls.car.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]

    def test_real_shared_pawn_no_longer_teleports_valid_reverse_motion(self):
        tick = self.tick
        self.assertIn('#include "Vehicles/ReverseSpeedGovernor.h"', self.car)
        for marker in (
            "latebrake::CalculateReverseSpeedGovernor({",
            "const latebrake::ReverseSpeedGovernorOutput ReverseGovernor",
            "if (!ReverseGovernor.Valid)",
            "if (ReverseGovernor.Active)",
            "static_cast<float>(ReverseGovernor.ForceX)",
            "static_cast<float>(ReverseGovernor.ForceY), 0.f",
        ):
            self.assertIn(marker, tick)
        for forbidden in (
            "latebrake::LimitPlanarReverseSpeed(",
            "ReverseLimit.Limited",
            "const FVector PostCapVelocity",
            "ReverseLimit.VelocityX",
            "ReverseLimit.VelocityY",
        ):
            self.assertNotIn(forbidden, tick)
        physical = tick.split("const latebrake::ReverseSpeedGovernorOutput", 1)[1]
        self.assertNotIn("Chassis->SetPhysicsLinearVelocity(", physical)
        self.assertIn("RejectOptionalDriveAfterSpring(SpringResponse);", physical)
        self.assertNotIn("NeutralizeMotionWhenDrivingBlocked();", physical)
        self.assertIn("MaximumReverseSpeedCmPerSecond", self.policy)
        self.assertIn("inline ReverseSpeedLimitOutput LimitPlanarReverseSpeed(", self.reference)

    def test_chassis_contact_steering_and_motor_paths_use_one_physical_budget(self):
        tick = self.tick
        ordered = (
            "ApplySprungForces(SpringResponse, WheelWorldAnchors);",
            "latebrake::CalculateOpposingThrottleBraking({",
            "latebrake::CalculateReverseSpeedGovernor({",
            "latebrake::CalculateLateralGripForce({",
            "EngineHeat.Observe({",
            "const double AuthorizedMotorThrottle",
            "latebrake::CalculateTractionLimitedPropulsion({",
            "if (ReverseGovernor.Active)",
            "LATEBRAKE_DRIVE v=1",
        )
        self.assertEqual([tick.index(s) for s in ordered],
                         sorted(tick.index(s) for s in ordered))
        for marker in (
            "GroundContact.ContactTractionScale, bArcade,",
            "AxleBudget.RearDriveReady ? AxleBudget.RearDriveTractionScale : 0.0",
            "bEffectiveBraking || OpposingBraking.OpposingThrottle",
            "ReverseGovernor.WholeBodyBudgetDeceleration,",
            "ReverseGovernor.ReverseMotorBlocked ||",
            "!ReverseGovernor.ReverseMotorBlocked)",
            "AuthorizedMotorThrottle = ReverseGovernor.ReverseMotorBlocked",
            "Chassis->AddForce(FVector(",
        ):
            self.assertIn(marker, tick)
        # No new force may run without the authenticated race/physics frame.
        self.assertLess(tick.index("if (!IsRacePhysicsReady())"),
                        tick.index("CalculateReverseSpeedGovernor({"))
        self.assertLess(tick.index("if (!ReverseGovernor.Valid)"),
                        tick.index("if (ReverseGovernor.Active)"))

    def test_no_extra_scoring_or_tire_simulation_authority(self):
        for forbidden in (
            "GetWorld(", "SetPhysicsLinearVelocity(", "PassCheckpoint(",
            "AwardCredits(", "SaveGame(", "UCLASS(", "AddForce(",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("ReverseGovernorMaximumFrameSeconds = 0.10", self.policy)
        self.assertIn("ReverseGovernorArcadeMaxDeceleration = 850.0", self.policy)
        self.assertIn("s.RearTractionScale > s.ContactTractionScale", self.policy)
        self.assertIn("s.OtherLongitudinalBrake", self.policy)
        self.assertIn("850 cm/s² Arcade", self.docs)
        self.assertIn("650 cm/s² Simulation", self.docs)
        self.assertIn("not a verified playable Win64 game", self.docs)
        self.assertIn("ReverseSpeedGovernorTests.cpp", self.cmake)
        self.assertIn("NAME ReverseSpeedGovernor", self.cmake)
        self.assertIn("latebrake_reverse_speed_governor_tests PRIVATE /W4 /WX",
                      self.cmake)
        self.assertIn("100000", self.tests)
        self.assertIn("Settle(30)-Settle(120)", self.tests)


if __name__ == "__main__":
    unittest.main()
