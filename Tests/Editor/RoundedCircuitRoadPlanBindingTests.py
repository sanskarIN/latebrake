"""Source-only preflight: rounded road must not silently replace live UE road.

The tests do not compile Unreal, cook a map, render assets or prove handling.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

class RoundedCircuitRoadPlanBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.plan = read("Source/Latebrake/Public/Track/RoundedCircuitRoadPlan.h")
        cls.test = read("Tests/Standalone/RoundedCircuitRoadPlanTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.active = read("Source/Latebrake/Public/Track/CircuitCollisionPlan.h")
        cls.track = read("docs/TRACK.md")

    def test_physically_defined_bends_not_painted_fake_roads(self):
        for name in ("RoundedPhysicalRoadCount", "RoundedCornerSegmentCount = 16",
                     "RoundedHalfRoadWidthCm = 450.0", "RoundedRoadTopCm = -1.0",
                     "YawDegrees", "ContainsRoundedPhysicalRoadPoint(",
                     "IsOnRoundedPhysicalRoad(CircuitPoint point)",
                     "RoundedRoadCenterlineAt(", "std::fmod(distanceCm"):
            self.assertIn(name, self.plan)
        self.assertIn("std::isfinite(point.X)", self.plan)
        self.assertIn("std::isfinite(point.Y)", self.plan)

    def test_exact_cmake_and_corner_coverage(self):
        self.assertIn("RoundedCircuitRoadPlanTests.cpp", self.cmake)
        self.assertIn("NAME RoundedCircuitRoadPlan", self.cmake)
        for expected in (
            "all existing four scoring gates still lie on physical asphalt",
            "existing player and rival grid spawns lie on physical asphalt",
            "existing player recovery positions remain on asphalt",
            "existing rival recovery positions remain on asphalt",
            "the full corner interior and road edges have NO gaps",
            "outside-edge/grass samples must NOT be physically paved",
            "ALL 4096 centerline lap samples are on physical asphalt",
        ):
            self.assertIn(expected, self.test)

    def test_runtime_still_old_road_until_licensed_engine_qa(self):
        self.assertIn("CircuitCollisionBlockCount", self.mode)
        self.assertIn("CircuitCollisionBlocks", self.active)
        self.assertNotIn('#include "Track/RoundedCircuitRoadPlan.h"', self.mode)
        self.assertNotIn("IsOnRoundedPhysicalRoad(", self.mode)
        self.assertNotIn("PlannedRoundedPhysicalRoadPiece(", self.mode)
        self.assertIn("ROUNDED_CIRCUIT_PLAN_QA.md", self.track)

if __name__ == "__main__":
    unittest.main()
