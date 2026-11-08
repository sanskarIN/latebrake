"""Source-only guard: proposed 1.70km AI is not secretly the live rival."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

class WorldScaleCandidateRivalPilotBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pilot = read("Source/Latebrake/Public/AI/WorldScaleCandidateRivalPilot.h")
        cls.tests = read("Tests/Standalone/WorldScaleCandidateRivalPilotTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.rival = read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")

    def test_future_controller_uses_actual_candidate_plan_and_safe_envelope(self):
        for marker in (
            '#include "AI/RivalPilot.h"',
            '#include "Track/WorldScaleRacePacePlan.h"',
            "ProjectWorldScaleRoute(plan, sample.Position)",
            "road.InsidePhysicalCandidateRoad",
            "SampleWorldScalePace(",
            "sample.ForwardSpeedCmps < -50.0",
            "2.0 * sample.FlatSpeedCmps * sample.DeltaSeconds",
            "std::remainder(",
            "Reset();",
            "return {};",
        ):
            self.assertIn(marker, self.pilot)

    def test_no_unverified_gameplay_authority(self):
        for marker in ("AddForce", "SetActorLocation", "PassCheckpoint(",
                       "AwardCredits", "SaveGame(", "AddTimePenalty("):
            self.assertNotIn(marker, self.pilot)
        self.assertNotIn("WorldScaleCandidateRivalPilot", self.rival)
        self.assertNotIn("WorldScaleCandidateRivalPilot", self.mode)

    def test_production_source_and_portable_tests_registered(self):
        self.assertIn("WorldScaleCandidateRivalPilotTests.cpp", self.cmake)
        self.assertIn("NAME WorldScaleCandidateRivalPilot", self.cmake)
        self.assertIn("30.0, 60.0, 120.0", self.tests)
        self.assertIn("WorldScaleCircuitLapCm - 95.0", self.tests)
        self.assertIn("opposite-direction rival", self.tests)
        self.assertIn("unverified 105m teleport", self.tests)

if __name__ == "__main__":
    unittest.main()
