"""Source-only check: future 1.70km pace plan is not active game authority."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class WorldScaleRacePaceBindingTests(unittest.TestCase):
    def test_future_track_pace_is_derived_and_not_yet_live(self):
        pace=read("Source/Latebrake/Public/Track/WorldScaleRacePacePlan.h")
        game=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        rival=read("Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp")
        cmake=read("Tests/Standalone/CMakeLists.txt")
        tests=read("Tests/Standalone/WorldScaleRacePaceTests.cpp")
        for token in ("WorldScaleCenterlineAt(", "WorldScaleSampleCurvature(",
                      "WorldScalePaceBrakeAccelCmps2", "WorldScalePaceDriveAccelCmps2",
                      "IsOnWorldScaleRoad(car)", "ProjectWorldScaleRoute("):
            self.assertIn(token,pace)
        for token in ("PassCheckpoint(", "AddTimePenalty(", "AddForce(",
                      "SaveGame"):
            self.assertNotIn(token,pace)
        self.assertNotIn("WorldScalePacePlan",game)
        self.assertNotIn("WorldScalePacePlan",rival)
        self.assertIn("NAME WorldScaleRacePacePlan",cmake)
        self.assertIn("WorldScaleRacePaceTests.cpp",cmake)
        self.assertIn("3072",tests)
        self.assertIn("8192",tests)
if __name__=="__main__":unittest.main()
