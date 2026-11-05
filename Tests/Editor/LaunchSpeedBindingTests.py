"""Source binding checks for the optional 0–5 km/h launch estimate.

These checks DO NOT compile Unreal or certify rendering, real physics,
controller latency, cooked tracks, packaged builds or physical devices.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class LaunchSpeedBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def read(path):
            return (ROOT / path).read_text(encoding="utf-8")
        cls.mode_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy = read("Source/Latebrake/Public/Racing/LaunchSpeedMeter.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.road_speed = read("Source/Latebrake/Public/Vehicles/MeasuredPlanarForwardSpeed.h")
        cls.road_speed_tests = read("Tests/Standalone/MeasuredPlanarForwardSpeedTests.cpp")

    def test_only_authoritative_clock_and_forward_chassis_speed(self):
        tick = self.mode.split("void ALatebrakeRaceGameMode::Tick(", 1)[1].split(
            "AStaticMeshActor* ALatebrakeRaceGameMode::SpawnBlock", 1)[0]
        for token in (
            "StartLights.Advance(", "AdvancePlayerRaceClockIfReady(",
            "LaunchMeter.Observe(", "Race.GetElapsedSeconds() - Race.GetTotalPenaltySeconds()",
            "Car->GetForwardSpeedKmH()", "bStartQualified",
        ):
            self.assertIn(token, tick)
        self.assertLess(tick.index("AdvancePlayerRaceClockIfReady("),
                        tick.index("LaunchMeter.Observe("))
        self.assertIn("float GetForwardSpeedKmH() const;", self.pawn_h)
        self.assertIn("GetPhysicsLinearVelocity()", self.pawn)
        self.assertIn("FMath::IsFinite", self.pawn)
        # The 3D chassis may pitch while its vertical springs settle.
        # Vertical falling must not become a fictional forward launch.
        speed = self.pawn.split(
            "float ALatebrakeCarPawn::GetForwardSpeedKmH() const", 1
        )[1].split("bool ALatebrakeCarPawn::TryRecoverTo(", 1)[0]
        self.assertIn("latebrake::MeasuredPlanarForwardSpeedKmh(", speed)
        for required in ("Velocity.X", "Velocity.Y", "Forward.X", "Forward.Y"):
            self.assertIn(required, speed)
        for forbidden in ("Velocity.Z", "Forward.Z", "FVector::DotProduct"):
            self.assertNotIn(forbidden, speed)
        self.assertIn('#include "Vehicles/MeasuredPlanarForwardSpeed.h"',
                      self.pawn)

    def test_road_speed_policy_is_bounded_observational_and_portable(self):
        for marker in ("std::hypot(", "std::isfinite(", 
                       "MinimumProjectedForwardLength = 1.0e-4",
                       "RoadCmPerSecondToKmH = 0.036",
                       "std::numeric_limits<double>::quiet_NaN()"):
            self.assertIn(marker, self.road_speed)
        for forbidden in ("GetWorld(", "AddForce(", "AwardCredits(", 
                          "PassCheckpoint(", "SaveGame(", "UCLASS("):
            self.assertNotIn(forbidden, self.road_speed)
        self.assertIn("120000", self.road_speed_tests)
        self.assertIn("MeasuredPlanarForwardSpeedTests.cpp", self.cmake)
        self.assertIn("NAME MeasuredPlanarForwardSpeed", self.cmake)

    def test_reset_and_verified_recovery_erase_launch_evidence(self):
        self.assertIn("latebrake::LaunchSpeedMeter LaunchMeter;", self.mode_h)
        self.assertIn("GetLaunchSpeedMeter() const", self.mode_h)
        begin = self.mode.split("void ALatebrakeRaceGameMode::BeginPlay()", 1)[1].split(
            "void ALatebrakeRaceGameMode::Tick(", 1)[0]
        restart = self.mode.split("void ALatebrakeRaceGameMode::RestartRace()", 1)[1].split(
            "void ALatebrakeRaceGameMode::RecoverPlayer(", 1)[0]
        recover = self.mode.split("void ALatebrakeRaceGameMode::RecoverPlayer(", 1)[1].split(
            "void ALatebrakeRaceGameMode::LoadRecords(", 1)[0]
        for part in (begin, restart):
            self.assertIn("LaunchMeter.Reset();", part)
        self.assertLess(recover.index("TryChargePlayerRecoveryAfterVerifiedTeleport("),
                        recover.index("LaunchMeter.Invalidate();"))
        self.assertIn("return;", recover[:recover.index("LaunchMeter.Invalidate();")])

    def test_interpolation_is_bounded_and_not_gameplay_authority(self):
        for token in (
            "MaxInterpolationGapSeconds = 0.25",
            "ContinuousSamples = false;",
            "activeSeconds <= LastSampleSeconds",
            "std::isfinite(speedChange)",
            "LastSampleSeconds + gap * fraction",
        ):
            self.assertIn(token, self.policy)
        self.assertNotIn("AddTimePenalty(", self.policy)
        self.assertNotIn("SetTime", self.policy)

    def test_hud_is_advisory_only(self):
        for token in (
            "GetLaunchSpeedMeter()", "HasSample()",
            "SecondsToFiveKmh()", "LAUNCH 0-5 km/h",
        ):
            self.assertIn(token, self.hud)
        for forbidden in ("RaceRewards", "CareerProgress", "SaveGame",
                          "PassCheckpoint", "AddTimePenalty", "UCLASS("):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("LaunchSpeedMeterTests.cpp", self.cmake)
        self.assertIn("NAME LaunchSpeedMeter", self.cmake)


if __name__ == "__main__":
    unittest.main()
