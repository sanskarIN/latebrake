"""Source-only regression tests for a non-authoritative pit marker.

The portable race radar is not a UE rendering/runtime qualification.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class UnrealPitRadarBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")
        cls.pit = (ROOT / "Source/Latebrake/Public/Racing/RacePitService.h").read_text(encoding="utf-8")
        cls.cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")

    def test_radar_icon_uses_exact_physical_service_point(self):
        radar = self.hud.split("// Live north-up radar:", 1)[1].split(
            "    if (bCompact)", 1)[0]
        policy = (ROOT / "Source/Latebrake/Public/UI/HudRaceRadar.h").read_text(encoding="utf-8")
        self.assertIn("if (RadarLayout.Valid)", radar)
        self.assertIn("latebrake::IsOnPavedSurface(latebrake::RacePitBay)", radar)
        self.assertIn("BuildHudRadarPresentation(", radar)
        self.assertIn("HudRadarMarkerKind::Pit", radar)
        self.assertIn('Name=TEXT("PIT")', radar)
        self.assertIn("NormalizeCircuitMap(RacePitBay)", policy)
        self.assertIn("if(pitReady && IsOnPavedSurface(RacePitBay))", policy)
        self.assertIn("RacePitBay{0.0, -1300.0}", self.pit)

    def test_radar_remains_visual_only(self):
        block = self.hud.split("// Live north-up radar:", 1)[1].split(
            "    if (bCompact)", 1)[0]
        for forbidden in ("PitService.Observe(", "RepairAtPit(", "RefillAtPit(",
                          "ReplaceAtPit(", "SaveRecords(", "EarnCredits(",
                          "RegisterCheckpoint(", "SpawnActor(", "SetActorLocation(",
                          "AddForce(", "AddTorque("):
            self.assertNotIn(forbidden, block)
        self.assertIn("NAME RacePitRadar", self.cmake)
        self.assertIn("RacePitRadarTests.cpp", self.cmake)


if __name__ == "__main__":
    unittest.main()
