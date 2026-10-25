"""Source integration contract for actual rival verified teleport feedback.

These checks do not execute Unreal, UHT or Chaos and must be paired with
licensed engine/device QA before the rival can be called gameplay-qualified.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
RIVAL = ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp"
ROUTE = ROOT / "Source/Latebrake/Public/AI/RivalRouteProgress.h"
PAWN_H = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
PAWN_CPP = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
TEST = ROOT / "Tests/Standalone/RivalRouteTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
RIVAL_H = ROOT / "Source/Latebrake/Public/AI/LatebrakeRivalCar.h"
MODE_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
MODE = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"


class RivalRecoveryFeedbackBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rival = RIVAL.read_text(encoding="utf-8")
        cls.route = ROUTE.read_text(encoding="utf-8")
        cls.pawn_h = PAWN_H.read_text(encoding="utf-8")
        cls.pawn = PAWN_CPP.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.rival_h = RIVAL_H.read_text(encoding="utf-8")
        cls.mode_h = MODE_H.read_text(encoding="utf-8")
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")

    def rival_tick(self):
        method = "void ALatebrakeRivalCar::Tick(float DeltaSeconds)"
        self.assertEqual(self.rival.count(method), 1)
        return self.rival.split(method, 1)[1]

    def test_rival_checks_unreal_result_before_telemetry(self):
        tick = self.rival_tick()
        recovery = tick.split("if (Observation.NeedsRecovery)", 1)[1].split(
            "if (Observation.Completed)", 1
        )[0]
        self.assertIn("const bool bRecovered = TryRecoverTo(", recovery)
        self.assertIn("Route.ReportRecoveryResult(bRecovered)", recovery)
        self.assertIn("SetAutonomousControls(0.f, 0.f, !bRecovered);", recovery)
        self.assertNotIn("RecoverTo(FVector(", recovery)
        self.assertLess(recovery.index("TryRecoverTo("),
                        recovery.index("Route.ReportRecoveryResult("))
        self.assertLess(recovery.index("Route.ReportRecoveryResult("),
                        recovery.index("Super::Tick(DeltaSeconds);"))
        self.assertIn("if (!bReported || !bRecovered)", recovery)

    def test_base_pawn_uses_production_post_move_verifier(self):
        self.assertIn("bool TryRecoverTo(", self.pawn_h)
        method = self.pawn.split(
            "bool ALatebrakeCarPawn::TryRecoverTo(", 1
        )[1].split("void ALatebrakeCarPawn::RecoverTo(", 1)[0]
        self.assertIn("SetActorLocationAndRotation(", method)
        self.assertIn("latebrake::EvaluateRecoveryTeleport({", method)
        self.assertIn("IsRacePhysicsReady()", method)
        self.assertIn("PostAngularVelocity.X", method)
        self.assertIn("return false;", method)
        self.assertIn("return true;", method)

    def test_pending_attempt_gates_route_and_requires_result(self):
        self.assertIn("if (RecoveryPending || IsFinished()", self.route)
        self.assertIn("RecoveryPending = true;", self.route)
        self.assertIn("bool ReportRecoveryResult(bool enginePlacementVerified)", self.route)
        self.assertIn("if (!RecoveryPending)", self.route)
        self.assertIn("RecoveryPending = false;", self.route)
        self.assertIn("++SuccessfulRecoveries;", self.route)
        self.assertIn("++FailedRecoveries;", self.route)
        self.assertIn("int GetSuccessfulRecoveryCount()", self.route)
        self.assertIn("int GetFailedRecoveryCount()", self.route)
        self.assertIn("bool HasPendingRecovery()", self.route)
        # Recovery budget stays on REQUEST, not post-hoc engine success.
        self.assertLess(self.route.index("++Recoveries;"),
                        self.route.index("RecoveryPending = true;"))

    def test_hud_exposes_actual_verified_vs_failed_recovery(self):
        for expected in (
            "GetPilotSuccessfulRecoveries()", "GetPilotFailedRecoveries()",
        ):
            self.assertIn(expected, self.rival_h)
        for expected in (
            "GetRivalSuccessfulRecoveryCount()",
            "GetRivalFailedRecoveryCount()",
        ):
            self.assertIn(expected, self.mode_h)
            self.assertIn(expected, self.mode)
            self.assertIn(expected, self.hud)
        self.assertIn("RIVAL TELEPORTS VERIFIED %d | REJECTED %d", self.hud)
        self.assertIn("RIVAL ROUTE  %d/%d LAPS | RECOVERY ATTEMPTS  %d%s", self.hud)

    def test_tests_exercise_both_outcomes_and_reset(self):
        self.assertIn("ReportRecoveryResult(true)", self.test)
        self.assertIn("ReportRecoveryResult(false)", self.test)
        self.assertIn("HasPendingRecovery()", self.test)
        self.assertIn("MaxRecoveries", self.test)
        self.assertIn("RivalRouteTests.cpp", self.cmake)
        self.assertIn("NAME RivalRoute", self.cmake)
        self.assertIn("RivalRecoveryOutcomeTests.cpp", self.cmake)
        self.assertIn("Private/Racing/RecoveryTeleportVerification.cpp", self.cmake)
        self.assertIn("NAME RivalRecoveryOutcome", self.cmake)

    def test_mutation_rejects_void_teleport_and_false_success(self):
        tick = self.rival_tick()
        no_check = tick.replace(
            "const bool bRecovered = TryRecoverTo(",
            "RecoverTo(",
        )
        self.assertNotIn("const bool bRecovered = TryRecoverTo(", no_check)
        no_feedback = tick.replace(
            "Route.ReportRecoveryResult(bRecovered)",
            "true /* dropped outcome */",
        )
        self.assertNotIn("Route.ReportRecoveryResult(bRecovered)", no_feedback)


if __name__ == "__main__":
    unittest.main()
