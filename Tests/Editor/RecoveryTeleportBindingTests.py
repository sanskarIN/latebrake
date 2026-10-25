"""Source-only Unreal bindings for verified paid RECOVER.

Does not execute Unreal Engine, UHT, SetActorLocationAndRotation or Chaos.
Actual installed-device teleport failure QA must be collected separately.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODE = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
PAWN_H = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Racing/RecoveryCharge.h"
VERIFICATION = ROOT / "Source/Latebrake/Private/Racing/RecoveryTeleportVerification.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


def body(source: str, signature: str, next_signature: str) -> str:
    if source.count(signature) != 1:
        raise AssertionError(f"missing or duplicate method: {signature}")
    return source.split(signature, 1)[1].split(next_signature, 1)[0]


class RecoveryTeleportBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.pawn_h = PAWN_H.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.verification = VERIFICATION.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_game_mode_stages_payment_around_verified_move(self):
        recovery = body(
            self.mode,
            "void ALatebrakeRaceGameMode::RecoverPlayer(",
            "void ALatebrakeRaceGameMode::LoadRecords(",
        )
        self.assertIn("TryChargePlayerRecoveryAfterVerifiedTeleport(", recovery)
        self.assertIn("Car->TryRecoverTo(Target, Facing)", recovery)
        self.assertIn("ManualPause.IsSuspended()", recovery)
        self.assertIn("AppFocusGuard.IsSuspended()", recovery)
        self.assertIn("Car->IsRacePhysicsReady()", recovery)
        self.assertNotIn("Car->RecoverTo(", recovery)
        self.assertNotIn("TryChargePlayerRecoveryIfReady(", recovery)

    def test_shared_transaction_commits_only_after_success(self):
        self.assertIn("RaceProgress pendingRace = race;", self.policy)
        self.assertIn("int pendingRecoveries = recoveries;", self.policy)
        self.assertIn("TryChargePlayerRecovery(pendingRace, pendingRecoveries)", self.policy)
        self.assertIn("std::forward<TryVerifiedTeleport>(tryVerifiedTeleport)()", self.policy)
        callback = self.policy.index("std::forward<TryVerifiedTeleport>(tryVerifiedTeleport)()")
        commit = self.policy.index("race = pendingRace;", callback)
        self.assertLess(callback, commit)
        self.assertLess(commit, self.policy.index("recoveries = pendingRecoveries;", callback))

    def test_unreal_reports_result_and_observed_pose(self):
        self.assertIn("bool TryRecoverTo(", self.pawn_h)
        self.assertIn("void RecoverTo(", self.pawn_h)
        attempt = body(
            self.pawn, "bool ALatebrakeCarPawn::TryRecoverTo(",
            "void ALatebrakeCarPawn::RecoverTo(",
        )
        self.assertIn("++TeleportRevision;", attempt)
        self.assertIn("SetActorLocationAndRotation(", attempt)
        self.assertIn("const bool bEngineMoved =", attempt)
        self.assertIn("GetActorLocation();", attempt)
        self.assertIn("GetActorRotation();", attempt)
        self.assertIn("Chassis->GetPhysicsLinearVelocity()", attempt)
        self.assertIn("Chassis->GetPhysicsAngularVelocityInDegrees()", attempt)
        self.assertIn("bChassisStillValid && IsRacePhysicsReady()", attempt)
        self.assertIn("Rotation.Pitch, Rotation.Roll", attempt)
        self.assertIn("ObservedRotation.Pitch, ObservedRotation.Roll", attempt)
        self.assertIn("PostLinearVelocity.X, PostLinearVelocity.Y, PostLinearVelocity.Z", attempt)
        self.assertIn("PostAngularVelocity.X, PostAngularVelocity.Y, PostAngularVelocity.Z", attempt)
        self.assertIn("latebrake::EvaluateRecoveryTeleport({", attempt)
        self.assertLess(attempt.index("++TeleportRevision;"),
                        attempt.index("SetActorLocationAndRotation("))
        self.assertLess(attempt.index("SetActorLocationAndRotation("),
                        attempt.index("EvaluateRecoveryTeleport({"))
        self.assertIn("return false;", attempt)
        self.assertIn("return true;", attempt)

    def test_legacy_blueprint_restart_and_ai_callers_retained(self):
        compatibility = body(
            self.pawn, "void ALatebrakeCarPawn::RecoverTo(",
            "void ALatebrakeCarPawn::RecoverPressed(",
        )
        self.assertIn("TryRecoverTo(Position, Rotation)", compatibility)
        self.assertIn("Car->RecoverTo(GetStartPosition()", self.mode)
        self.assertIn("RivalCar->RecoverTo(", self.mode)

    def test_observed_3d_pose_and_wrapped_yaw_are_checked(self):
        for flag in ("sample.EngineAccepted", "sample.ActualX",
                     "sample.ActualY", "sample.ActualZ",
                     "sample.ActualYawDegrees", "sample.TargetX",
                     "sample.TargetY", "sample.TargetZ",
                     "sample.TargetYawDegrees"):
            with self.subTest(flag=flag):
                self.assertIn(flag, self.verification)
        self.assertIn("std::hypot(std::hypot(dx, dy), dz)", self.verification)
        self.assertIn("std::remainder(", self.verification)
        self.assertIn("RecoveryTeleportDecision::PositionMismatch", self.verification)
        self.assertIn("RecoveryTeleportDecision::HeadingMismatch", self.verification)

    def test_post_move_readiness_and_motion_are_mandatory(self):
        for expected in (
            "sample.TargetPitchDegrees", "sample.TargetRollDegrees",
            "sample.ActualPitchDegrees", "sample.ActualRollDegrees",
            "sample.ChassisReadyAfterMove", "sample.PostLinearVelocityX",
            "sample.PostLinearVelocityY", "sample.PostLinearVelocityZ",
            "sample.PostAngularVelocityXDegrees",
            "sample.PostAngularVelocityYDegrees",
            "sample.PostAngularVelocityZDegrees",
            "RecoveryTeleportDecision::OrientationMismatch",
            "RecoveryTeleportDecision::PostMovePhysicsUnavailable",
            "RecoveryTeleportDecision::NonfinitePostMoveMotion",
            "RecoveryTeleportDecision::ResidualMotion",
        ):
            with self.subTest(expected=expected):
                self.assertIn(expected, self.verification)
        self.assertIn("MaxResidualSpeedCmPerSecond", self.verification)
        self.assertIn("MaxResidualAngularSpeedDegreesPerSecond", self.verification)
        self.assertIn("IsRotationWithinTolerance(", self.verification)

    def test_mutation_removed_chassis_snapshot_is_detected(self):
        attempt = body(
            self.pawn, "bool ALatebrakeCarPawn::TryRecoverTo(",
            "void ALatebrakeCarPawn::RecoverTo(",
        )
        removed = attempt.replace(
            "bChassisStillValid && IsRacePhysicsReady()",
            "true /* unsafe readiness bypass */",
        )
        self.assertNotIn("bChassisStillValid && IsRacePhysicsReady()", removed)
        self.assertIn("bChassisStillValid && IsRacePhysicsReady()", attempt)

    def test_production_cpp_is_compiled_by_standalone_suite(self):
        self.assertIn("RecoveryTeleportTransactionTests.cpp", self.cmake)
        self.assertIn("Private/Racing/RecoveryTeleportVerification.cpp", self.cmake)
        self.assertIn("Private/Racing/RaceProgress.cpp", self.cmake)
        self.assertIn("NAME RecoveryTeleportTransaction", self.cmake)

    def test_mutant_early_penalty_commit_trips_contract(self):
        damaged = self.policy.replace(
            "race = pendingRace;",
            "/* unsafe commit omitted */",
        )
        self.assertNotIn("race = pendingRace;", damaged)
        skipped = self.mode.replace(
            "Car->TryRecoverTo(Target, Facing)",
            "true /* hypothetical unconditional teleport acceptance */",
        )
        self.assertNotIn("Car->TryRecoverTo(Target, Facing)", skipped)


if __name__ == "__main__":
    unittest.main()
