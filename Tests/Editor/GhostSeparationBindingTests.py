"""Static Unreal integration checks for cosmetic live ghost separation.

These source contracts are not a substitute for UE 5.8 compilation or an
installed Windows/Android race recording.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class GhostSeparationBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def read(path):
            return (ROOT / path).read_text(encoding="utf-8")

        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.mode_header = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_visible_beacon_required_and_real_chassis_sampling(self):
        source = self.mode.split(
            "bool ALatebrakeRaceGameMode::TryGetBestLapGhostSeparationMeters(", 1
        )[1].split(
            "void ALatebrakeRaceGameMode::UpdateBestLapGhostMarker(", 1
        )[0]
        for requirement in (
            "OutMeters = 0.0",
            "!bCircuitReady",
            "!StartLights.IsGreen()",
            "Race.IsFinished()",
            "Race.GetCompletedLaps() <= 0",
            "!GhostRecorder.HasBest()",
            "!GhostDisplay.IsEnabled()",
            "Menu.IsOpen()",
            "UGameplayStatics::IsGamePaused(this)",
            "AppFocusGuard.IsSuspended()",
            "ManualPause.IsSuspended()",
            "!HasLocalPlayerDriver()",
            "!IsValid(BestLapGhostMarker)",
            "BestLapGhostMarker->IsHidden()",
            "Car->IsRacePhysicsReady()",
            "GhostRecorder.Evaluate(Race.GetCurrentLapSeconds(), Sample)",
            "Car->GetActorLocation()",
            "Sample.X, Sample.Y, Sample.Z",
            "TryComputeGhostSeparationMeters({",
        ):
            self.assertIn(requirement, source)
        self.assertIn("TryGetBestLapGhostSeparationMeters(double& OutMeters) const",
                      self.mode_header)
        self.assertIn('#include "Racing/GhostSeparation.h"', self.mode)
        for forbidden in ("AddTimePenalty(", "PassCheckpoint(", "SaveRecords(",
                          "AddForce(", "AddCredits(", "Award("):
            self.assertNotIn(forbidden, source)

    def test_optional_hud_readout_does_not_replace_race_clock(self):
        self.assertIn("TryGetBestLapGhostSeparationMeters(GhostDistanceMeters)",
                      self.hud)
        self.assertIn("GHOST DIST  %.0f m (SAME LAP TIME)", self.hud)
        self.assertIn("GHOST DIST  %.0f ft (SAME LAP TIME)", self.hud)
        self.assertIn("GhostDistanceMeters * 3.280839895", self.hud)
        self.assertIn("if (!bCompact)", self.hud)
        self.assertLess(self.hud.index("TIME  %02d:%05.2f"),
                        self.hud.index("GHOST DIST  %.0f m"))
        self.assertIn("add_test(NAME GhostSeparation", self.cmake)


if __name__ == "__main__":
    unittest.main()
