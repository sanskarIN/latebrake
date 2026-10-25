"""v0.5.63: rival warning binding and HUD results-menu C++ scope guard.

These are source contracts, not Unreal Header Tool or an installed game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HUD_H = ROOT / "Source/Latebrake/Public/UI/LatebrakeRaceHUD.h"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Track/RivalProximityIndicator.h"
RADAR = ROOT / "Source/Latebrake/Private/Track/RaceMiniMap.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
TEST = ROOT / "Tests/Standalone/RivalProximityIndicatorTests.cpp"


class RivalProximityBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HUD_H.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.radar = RADAR.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.test = TEST.read_text(encoding="utf-8")

    def test_results_menu_race_variable_is_declared_before_use(self):
        # Previously this reference appeared below the early menu-return,
        # making Race a use-before-declaration in a real UE compilation.
        declaration = "const latebrake::RaceProgress& Race = Mode->GetRace();"
        self.assertEqual(self.hud.count(declaration), 1)
        self.assertLess(self.hud.index(declaration),
                        self.hud.index("case latebrake::MenuPage::Results:"))
        self.assertLess(self.hud.index(declaration),
                        self.hud.index("if (Race.GetTimePenaltyCount() > 0)"))

    def test_real_single_radar_snapshot_drives_map_and_warning(self):
        self.assertIn('#include "Track/RivalProximityIndicator.h"', self.header)
        self.assertIn("latebrake::RivalProximityIndicator RivalProximityHint;",
                      self.header)
        self.assertEqual(self.hud.count("BuildRaceMiniMapSnapshot("), 1)
        self.assertIn("bRivalRace && bWayfindingEligible &&", self.hud)
        self.assertIn("Mode->TryGetRivalRadarPosition(RivalWorldPosition)",
                      self.hud)
        self.assertIn("RivalProximityHint.Update(", self.hud)
        self.assertIn("bWayfindingEligible && bRivalRace", self.hud)
        self.assertIn('TEXT("RIVAL CLOSE | %.0fm DIRECT")', self.hud)
        self.assertLess(self.hud.index("RivalProximityHint.Update("),
                        self.hud.index("if (!bCompact && Canvas->SizeX >= 1120)"))
        self.assertIn("if (Radar.Rival.Valid)", self.hud)

    def test_resets_on_menu_pawn_epoch_recovery_and_ineligible_frames(self):
        self.assertGreaterEqual(self.hud.count("RivalProximityHint.Reset();"), 4)
        for marker in (
            "LastWayfindingPawn.Get() != Car",
            "LastWayfindingSession != CurrentWayfindingSession",
            "LastWayfindingTeleport != CurrentWayfindingTeleport",
            "LastRivalVerifiedRecoveries != CurrentRivalVerifiedRecoveries",
            "Mode->IsFocusLossGuardActive()",
            "Mode->IsManualRacePauseActive()",
            "GetWorld()->GetDeltaSeconds()",
        ):
            self.assertIn(marker, self.hud)
        for marker in (
            "!radar.Player.Position.Valid", "!radar.Rival.Valid",
            "!radar.DirectRivalSeparationValid",
            "std::isfinite(radar.DirectRivalSeparationMeters)",
            "deltaSeconds > 0.25", "DebounceSeconds = 0.6",
            "EnterMeters = 15.0", "ExitMeters = 20.0",
        ):
            self.assertIn(marker, self.policy)

    def test_non_authoritative_and_production_linked_cross_platform(self):
        for forbidden in (
            "PassCheckpoint(", "AddTimePenalty(", "SaveRecords(",
            "SetActorLocation(", "SetAutonomousControls(",
        ):
            self.assertNotIn(forbidden, self.policy)
        for marker in (
            "RivalProximityIndicatorTests.cpp", "NAME RivalProximityIndicator",
            "Private/Track/RaceMiniMap.cpp", "cxx_std_20",
            "-Wpedantic -Werror",
        ):
            self.assertIn(marker, self.cmake)
        for marker in (
            "BuildRaceMiniMapSnapshot(", "radarAt(19.0)",
            "radarAt(21.0)", "quiet_NaN()", "warning.Reset()",
        ):
            self.assertIn(marker, self.test)


if __name__ == "__main__":
    unittest.main()
