"""Source-only binding tests for the actual Unreal rival result observation.

This checks source composition and cannot substitute licensed UE/UHT compile.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODE_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
MODE_CPP = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
HUD_CPP = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
POLICY = ROOT / "Source/Latebrake/Public/Racing/RivalFinishObservation.h"


class UnrealRivalFinishObservationBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.h = MODE_H.read_text(encoding="utf-8")
        cls.cpp = MODE_CPP.read_text(encoding="utf-8")
        cls.hud = HUD_CPP.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_actual_eligible_finish_captures_observation_before_save(self):
        finish = self.cpp.split("void ALatebrakeRaceGameMode::RegisterCheckpoint(",1)[1].split(
            "void ALatebrakeRaceGameMode::SyncDesktopMenuPointerMode(",1)[0]
        self.assertLess(finish.index("Race.PassCheckpoint("),
                        finish.index("ValidateFinishTiming(Race)"))
        self.assertLess(finish.index("ValidateFinishTiming(Race)"),
                        finish.index("latebrake::ObserveRivalFinish({"))
        self.assertLess(finish.index("latebrake::ObserveRivalFinish({"),
                        finish.index("SaveRecords();"))
        for required in ("RivalCar->IsRacePhysicsReady()",
                         "RivalCar->GetVerifiedGateCount()",
                         "RivalCar->GetExpectedLaps()",
                         "RivalCar->GetApproximateLaps()",
                         "RivalCar->GetNextVerifiedGate()"):
            self.assertIn(required, finish)
        self.assertIn("GetLastRivalFinishObservation() const", self.h)
        self.assertIn("LastRivalFinishObservation = {};", finish)

    def test_new_attempt_and_load_clear_old_finish(self):
        self.assertEqual(self.cpp.count("LastRivalFinishObservation = {};"), 3)
        restart = self.cpp.split("void ALatebrakeRaceGameMode::RestartRace()",1)[1].split(
            "void ALatebrakeRaceGameMode::RecoverPlayer(",1)[0]
        self.assertLess(restart.index("LastRivalFinishObservation = {};"),
                        restart.index("LaunchMeter.Reset();"))

    def test_results_text_never_claims_race_rank(self):
        results = self.hud.split("case latebrake::MenuPage::Results:",1)[1].split(
            "case latebrake::MenuPage::Pause:",1)[0]
        for required in ("GetLastRivalFinishObservation().State",
                         "RivalRouteAlreadyComplete", "RivalStillOnRoute",
                         "UNOFFICIAL: RIVAL ALREADY CLEARED FINAL GATE",
                         "UNOFFICIAL: RIVAL ROUTE INCOMPLETE AT FINISH"):
            self.assertIn(required, results)
        self.assertIn("sample.Career", self.policy)
        for forbidden in ("SaveRecords(", "Race.PassCheckpoint(", "EarnCredits(",
                          "AddTimePenalty(", "CommitFinish("):
            self.assertNotIn(forbidden, self.policy)

    def test_compiled_regression_and_visible_docs(self):
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("RivalFinishObservationTests.cpp", cmake)
        self.assertIn("add_test(NAME RivalFinishObservation", cmake)
        nav = (ROOT / "mkdocs.yml").read_text(encoding="utf-8")
        self.assertIn("RIVAL_FINISH_OBSERVATION_QA.md", nav)


if __name__ == "__main__":
    unittest.main()
