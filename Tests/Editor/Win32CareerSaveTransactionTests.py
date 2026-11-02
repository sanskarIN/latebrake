"""Source contracts for experimental V5 transactional career persistence.

No Unreal Engine build or actual Windows file-system fault injection is claimed.
Native Windows --self-test exercises the real functions with synthetic I/O.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Experiments/Win32Preview/LateBrakePreview.cpp"
WORKFLOW = ROOT / ".github/workflows/experimental-win32-preview.yml"


class Win32CareerSaveTransactionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.code = SOURCE.read_text(encoding="utf-8")
        cls.workflow = WORKFLOW.read_text(encoding="utf-8")

    def test_save_requires_a_committed_file(self):
        save = self.code.split("static bool saveProgress()", 1)[1].split(
            "// Separate versioned UI preferences", 1)[0]
        for marker in (
            "if(testing)return !simulateProgressSaveFailure;",
            "WriteFile(h,&d,sizeof(d),&written,0)",
            "written==sizeof(d)",
            "FlushFileBuffers(h)",
            "CloseHandle(h)",
            "MoveFileExA(tmp,file,0x1u|0x8u)",
            "DeleteFileA(tmp);return false;",
            "return true;",
        ):
            self.assertIn(marker, save)
        self.assertNotIn("if(testing)return;", save)

    def test_finish_rollback_and_result_message(self):
        finish = self.code.split("static void finishRace(bool winner)", 1)[1].split(
            "static bool correctGateDirection", 1)[0]
        for marker in (
            "const int oldCredits=credits,oldCompleted=completed,oldStars=careerStars;",
            "if(saveProgress())",
            "bonusCredits=credits-oldCredits;",
            "race.awarded=true;",
            "credits=oldCredits;completed=oldCompleted;careerStars=oldStars;",
            "progressSaveFailed=true;",
            "careerStars=(int)minf(100000.0f",
            "credits=(int)minf(500000.0f",
        ):
            self.assertIn(marker, finish)
        results = self.code.split("static void resultsOverlayV3()", 1)[1].split(
            "static void pausedOverlayV3()", 1)[0]
        self.assertIn('progressSaveFailed?"SAVE FAILED"', results)
        self.assertIn('REWARD NOT SAVED. CHECK LOCAL STORAGE.', results)

    def test_unlock_rolls_back_payment_and_ownership(self):
        unlock = self.code.split("static void unlockSelectedCar()", 1)[1].split(
            "static void activateSelection()", 1)[0]
        for marker in (
            "const int oldCredits=credits,oldMask=ownedMask;",
            "credits-=carPrices[selectedCar];ownedMask|=1<<selectedCar;",
            "if(saveProgress())",
            "credits=oldCredits;ownedMask=oldMask;",
            "SAVE FAILED - UNLOCK AND CREDITS RESTORED",
        ):
            self.assertIn(marker, unlock)

    def test_native_selftest_exercises_failure_success_and_caps(self):
        test = self.code.split("static bool selftest()", 1)[1]
        for marker in (
            "selfTestStage=80;",
            "selfTestStage=81;",
            "selfTestStage=82;",
            "simulateProgressSaveFailure=true;",
            "simulateProgressSaveFailure=false;",
            "finishRace(true);",
            "unlockSelectedCar();",
            "race.awarded",
            "progressSaveFailed",
            "credits!=500000",
            "careerStars!=100000",
            "bonusCredits!=5",
        ):
            self.assertIn(marker, test)
        self.assertIn("'--self-test'", self.workflow)
        self.assertIn("Prove synthetic self-test does not modify user career data",
                      self.workflow)


if __name__ == "__main__":
    unittest.main()
