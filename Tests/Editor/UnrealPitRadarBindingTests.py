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
        radar = self.hud.split(
            "// Source-only prototype racing radar:", 1)[1].split(
            "    if (bCompact)", 1)[0]
        self.assertIn("if (!bCompact && Canvas->SizeX >= 1120)", radar)
        self.assertIn("latebrake::RacePitBay", radar)
        self.assertIn("NormalizeCircuitMap(latebrake::RacePitBay)", radar)
        self.assertIn("IsOnPavedSurface(latebrake::RacePitBay)", radar)
        self.assertIn('DrawText(TEXT("PIT")', radar)
        self.assertIn("DrawRect(PitColor,", radar)
        self.assertIn("RacePitBay{0.0, -1300.0}", self.pit)

    def test_radar_remains_visual_only(self):
        block = self.hud.split(
            "// Optional source-defined pit map marker:", 1)[1].split(
            "if (Radar.NextGate.Valid)", 1)[0]
        for forbidden in ("PitService.Observe(", "RepairAtPit(", "RefillAtPit(",
                          "ReplaceAtPit(", "SaveRecords(", "EarnCredits(",
                          "RegisterCheckpoint(", "SpawnActor(", "SetActorLocation(",
                          "AddForce(", "AddTorque("):
            self.assertNotIn(forbidden, block)
        self.assertIn("NAME RacePitRadar", self.cmake)
        self.assertIn("RacePitRadarTests.cpp", self.cmake)


if __name__ == "__main__":
    unittest.main()
