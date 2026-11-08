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
        self.assertIn("StewardTrackedPawn.Get() == StewardsDriver", cpp)
        self.assertIn("StewardTeleportRevision == StewardsDriver->GetTeleportRevision()", cpp)
        self.assertIn("StewardCheckpointSessionRevision == CheckpointSession.Current()", cpp)
        self.assertIn("bStewardReady && bSameStewardSession", cpp)
        self.assertIn("StewardsDriver->GetVelocity().Size2D()", cpp)
        self.assertIn("Race.AddTimePenalty(seconds)", cpp)
        self.assertIn("StartLights.IsGreen()", cpp)
        self.assertIn("HasLocalPlayerDriver()", cpp)
        self.assertIn("TrackLimits.Reset();", cpp)
        for required in (
            "Race.Reset();", "TrackLimits.Reset();",
            "StewardTrackedPawn.Reset();",
            "StewardTeleportRevision = 0;",
            "StewardCheckpointSessionRevision = 0;",
        ):
            self.assertGreaterEqual(cpp.count(required), 2)
        self.assertIn("// A saved PB may change at finish", cpp)
        self.assertIn("TRACK LIMIT WARNING", self.source[HUD])
        self.assertIn("TRACK LIMITS  %d", self.source[HUD])

if __name__ == "__main__":
    unittest.main()
