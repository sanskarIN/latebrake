"""Read-only source contract for the sampled post-race focus shortlist."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(name):
    return (ROOT / name).read_text(encoding="utf-8")


class DrivingCoachFocusSourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = source("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.policy = source("Source/Latebrake/Public/Racing/DrivingCoachFocusAreas.h")
        cls.cmake = source("Tests/Standalone/CMakeLists.txt")
        cls.tests = source("Tests/Standalone/DrivingCoachFocusAreasTests.cpp")

    def test_result_screen_requires_frozen_race_completion(self):
        results = self.hud.split("case latebrake::MenuPage::Results:", 1)[1]
        self.assertIn("if (latebrake::CanAwardFinishedRace(Race))", results)
        self.assertIn("VerifyDrivingCoachIntegrity(", results)
        self.assertIn("BuildDrivingCoachFocusAreas(Coaching)", results)
        self.assertIn("DrivingCoachFocusLabel(", results)
        self.assertLess(results.index("VerifyDrivingCoachIntegrity("),
                        results.index("BuildDrivingCoachFocusAreas(Coaching)"))
        self.assertIn('TEXT("COACH FOCUS %.1f-%.1fs | %s")', results)
        self.assertIn('TEXT("COACH IS ADVISORY: NO TIMING OR CREDIT AUTHORITY")',
                      results)

    def test_no_time_rewards_or_actor_mutation(self):
        for forbidden in (
            "Race.PassCheckpoint(", "Race.Advance(", "EarnCredits(",
            "SaveRecords(", "Garage.Unlock(", "SetActorTransform(",
            "SetActorLocation(", "AddForce(", "SetPhysicsLinearVelocity(",
            "GameMode->", "UWorld"):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, self.policy)
        self.assertIn("ValidDrivingCoachReadout(readout)", self.policy)
        self.assertIn("previous->ContinuityEpoch==window.ContinuityEpoch", self.policy)
        self.assertIn("window.Complete", self.policy)
        self.assertIn("!review.NoRewards || !review.NoOfficialTiming", self.policy)

    def test_ci_includes_portable_focus_regressions(self):
        self.assertIn("DrivingCoachFocusAreasTests.cpp", self.cmake)
        self.assertIn("NAME DrivingCoachFocusAreas", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)
        self.assertIn("FuzzWithAcceptedObservations", self.tests)
        self.assertIn("FragmentAndPauseBoundaries", self.tests)
        self.assertIn("DirectClassifierContract", self.tests)


if __name__ == "__main__":
    unittest.main()
