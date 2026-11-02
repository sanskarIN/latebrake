"""Source-only contracts for Win32 preview mode-isolated rivals.

The strict policy is compiled by PreviewRaceModesTests.cpp; this test
checks that the preview actually consumes the per-attempt rival count.
Neither test proves real Windows/Unreal gameplay or hardware QA.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CODE = ROOT / "Experiments/Win32Preview/LateBrakePreview.cpp"
POLICY = ROOT / "Experiments/Win32Preview/PreviewRaceModes.h"
WORKFLOW = ROOT / ".github/workflows/experimental-win32-preview.yml"


class Win32SoloRaceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = CODE.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.workflow = WORKFLOW.read_text(encoding="utf-8")

    def test_solo_and_rival_modes_have_explicit_rules(self):
        for rule in (
            "case 0: return {3, 3, true}",
            "case 1: return {1, 0, true}",
            "case 2: return {2, 3, true}",
            "case 3: return {2, 0, true}",
            "default: return {}",
        ):
            with self.subTest(rule=rule):
                self.assertIn(rule, self.policy)

    def test_simulation_uses_snapshot_not_menu_only(self):
        for marker in (
            '#include "PreviewRaceModes.h"',
            "int lap,nextGate,passes,pitStops,rivalCount;",
            "race.rivalCount=mode.RivalCount",
            "if(!mode.Valid)",
            "for(int i=0;i<race.rivalCount;i++){",
            "rivalDuelAlreadyLost()",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, self.source)
        self.assertEqual(
            self.source.count("for(int i=0;i<race.rivalCount;i++){"), 3
        )
        self.assertNotIn("race.ai[2].rounds>=totalLaps", self.source)

    def test_visuals_and_headless_probe_match_race_policy(self):
        for marker in (
            "for(int i=0;i<race.rivalCount;i++)dot",
            "GOLD YOU / SOLO EVENT",
            "SOLO CLOCK CHALLENGE",
            "for(int event=0;event<4;event++)",
            "if(policy.RivalCount==0)",
            "race.ai[i].rounds!=0",
            "selfTestStage=12",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, self.source)
        self.assertIn("Build-Windows.ps1", self.workflow)
        self.assertIn("'--self-test'", self.workflow)
        self.assertIn("UNVERIFIED", self.workflow)


if __name__ == "__main__":
    unittest.main()
