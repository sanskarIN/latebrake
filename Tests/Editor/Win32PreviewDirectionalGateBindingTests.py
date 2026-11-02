"""Contracts for standalone Win32 2D checkpoint geometry; not Unreal game QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PREVIEW = ROOT / "Experiments/Win32Preview/LateBrakePreview.cpp"
WORKFLOW = ROOT / ".github/workflows/experimental-win32-preview.yml"


class Win32PreviewDirectionalGateBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.code = PREVIEW.read_text(encoding="utf-8")
        cls.workflow = WORKFLOW.read_text(encoding="utf-8")

    def test_real_update_requires_this_tick_physical_segment(self):
        self.assertIn("const float beforePlayerX=race.x,beforePlayerY=race.y;", self.code)
        self.assertIn("crossedGatePlane(race.nextGate,beforePlayerX,beforePlayerY,race.x,race.y)", self.code)
        self.assertIn("correctGateDirection(race.nextGate)", self.code)
        self.assertIn("race.readyForGate=false;creditGate();", self.code)
        self.assertIn("onroad(beforeX,beforeY) && onroad(afterX,afterY)", self.code)
        self.assertIn("before<0.0f && after>=0.0f && after>before", self.code)

    def test_synthetic_test_rejects_exploits_and_exercises_real_progress(self):
        test = self.code.split("static bool selftest()", 1)[1]
        for phrase in ("selfTestStage=20;", "for(int lap=0;lap<3;lap++)",
                       "for(int g=0;g<4;g++)", "Wrong-way motion rejected",
                       "On-gate orientation is not crossing",
                       "Already beyond gate cannot earn it",
                       "Forward swept crossing accepted",
                       "No stationary duplicate award",
                       "race.passes!=12||page!=RESULTS"):
            self.assertIn(phrase, test)
        self.assertIn("'--self-test'", self.workflow)
        self.assertIn("LateBrakePreviewV5.exe", self.workflow)

    def test_release_gate_stays_separate_from_unreal(self):
        self.assertIn("NOT Unreal", self.workflow)
        self.assertIn("UNVERIFIED", self.workflow)


if __name__ == "__main__":
    unittest.main()
