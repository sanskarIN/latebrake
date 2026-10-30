"""Observed stopping-distance bindings; static source QA, not installed Unreal."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(name: str) -> str:
    return (ROOT / name).read_text(encoding="utf-8")


class ObservedBrakeDistanceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Racing/ObservedBrakeDistance.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.tests = read("Tests/Standalone/ObservedBrakeDistanceTests.cpp")

    def test_real_validated_chassis_velocity_is_observed(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        for key in (
            "if (!IsRacePhysicsReady())",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity()",
            "const float Speed = FlatVelocity.Size()",
            "const float LongitudinalSpeed = FVector::DotProduct(FlatVelocity, Forward)",
            "if (!FMath::IsFinite(Speed)",
            "BrakeDistance.Observe({",
            "static_cast<double>(Speed)",
            "static_cast<double>(LongitudinalSpeed), bEffectiveBraking",
        ):
            self.assertIn(key, tick)
        self.assertLess(tick.index("!IsRacePhysicsReady()"),
                        tick.index("BrakeDistance.Observe({"))
        self.assertLess(tick.index("!FMath::IsFinite(Speed)"),
                        tick.index("BrakeDistance.Observe({"))
        self.assertLess(tick.index("BrakeDistance.Observe({"),
                        tick.index("Chassis->AddForce("))
        self.assertIn('#include "Racing/ObservedBrakeDistance.h"', self.pawn_h)
        self.assertIn("latebrake::ObservedBrakeDistance BrakeDistance;", self.pawn_h)
        self.assertIn("GetObservedBrakeDistance() const", self.pawn_h)

    def test_reset_and_focus_loss_never_keep_stale_braking_estimate(self):
        self.assertIn("BrakeDistance.Reset();", self.pawn_h)
        self.assertIn("BrakeDistance.Suspend();", self.pawn)
        invalid = self.pawn.split("if (!Chassis->IsSimulatingPhysics()", 1)[1].split(
            "const bool bArcade", 1)[0]
        self.assertIn("BrakeDistance.Suspend();", invalid)
        blocked = self.pawn.split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1
        )[1].split("void ALatebrakeCarPawn::ThrottleInput(", 1)[0]
        self.assertIn("BrakeDistance.Suspend();", blocked)
        clear = self.pawn.split(
            "void ALatebrakeCarPawn::ClearDriverInputs()", 1
        )[1].split("void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        self.assertIn("BrakeDistance.Suspend();", clear)
        recover = self.pawn.split(
            "bool ALatebrakeCarPawn::TryRecoverTo(", 1
        )[1].split("void ALatebrakeCarPawn::RecoverTo(", 1)[0]
        self.assertIn("BrakeDistance.Suspend();", recover)

    def test_estimate_only_on_noncompact_hud_after_authoritative_race_clock(self):
        live = self.hud.split("const auto Line = [&]", 1)[1]
        self.assertIn("Car->GetObservedBrakeDistance().HasLiveEstimate()", live)
        self.assertIn("BRAKE DIST EST %.0f m (CURRENT RATE)", live)
        self.assertIn("BRAKE DIST EST %.0f ft (CURRENT RATE)", live)
        self.assertIn("!bCompact", live)
        self.assertLess(live.index('TEXT("TIME  %02d:%05.2f")'),
                        live.index('TEXT("BRAKE DIST EST %.0f m'))
        self.assertIn("Mode->UsesMilesPerHour()", live)

    def test_math_policy_rejects_bad_data_and_keeps_authority_read_only(self):
        for required in (
            "std::isfinite(sample.DeltaSeconds)",
            "std::isfinite(speed)",
            "std::isfinite(forward)",
            "sample.Braking", "ConsecutivePairs >= 3",
            "MinimumConfirmedSeconds = 0.12",
            "MinimumForwardFraction = 0.88",
            "MaxFrameSeconds = 0.25",
            "MaxAdvisoryDistanceMeters = 2000.0",
            "PreviousPlanarSpeed - speed",
            "LiveEstimate =",
        ):
            self.assertIn(required, self.policy)
        for forbidden in (
            "AddForce(", "AddTorque(", "SaveGame", "AddTimePenalty(",
            "AwardCredits", "PassCheckpoint(", "UCLASS(", "UE_LOG(",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("NAME ObservedBrakeDistance", self.cmake)
        self.assertIn("ObservedBrakeDistanceTests.cpp", self.cmake)
        self.assertIn("FrameStableDamping", self.cmake)
        self.assertIn("5000", self.tests)
        self.assertIn("cxx_std_20", self.cmake)


if __name__ == "__main__":
    unittest.main()
