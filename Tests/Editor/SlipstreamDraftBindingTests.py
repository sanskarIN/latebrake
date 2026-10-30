"""Source integration checks for physical-rival slipstream; NOT Unreal runtime."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class SlipstreamDraftBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Racing/SlipstreamDraftBoost.h")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.mode_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.guide = read("docs/SLIPSTREAM_DRAFT_QA.md")

    def test_real_rival_source_requires_valid_physics(self):
        block = self.mode.split("bool ALatebrakeRaceGameMode::TryGetRivalDraftingState(", 1)[1].split(
            "int32 ALatebrakeRaceGameMode::GetRivalApproximateLaps()", 1)[0]
        for required in ("IsValid(RivalCar)", "RivalCar->IsRacePhysicsReady()",
                         "RivalCar->HasCompletedRoute()", "Race.IsFinished()",
                         "RivalCar->GetActorLocation()", "RivalCar->GetActorForwardVector()",
                         "RivalCar->GetVelocity()", "FMath::IsFinite("):
            self.assertIn(required, block)
        self.assertIn("TryGetRivalDraftingState(FVector& OutPosition,", self.mode_h)

    def test_human_only_after_real_physics_and_race_authority(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        for required in (
            "if (!IsRacePhysicsReady())",
            "const FVector Velocity = Chassis->GetPhysicsLinearVelocity()",
            "const float LongitudinalSpeed = FVector::DotProduct(FlatVelocity, Forward)",
            "if (!FMath::IsFinite(Speed)",
            "IsLocallyControlled()",
            "Cast<APlayerController>(GetController()) != nullptr",
            "Mode->HasLocalPlayerDriver()",
            "Mode->TryGetRivalDraftingState(",
            "FVector::DotProduct(RivalVelocity, RivalForward)",
            "SlipstreamBoost.Observe({",
            "static_cast<double>(EffectiveThrottle)",
            "bEffectiveBraking, true",
            "SlipstreamBoost.Suspend();",
        ):
            self.assertIn(required, tick)
        self.assertLess(tick.index("if (!IsRacePhysicsReady())"),
                        tick.index("SlipstreamBoost.Observe({"))
        self.assertLess(tick.index("if (!FMath::IsFinite(Speed)"),
                        tick.index("SlipstreamBoost.Observe({"))

    def test_base_motor_and_brakes_remain_authoritative(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertIn("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *", tick)
        self.assertIn("if (DraftExtraForce > 0.0)", tick)
        self.assertIn("static_cast<float>(DraftExtraForce)", tick)
        self.assertLess(tick.index("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *"),
                        tick.index("if (DraftExtraForce > 0.0)"))
        self.assertIn("FrameStableDampingRate(GripRate, DeltaSeconds)", tick)
        self.assertEqual(tick.count("static_cast<float>(Surface.MotorMultiplier)"), 3)

    def test_interruptions_reset_session_and_expanded_hud_only(self):
        self.assertIn('#include "Racing/SlipstreamDraftBoost.h"', self.pawn_h)
        self.assertIn("latebrake::SlipstreamDraftBoost SlipstreamBoost;", self.pawn_h)
        self.assertIn("SlipstreamBoost.Reset();", self.pawn_h)
        self.assertIn("GetSlipstreamBoost() const", self.pawn_h)
        clear = self.pawn.split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        self.assertIn("SlipstreamBoost.Suspend();", clear)
        self.assertIn("SLIPSTREAM +%.1f%% DRIVE", self.hud)
        self.assertIn("!bCompact && Car->GetSlipstreamBoost().IsActive()", self.hud)

    def test_production_policy_and_verification_avoid_authority_mutation(self):
        for forbidden in ("RaceProgress", "SaveGame", "AwardCredits(",
                          "PassCheckpoint(", "UCLASS(", "AddForce(", "UE_LOG("):
            self.assertNotIn(forbidden, self.policy)
        for required in ("MaxForceExtraFraction = 0.06",
                         "MinimumHeadingAlignment = 0.94",
                         "ConfirmSeconds = 0.18", "std::isfinite(", "std::clamp("):
            self.assertIn(required, self.policy)
        self.assertIn("NAME SlipstreamDraftBoost", self.cmake)
        self.assertIn("SlipstreamDraftBoostTests.cpp", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)
        self.assertIn("Unreal Engine 5.8", self.guide)
        self.assertIn("source-only", self.guide.lower())


if __name__ == "__main__":
    unittest.main()
