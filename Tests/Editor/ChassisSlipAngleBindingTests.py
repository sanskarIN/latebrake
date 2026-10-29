"""Unreal source integration checks, not a substitute for UE/UHT or device QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class ChassisSlipAngleBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Racing/ChassisSlipAngleMeter.h")
        cls.pawn_header = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_observation_is_bound_to_real_chaos_velocity(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        for marker in (
            "if (!Mode || !Mode->IsRaceStartGreen()",
            "if (!IsRacePhysicsReady())",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity()",
            "const float LongitudinalSpeed = FVector::DotProduct(FlatVelocity, Forward)",
            "const float LateralSpeed = FVector::DotProduct(FlatVelocity, Right)",
            "SlipAngleMeter.Observe({",
            "static_cast<double>(LongitudinalSpeed)",
            "static_cast<double>(LateralSpeed)",
        ):
            self.assertIn(marker, tick)
        self.assertLess(tick.index("!IsRacePhysicsReady()"),
                        tick.index("SlipAngleMeter.Observe({"))
        self.assertLess(tick.index("!FMath::IsFinite(LateralSpeed)"),
                        tick.index("SlipAngleMeter.Observe({"))
        self.assertIn("latebrake::ChassisSlipAngleMeter SlipAngleMeter;",
                      self.pawn_header)
        self.assertIn("GetChassisSlipAngleMeter() const", self.pawn_header)

    def test_pause_recovery_restart_are_authoritative(self):
        blocked = self.pawn.split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1
        )[1].split("void ALatebrakeCarPawn::ThrottleInput(", 1)[0]
        recover = self.pawn.split("bool ALatebrakeCarPawn::TryRecoverTo(", 1
        )[1].split("void ALatebrakeCarPawn::RecoverTo(", 1)[0]
        self.assertIn("SlipAngleMeter.Suspend();", blocked)
        self.assertIn("SlipAngleMeter.Suspend();", recover)
        self.assertIn("SlipAngleMeter.Reset();", self.pawn_header)
        self.assertIn("SlipAngleMeter.Suspend();", self.pawn.split(
            "if (!Chassis->IsSimulatingPhysics()", 1
        )[1].split("const bool bArcade =", 1)[0])

    def test_hud_is_advisory_and_portable_ci_compiles_policy(self):
        self.assertIn("GetChassisSlipAngleMeter()", self.hud)
        self.assertIn("CHASSIS SLIP (EST)", self.hud)
        self.assertIn("PEAK CHASSIS SLIP (EST)", self.hud)
        self.assertIn("if (!bCompact)", self.hud)
        self.assertIn("VerticalPlan.MaxTelemetryLines >= 28", self.hud)
        self.assertIn("latebrake::CanAwardFinishedRace(Race)", self.hud)
        for forbidden in (
            "AddForce", "AddTorque", "SaveGame", "UCLASS(", "PassCheckpoint",
            "EarnCredits", "AddTimePenalty", "SetPhysicsLinearVelocity",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("ChassisSlipAngleMeterTests.cpp", self.cmake)
        self.assertIn("NAME ChassisSlipAngleMeter", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)

    def test_policy_rejects_bad_frames_and_reverse(self):
        for marker in (
            "std::isfinite(sample.ForwardCmPerSecond)",
            "std::isfinite(sample.LateralCmPerSecond)",
            "sample.ForwardCmPerSecond < MinForwardCmPerSecond",
            "std::atan2(sample.LateralCmPerSecond",
            "std::hypot(sample.ForwardCmPerSecond",
            "HighAngleThresholdDegrees = 12.0",
            "MaxSampleSeconds = 0.25",
        ):
            self.assertIn(marker, self.policy)


if __name__ == "__main__":
    unittest.main()
