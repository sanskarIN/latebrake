"""3D playtest procedure is usable without pretending a 2D artifact is Unreal."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
class Pr452VisualTestPlanBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.doc=(ROOT/"docs/PR452_UNREAL_3D_PLAYTEST.md").read_text(encoding="utf-8")
        cls.main=(ROOT/"README.md").read_text(encoding="utf-8")
        cls.old=(ROOT/"docs/WINDOWS_PERSONAL_PLAYTEST.md").read_text(encoding="utf-8")
    def test_reviewed_pr_and_actual_engine_required(self):
        for item in ("pull/452/head", "git switch --detach",
                     "git rev-parse FETCH_HEAD", "git lfs pull",
                     "expected_head_sha", "pr_number=452",
                     "validation=Editor", "validation=Win64",
                     "Build-WindowsPlaytest.ps1", "-Launch",
                     "Run-WindowsPlaytest.ps1", "LATEBRAKE_UNREAL_ROOT",
                     "UNVERIFIED"):
            self.assertIn(item,self.doc)
        self.assertIn("PR452_UNREAL_3D_PLAYTEST.md",self.main)
        self.assertIn("PR452_UNREAL_3D_PLAYTEST.md",self.old)
    def test_review_covers_new_car_ux_and_existing_data_safety(self):
        for item in ("STREET 7", "SPORT 5", "TRACK 12",
                     "SOURCE MESH COMMITTED", "LEGACY CUBE FALLBACK",
                     "F3", "F5", "F6", "F7", "F8",
                     "Save and reload", "Pause", "Dusk",
                     "Review-WindowsPlaytest.py"):
            self.assertIn(item,self.doc)
        self.assertIn("NOT the six-car UE5.8 racing game",self.doc)
        self.assertIn("cannot qualify 3D",self.doc)
    def test_workflow_and_build_scripts_are_existing_real_paths(self):
        for path in (".github/workflows/validate-draft-unreal-pr.yml",
                     "scripts/Build-WindowsPlaytest.ps1",
                     "scripts/Run-WindowsPlaytest.ps1",
                     "scripts/Review-WindowsPlaytest.py",
                     "Experiments/Win32Preview/TRIAL_QUICKSTART.md"):
            self.assertTrue((ROOT/path).is_file(),path)
if __name__=="__main__":
    unittest.main()
