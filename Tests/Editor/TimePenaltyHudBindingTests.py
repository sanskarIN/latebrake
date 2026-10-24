"""Source-level contracts only: do not claim a compiled Unreal HUD."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class TimePenaltyHudBindingTests(unittest.TestCase):
    def test_hud_uses_portable_accounting_not_own_counter(self):
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")
        race_h = (ROOT / "Source/Latebrake/Public/Racing/RaceProgress.h").read_text(encoding="utf-8")
        race_cpp = (ROOT / "Source/Latebrake/Private/Racing/RaceProgress.cpp").read_text(encoding="utf-8")
        self.assertIn("Race.GetTimePenaltyCount()", hud)
        self.assertIn("Race.GetTotalPenaltySeconds()", hud)
        self.assertIn("ALREADY IN TIME", hud)
        self.assertIn("Time penalties included:", hud)
        self.assertIn("GetTimePenaltyCount() const noexcept", race_h)
        self.assertIn("GetTotalPenaltySeconds() const noexcept", race_h)
        self.assertIn("TimePenaltyCount = 0;", race_cpp)
        self.assertIn("TotalPenaltySeconds = 0.0;", race_cpp)
        self.assertIn("TotalPenaltySeconds = newTotal;", race_cpp)

if __name__ == "__main__":
    unittest.main()
