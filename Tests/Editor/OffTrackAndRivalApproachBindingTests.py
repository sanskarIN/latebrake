"""Source-level bindings for off-track HUD and inbound AI corner geometry.

These do not compile UE5, exercise Chaos physics, or validate installed UI.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD_H = ROOT / "Source/Latebrake/Public/UI/LatebrakeRaceHUD.h"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
ADVISOR = ROOT / "Source/Latebrake/Public/Track/OffTrackAdvisor.h"
RIVAL = ROOT / "Source/Latebrake/Private/AI/RivalCornerAnticipation.cpp"
RIVAL_ACTOR = ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
TEST = ROOT / "Tests/Standalone/OffTrackAdvisorTests.cpp"
RIVAL_TEST = ROOT / "Tests/Standalone/RivalCornerAnticipationTests.cpp"


class OffTrackAndRivalApproachBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HUD_H.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.advisor = ADVISOR.read_text(encoding="utf-8")
        cls.rival = RIVAL.read_text(encoding="utf-8")
        cls.actor = RIVAL_ACTOR.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")
        cls.rival_test = RIVAL_TEST.read_text(encoding="utf-8")

    def test_real_hud_instantiates_off_track_policy(self):
        self.assertIn('#include "Track/OffTrackAdvisor.h"', self.header)
        self.assertIn("latebrake::OffTrackIndicator OffTrackHint;", self.header)
        for marker in (
            "OffTrackHint.Update(", "AdviseReturnToCircuit(PlanarPosition)",
            "ToRoad.Valid && ToRoad.OffPavement", "bWayfindingEligible",
            "NEAREST PAVED ROAD", "OFF TRACK | ROAD", "ToRoad.UnitDirection.X",
            "ToRoad.UnitDirection.Y",
        ):
            self.assertIn(marker, self.hud)
        self.assertLess(
            self.hud.index("OffTrackHint.Update("),
            self.hud.index("AdviseReturnToCircuit(PlanarPosition)"))
        self.assertNotIn("PassCheckpoint(", self.advisor)
        self.assertNotIn("AddTimePenalty(", self.advisor)
        self.assertNotIn("SetActorLocation(", self.advisor)

    def test_discontinuity_and_menu_reset_both_warning_states(self):
        for marker in (
            "LastWayfindingPawn.Get() != Car",
            "LastWayfindingSession != CurrentWayfindingSession",
            "LastWayfindingTeleport != CurrentWayfindingTeleport",
            "Mode->IsManualRacePauseActive()",
            "Mode->IsFocusLossGuardActive()",
        ):
            self.assertIn(marker, self.hud)
        self.assertGreaterEqual(self.hud.count("OffTrackHint.Reset();"), 3)
        self.assertGreaterEqual(self.hud.count("WrongWayHint.Reset();"), 3)
        self.assertIn("if (!raceEligible", self.advisor)
        self.assertIn("deltaSeconds > 0.25", self.advisor)
        self.assertIn("RoadHalfWidth", self.advisor)
        self.assertIn("std::isfinite(position.X)", self.advisor)

    def test_rival_corner_actuator_only_after_inbound_corridor(self):
        self.assertIn("latebrake::AnticipateRivalCorner(", self.actor)
        for required in (
            "IsOnPavedSurface(carPosition)",
            "const double remaining =",
            "const double crossTrack =",
            "MaxCrossTrackCm",
            "remaining < 440.0",
            "remaining > 2200.0",
            "crossTrack > MaxCrossTrackCm",
            "std::isfinite(remaining)",
            "std::isfinite(crossTrack)",
        ):
            self.assertIn(required, self.rival)
        self.assertIn("0.55 * remaining", self.rival)
        self.assertNotIn("0.55 * distance", self.rival)
        self.assertIn("commanded.Brake = true;", self.rival)
        self.assertIn("commanded.Throttle = 0.0;", self.rival)
        self.assertNotIn("Route.ReportRecoveryResult", self.rival)

    def test_portable_production_policy_really_compiles(self):
        for required in ("OffTrackAdvisorTests.cpp", "NAME OffTrackAdvisor",
                         "target_compile_features(latebrake_off_track_advisor_tests",
                         "-Wpedantic -Werror"):
            self.assertIn(required, self.cmake)
        for required in ("OffTrackIndicator warning", "AdviseReturnToCircuit",
                         "quiet_NaN()", "GetOffPavementSeconds()"):
            self.assertIn(required, self.test)
        for required in ("beyondGate", "wrongLane", "nearCenter",
                         "outsidePavement"):
            self.assertIn(required, self.rival_test)


if __name__ == "__main__":
    unittest.main()
