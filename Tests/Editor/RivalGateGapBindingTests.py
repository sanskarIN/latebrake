"""The rival advisory gate gap uses accepted route gate counters only.

Source contracts, not Unreal compilation, physical vehicle QA, official
race classification, AI performance qualification, or device gameplay.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class RivalGateGapBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Racing/RivalGateGap.h")
        cls.mode_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.rival_h = read("Source/Latebrake/Public/AI/LatebrakeRivalCar.h")
        cls.rival_cpp = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.tests = read("Tests/Standalone/RivalGateGapTests.cpp")
        cls.docs = read("docs/RIVAL_VERIFIED_GATE_GAP_QA.md")

    def test_rival_cursor_traces_actual_directional_crossings(self):
        # Route records remain a separate experimental rival, but the
        # physical caller uses only its accepted forward gate trace.
        self.assertIn("GetNextVerifiedGate()", self.rival_h)
        self.assertIn("Route.GetNextWaypoint()", self.rival_h)
        self.assertIn("Route.GetWaypointCount()", self.rival_h)
        self.assertIn("Route.ObserveVerifiedGate(", self.rival_cpp)
        self.assertIn("RivalGateTrace.Sample(", self.rival_cpp)
        self.assertLess(self.rival_cpp.index("RivalGateTrace.Sample("),
                        self.rival_cpp.index("Route.ObserveVerifiedGate("))

    def test_game_mode_rejects_interrupted_and_mismatched_races(self):
        self.assertIn('#include "Racing/RivalGateGap.h"', self.mode_h)
        self.assertIn("TryGetRivalCreditedGateGap(", self.mode_h)
        section = self.mode.split(
            "bool ALatebrakeRaceGameMode::TryGetRivalCreditedGateGap(", 1)[1].split(
            "FString ALatebrakeRaceGameMode::GetSelectedVehicleName()", 1)[0]
        for marker in (
            "OutGap = {};", "GetRaceEvent(SelectedEvent).SpawnRival",
            "!bCircuitReady", "!StartLights.IsGreen()",
            "Race.IsFinished()", "!IsRestartPlacementReady()",
            "!HasLocalPlayerDriver()", "Menu.IsOpen()",
            "UGameplayStatics::IsGamePaused(this)",
            "AppFocusGuard.IsSuspended()", "ManualPause.IsSuspended()",
            "!IsValid(RivalCar)", "!RivalCar->IsRacePhysicsReady()",
            "GetExpectedLaps() != Race.GetRequiredLaps()",
            "GetVerifiedGateCount() != Race.GetCheckpointCount()",
            "Race.GetCompletedLaps()", "Race.GetNextCheckpoint()",
            "RivalCar->GetApproximateLaps()", "RivalCar->GetNextVerifiedGate()",
            "CompareRivalCreditedGates({", "return OutGap.Valid;",
        ):
            self.assertIn(marker, section)
        for disallowed in ("AwardCredits(", "SaveRecords(", "PassCheckpoint(",
                           "SetActorLocation(", "Chassis->AddForce("):
            self.assertNotIn(disallowed, section)

    def test_expanded_hud_not_race_position_or_compact_touch_overlay(self):
        section = self.hud.split(
            "// Read-only discrete comparison of already accepted directional gate",
            1)[1].split("if (RivalProximityHint.Update(", 1)[0]
        for required in (
            "!bCompact", "VerticalPlan.MaxTelemetryLines >= 28",
            "bRivalPositionVerified", "TryGetRivalCreditedGateGap(GateGap)",
            "PlayerCreditedGates > 0", "RivalCreditedGates > 0",
            "RivalGateGapKind::PlayerMoreCreditedGates",
            "RivalGateGapKind::RivalMoreCreditedGates",
            "RivalGateGapKind::SameCreditedGateCount",
            "NOT RACE PLACE", "SAME VERIFIED COUNT",
        ):
            self.assertIn(required, section)
        self.assertNotIn("DrawRect(", section)
        self.assertNotIn("SetActor", section)
        self.assertNotIn("PassCheckpoint(", section)
        self.assertNotIn("AwardCredits(", section)

    def test_policy_has_no_untrusted_geometry_or_reward_authority(self):
        for marker in (
            "std::int64_t", "MaxGates = 256", "MaxLaps = 100",
            "PlayerPhysicsReady", "RivalPhysicsReady",
            "SameCreditedGateCount", "CreditedGateDelta",
            "sample.PlayerCompletedLaps == sample.RequiredLaps",
            "sample.RivalCompletedLaps == sample.RequiredLaps",
        ):
            self.assertIn(marker, self.policy)
        for forbidden in (
            "UCLASS(", "FVector", "Chaos", "SaveGame", "AddForce(",
            "PassCheckpoint(", "AwardCredits(", "SetActorLocation",
            "std::hypot", "GetActorLocation", "GetVelocity", "Rank",
        ):
            self.assertNotIn(forbidden, self.policy)
        self.assertIn("RivalGateGapTests.cpp", self.cmake)
        self.assertIn("NAME RivalGateGap", self.cmake)
        self.assertIn("latebrake_rival_gate_gap_tests PRIVATE /W4 /WX",
                      self.cmake)
        self.assertIn("200000", self.tests)
        self.assertIn("RivalRouteProgress actualRivalRoute(4, 3)", self.tests)
        self.assertIn("actualRivalRoute.ObserveVerifiedGate(", self.tests)
        self.assertIn("mere rival radius arrival cannot gain", self.tests)
        self.assertIn("Unreal Engine 5.8", self.docs)
        self.assertIn("not a playable", self.docs.lower())


if __name__ == "__main__":
    unittest.main()
