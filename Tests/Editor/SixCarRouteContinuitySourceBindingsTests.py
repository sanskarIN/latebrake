"""Source-only six-car route provenance contracts, not UE game QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "Source/Latebrake/Public/AI/SixCarRouteContinuityAudit.h"
TESTS = ROOT / "Tests/Standalone/SixCarRouteContinuityAuditTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
QA = ROOT / "docs/SIX_CAR_ROUTE_CONTINUITY_QA.md"


class SixCarRouteContinuitySourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.tests = TESTS.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.qa = QA.read_text(encoding="utf-8")

    def test_all_six_physical_identities_are_mandatory(self):
        for token in (
            "SixCarRouteAuditCapacity = 6",
            "frame.SessionIdentity",
            "a.ActorIdentity == 0",
            "seenSlots[slot]",
            "ordered[i].ActorIdentity == ordered[j].ActorIdentity",
            "a.QualifiedRoadContact",
            "ProjectWorldScaleRoute(",
            "projection.InsidePhysicalCandidateRoad",
            "SixCarRouteDistanceDifference(",
            "This is not an AI controller",
        ):
            self.assertIn(token, self.header)

    def test_continuity_and_lap_seam_are_independent_from_rank(self):
        for token in (
            "PreviousFrameOrdinal + 1",
            "before.VelocityX + after.VelocityX",
            "before.VelocityY + after.VelocityY",
            "after.WorldX - expectedX",
            "after.WorldY - expectedY",
            "currentDistance[i] - PreviousReportedDistance[i]",
            "actor.CompletedLaps",
            "WorldScaleCircuitLapCm",
            "SixCarRouteAuditPassHysteresisCm",
            "SixCarRouteAuditMaxAdvanceErrorCm",
            "0.12 * measuredTravelMagnitude",
            "const auto tangentBefore = WorldScaleCenterlineAt(",
            "const auto tangentAfter = WorldScaleCenterlineAt(",
            "const double signedProgressSpeed =",
            "PreviousSignedRouteSpeed[i] +",
            "currentSignedRouteSpeed[i]",
            "currentSignedRouteSpeed[newLeader] >= 50.0",
            "const double projectedTotal =",
            "projectedDistance[slot] = projectedTotal;",
            "projectedDistance[static_cast<std::size_t>(a)]",
            "const double gap = projectedDistance[i] - projectedDistance[j];",
            "worldResidual > maxWorldError",
            "ConfirmedRelativePasses",
            "RejectAndBreakContinuity();",
            "PreviousActors = ordered;",
        ):
            self.assertIn(token, self.header)
        for test_name in (
            "NormalSixCarMotion", "InvalidAndIncompleteFleet",
            "TeleportsAndLapFraud", "EpochIdentityAndFrameGaps",
            "FullLapBoundary", "PassHysteresis",
            "FrozenAndStalePhysics",
            "SignedRoadMotionCannotBeForged",
            "RepeatedReportedProgressCannotFabricatePass",
            "30.0, 60.0, 120.0",
        ):
            self.assertIn(test_name, self.tests)

    def test_portable_toolchain_not_live_unreal_authority(self):
        self.assertIn(
            "add_latebrake_portable_research_test(SixCarRouteContinuityAudit)",
            self.cmake,
        )
        self.assertIn("NOT performed", self.qa)
        for banned in (
            "UCLASS", "UFUNCTION", "UPROPERTY",
            "SetPhysicsLinearVelocity(", "AddForce(",
            "PassCheckpoint(", "SaveGame", "AwardCredits(",
            "ApplyThrottle(", "GetWorld(",
        ):
            self.assertNotIn(banned, self.header)
        self.assertIn("This is not an AI controller", self.header)
        self.assertIn("not integrated Unreal gameplay", self.qa)

if __name__ == "__main__":
    unittest.main()
