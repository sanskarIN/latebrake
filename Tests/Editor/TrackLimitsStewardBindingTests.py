"""Static source integration checks for authoritative Unreal track limits.

These do NOT compile Unreal, drive a real car, or verify an installed game.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "Source/Latebrake/Public/Track/TrackLimitsSteward.h"
MODE_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
MODE_CPP = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"

class TrackLimitsStewardBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = {f: f.read_text(encoding="utf-8")
                      for f in (HEADER, MODE_H, MODE_CPP, HUD, CMAKE)}

    def test_authoritative_game_mode_only(self):
        header, h, cpp, hud = (self.source[x] for x in
                              (HEADER, MODE_H, MODE_CPP, HUD))
        self.assertIn('#include "Track/TrackLimitsSteward.h"', h)
        self.assertIn("latebrake::TrackLimitsSteward TrackLimits;", h)
        self.assertIn("TrackLimits.Update(", cpp)
        self.assertIn("StewardsDriver->GetVelocity().Size2D()", cpp)
        self.assertIn("Race.AddTimePenalty(seconds)", cpp)
        self.assertIn("StartLights.IsGreen()", cpp)
        self.assertIn("HasLocalPlayerDriver()", cpp)
        self.assertIn("TrackLimits.Reset();", cpp)
        self.assertGreaterEqual(cpp.count("TrackLimits.Suspend();"), 4)
        self.assertIn("const latebrake::TrackLimitsSteward& GetTrackLimits()", h)
        self.assertIn("Mode->GetTrackLimits().Snapshot()", hud)
        self.assertNotIn("AddTimePenalty(", hud)
        self.assertIn("IsOnPavedSurface(position)", header)
        self.assertNotIn("SaveGame", header)

    def test_source_suite_registered_in_real_ctest(self):
        cmake = self.source[CMAKE]
        for token in ("TrackLimitsStewardTests.cpp", "NAME TrackLimitsSteward",
                      "latebrake_track_limits_steward_tests",
                      "-Wpedantic -Werror"):
            self.assertIn(token, cmake)

    def test_penalty_is_coupled_to_authoritative_clock_only(self):
        cpp = self.source[MODE_CPP]
        self.assertIn("TrackLimitsRules::PenaltySeconds", self.source[HEADER])
        self.assertIn("if (!std::forward<ChargePenalty>(chargePenalty)", self.source[HEADER])
        self.assertIn("TrackLimits.Suspend();\n    SuspendBestLapGhost();", cpp)
        self.assertIn("TrackLimits.Reset();\n    // A saved PB", cpp)
        self.assertIn("TRACK LIMIT WARNING", self.source[HUD])
        self.assertIn("TRACK LIMITS  %d", self.source[HUD])

if __name__ == "__main__":
    unittest.main()
