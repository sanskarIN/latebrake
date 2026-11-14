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
        self.assertIn("TryGetRivalDraftingState(const ALatebrakeCarPawn* Observer,", self.mode_h)

    def test_draft_requires_both_actual_road_supported_chassis(self):
        traction = read(
            "Source/Latebrake/Public/Vehicles/ChassisGroundContactTraction.h")
        self.assertIn("HasQualifiedDraftGroundSupport(", traction)
        self.assertIn("contact.SupportingProbes == 3", traction)
        self.assertIn("contact.SupportingProbes == 4", traction)
        self.assertIn("contact.HasDriveContact", traction)
        self.assertIn("HasQualifiedDraftRoadSupport() const noexcept",
                      self.pawn_h)
        self.assertIn("HasQualifiedDraftGroundSupport(LastGroundContact)",
                      self.pawn_h)
        state = self.mode.split(
            "bool ALatebrakeRaceGameMode::TryGetRivalDraftingState(", 1
        )[1].split(
            "int32 ALatebrakeRaceGameMode::GetRivalApproximateLaps()", 1
        )[0]
        self.assertIn("RivalCar->HasQualifiedDraftRoadSupport()", state)
        self.assertLess(
            state.index("RivalCar->HasQualifiedDraftRoadSupport()"),
            state.index("OutPosition = Position;"))
        tick = self.pawn.split(
            "void ALatebrakeCarPawn::Tick(", 1
        )[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1
        )[0]
        self.assertIn(
            "latebrake::HasQualifiedDraftGroundSupport(GroundContact)", tick)
        self.assertLess(
            tick.index("HasQualifiedDraftGroundSupport(GroundContact)"),
            tick.index("SlipstreamBoost.Observe({"))
        self.assertIn("SlipstreamBoost.Suspend();", tick)
        self.assertIn("ChassisGroundContactTractionTests.cpp", self.cmake)

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
            "this, RivalPosition, RivalForward, RivalVelocity",
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

    def test_both_real_cars_have_measured_lateral_slip_qualification(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertIn("const float LateralSpeed =", tick)
        self.assertIn("FVector::DotProduct(FlatVelocity, Right)", tick)
        draft = tick.split("const bool bDraftTargetReady =", 1)[1].split(
            "SlipstreamBoost.Suspend();", 1
        )[0]
        for required in (
            "FVector::DotProduct(RivalVelocity, RivalForward)",
            "const double RivalLateralSpeed = FVector::DotProduct(",
            "FVector(-RivalForward.Y, RivalForward.X, 0.f)",
            "static_cast<double>(LateralSpeed)",
            "RivalLateralSpeed",
        ):
            self.assertIn(required, draft)
        self.assertLess(
            draft.index("const double RivalLateralSpeed ="),
            draft.index("SlipstreamBoost.Observe({"))
        self.assertLess(
            draft.index("static_cast<double>(LateralSpeed)"),
            draft.index("RivalLateralSpeed,\n            true"))
        for required in (
            "FollowerLateralSpeedCmPerSecond",
            "LeadLateralSpeedCmPerSecond",
            "MaximumLateralSlipFraction = 0.25",
            "MaximumLateralSpeedCmPerSecond = 500.0",
            "!std::isfinite(followerLateral)",
            "!std::isfinite(leadLateral)",
            "std::abs(followerLateral)",
            "std::abs(leadLateral)",
            "Reset();",
        ):
            self.assertIn(required, self.policy)


    def test_live_rival_identity_cannot_transfer_existing_draft_credit(self):
        state = self.mode.split(
            "bool ALatebrakeRaceGameMode::TryGetRivalDraftingState(", 1
        )[1].split("int32 ALatebrakeRaceGameMode::GetRivalApproximateLaps()", 1)[0]
        self.assertIn("uint32& OutIdentity) const", state)
        self.assertIn("RivalCar->GetUniqueID()", state)
        self.assertIn("if (RivalIdentity == 0)", state)
        self.assertIn("OutIdentity = RivalIdentity;", state)
        self.assertIn("uint32& OutIdentity) const;", self.mode_h)
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertIn("uint32 RivalIdentity = 0;", tick)
        self.assertIn("this, RivalPosition, RivalForward, RivalVelocity,",
                      tick)
        self.assertIn("static_cast<std::uint64_t>(RivalIdentity)", tick)
        for field in ("X", "Y", "Z"):
            self.assertIn(f"static_cast<double>(Position.{field})", tick)
        self.assertIn("PreviousFollowerWorldX = followerWorldX;", self.policy)
        self.assertIn("PreviousFollowerWorldY = followerWorldY;", self.policy)
        self.assertIn("PreviousFollowerWorldZ = followerWorldZ;", self.policy)
        self.assertIn("followerResidual > MaxRelativeMotionResidualCm",
                      self.policy)
        self.assertIn("std::abs(followerVerticalResidual)", self.policy)
        for field in ("FollowerWorldX", "FollowerWorldY", "FollowerWorldZ"):
            self.assertIn(f"double {field} =", self.policy)
        self.assertIn("std::uint64_t LeadIdentity = 0;", self.policy)
        self.assertIn("sample.LeadIdentity == 0", self.policy)
        self.assertIn("PreviousLeadIdentity != sample.LeadIdentity",
                      self.policy)
        self.assertIn("PreviousLeadIdentity = sample.LeadIdentity",
                      self.policy)
        self.assertIn("PreviousLeadIdentity = 0;", self.policy)

    def test_live_vertical_physics_is_bound_into_three_dimensional_continuity(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        draft = tick.split("const bool bDraftTargetReady =", 1)[1].split(
            "SlipstreamBoost.Suspend();", 1
        )[0]
        self.assertIn("static_cast<double>(Velocity.Z)", draft)
        self.assertIn("static_cast<double>(RivalVelocity.Z)", draft)
        self.assertLess(draft.index("RivalLateralSpeed,"),
                        draft.index("static_cast<double>(Velocity.Z)"))
        self.assertIn("FollowerVerticalSpeedCmPerSecond", self.policy)
        self.assertIn("LeadVerticalSpeedCmPerSecond", self.policy)
        self.assertIn("!std::isfinite(followerVertical)", self.policy)
        self.assertIn("!std::isfinite(leadVertical)", self.policy)
        self.assertIn("const double predictedZ = PreviousOffsetZ +", self.policy)
        self.assertIn("dz - predictedZ", self.policy)
        self.assertIn("MaxVerticalMotionResidualCm = 75.0", self.policy)
        self.assertIn("std::abs(verticalResidual) > MaxVerticalMotionResidualCm",
                      self.policy)
        self.assertIn("PreviousOffsetZ = dz;", self.policy)
        self.assertIn("std::numeric_limits<double>::quiet_NaN()", self.policy)

    def test_real_world_obstruction_is_required_before_draft(self):
        # A usable position alone is NOT aerodynamic line of sight.
        # An actual Unreal collision trace must be part of the existing
        # GameMode sample on every physical human/rival eligibility frame.
        block = self.mode.split(
            "bool ALatebrakeRaceGameMode::TryGetRivalDraftingState(", 1
        )[1].split(
            "int32 ALatebrakeRaceGameMode::GetRivalApproximateLaps()", 1
        )[0]
        for required in (
            "const ALatebrakeCarPawn* Observer",
            "IsValid(Observer)",
            "Observer == RivalCar.Get()",
            "Observer->GetWorld() != World",
            "GetRaceEvent(SelectedEvent).SpawnRival",
            "Observer->GetActorLocation()",
            "RivalCar->GetActorLocation()",
            "FCollisionQueryParams TraceParams(",
            "TraceParams.AddIgnoredActor(Observer)",
            "TraceParams.AddIgnoredActor(RivalCar.Get())",
            "World->LineTraceSingleByChannel(",
            "ECC_Visibility, TraceParams",
            "MaximumDraftTraceDistanceCm = 5000.0",
        ):
            self.assertIn(required, block)
        self.assertLess(
            block.index("World->LineTraceSingleByChannel("),
            block.index("OutPosition = Position;"))
        self.assertIn("#include \"CollisionQueryParams.h\"", self.mode)
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertIn("this, RivalPosition, RivalForward, RivalVelocity", tick)
        self.assertIn("RivalLateralSpeed,\n            true", tick)
        self.assertIn("bool ClearPhysicalDraftPath = false;", self.policy)
        self.assertIn("!sample.ClearPhysicalDraftPath", self.policy)
        self.assertIn("s.ClearPhysicalDraftPath = true;", read(
            "Tests/Standalone/SlipstreamDraftBoostTests.cpp"))
        self.assertIn("solid barrier", self.guide)

    def test_base_motor_and_brakes_remain_authoritative(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        self.assertIn("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *", tick)
        self.assertIn("if (DraftExtraForce > 0.0)", tick)
        self.assertIn("static_cast<float>(DraftExtraForce)", tick)
        self.assertLess(tick.index("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *"),
                        tick.index("if (DraftExtraForce > 0.0)"))
        self.assertIn("latebrake::CalculateLateralGripForce({", tick)
        self.assertIn("LateralForce.RightForce", tick)
        self.assertIn("latebrake::CalculateGripLimitedBraking({", tick)
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
