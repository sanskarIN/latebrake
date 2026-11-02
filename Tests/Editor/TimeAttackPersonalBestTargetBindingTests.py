"""Source binding tests for solo Time Attack personal-best clock HUD, not runtime QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class TimeAttackPersonalBestTargetBindingTests(unittest.TestCase):
    def test_actual_hud_binds_only_to_real_race_and_saved_event_pb(self):
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(encoding="utf-8")
        for phrase in ('#include "Racing/TimeAttackPersonalBestTarget.h"',
                       "latebrake::MeasureTimeAttackPersonalBestTarget(",
                       "Mode->GetSelectedEvent()", "Mode->IsCareerRace()",
                       "Race.GetElapsedSeconds()", "Mode->GetEventBestSeconds()",
                       "Race.IsFinished()", "Mode->IsRaceStartGreen()",
                       "Mode->HasLocalPlayerDriver()", "Mode->IsFocusLossGuardActive()",
                       "Mode->IsManualRacePauseActive()", "!bCompact && !bTouchHUD",
                       "PB CLOCK", "DeadlineReached"):
            self.assertIn(phrase, hud)

    def test_pure_policy_never_modifies_scoring_or_save(self):
        policy = (ROOT / "Source/Latebrake/Public/Racing/TimeAttackPersonalBestTarget.h").read_text(encoding="utf-8")
        for phrase in ("SanitizeBestTime(savedBestSeconds)", "careerRace",
                       "finishedRace", "std::isfinite(elapsedSeconds)"):
            self.assertIn(phrase, policy)
        for forbidden in ("ImprovedBestTime(", "AwardCredits(", "SaveRecords(",
                          "PassCheckpoint(", "AddForce(", "SetActorLocation("):
            self.assertNotIn(forbidden, policy)

    def test_portable_compiled_regression_registered(self):
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("NAME TimeAttackPersonalBestTarget", cmake)
        self.assertIn("TimeAttackPersonalBestTargetTests.cpp", cmake)


if __name__ == "__main__":
    unittest.main()
