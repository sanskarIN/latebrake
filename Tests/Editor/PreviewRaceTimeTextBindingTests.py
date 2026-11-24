"""Real Windows 2D HUD uses validated precision timing strings."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class PreviewRaceTimeTextBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app=read("Experiments/Win32Preview/LateBrakePreview.cpp")
        cls.header=read("Experiments/Win32Preview/PreviewRaceTimeText.h")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")
        cls.tests=read("Tests/Standalone/PreviewRaceTimeTextTests.cpp")

    def test_real_race_and_result_use_hundredths(self):
        self.assertIn('#include "PreviewRaceTimeText.h"',self.app)
        for marker in (
            "race.elapsed,true,liveTime",
            "race.best,race.best>0.0f,bestLap",
            "personalTrackBestMs[selectedTrack],savedPb",
            "race.elapsed,true,finishTime",
            "race.best,race.best>0.0f,finishBest",
            "selfTestStage=91;",
            "text(468,253,finishTime,CREAM(),3)",
            "text(130,177,liveTime,CREAM(),2)",
        ):
            self.assertIn(marker,self.app)

    def test_formatter_is_no_crt_and_zero_best_not_fake(self):
        self.assertNotIn("sprintf(",self.header)
        self.assertNotIn("printf(",self.header)
        self.assertIn("PreviewTimeUnavailable(output)",self.header)
        self.assertIn("std::strcmp(out",self.tests)
        self.assertIn("100000",self.tests)
        self.assertIn("NAME PreviewRaceTimeText",self.cmake)

if __name__=="__main__":
    unittest.main()
