"""Standalone algorithm fixture tests for visual ribbon geometry and contract."""
import math
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Standalone"))
from VerifyRoundedSmoothRoadObj import (SECTIONS, LAP, WIDTH, TOP, BOTTOM,
                                        centerline_at, expected_vertex)


class RoundedSmoothSurfaceSourceTests(unittest.TestCase):
    def test_road_geometry_is_continuous_and_valid(self):
        self.assertEqual(SECTIONS, 1024)
        for i in range(SECTIONS):
            x, y, heading = centerline_at(LAP * i / SECTIONS)
            self.assertTrue(math.isfinite(x) and math.isfinite(y))
            self.assertTrue(math.isfinite(heading))
            left = expected_vertex(4 * i)
            right = expected_vertex(4 * i + 1)
            self.assertAlmostEqual(math.dist(left[:2], right[:2]), 2 * WIDTH)
            self.assertAlmostEqual(left[2], TOP)
            self.assertAlmostEqual(expected_vertex(4 * i + 2)[2], BOTTOM)
        first = expected_vertex(0)
        last = expected_vertex(4 * (SECTIONS - 1))
        self.assertLess(math.dist(first[:2], last[:2]), 40)

    def test_round_corners_use_genuine_headings_not_rectangular_slabs(self):
        # Each arc transitions continuously through four quarter circles.
        curved = []
        for i in range(SECTIONS):
            _, _, deg = centerline_at(LAP * i / SECTIONS)
            if deg % 90.0 not in (0.0,):
                curved.append(deg)
        self.assertGreater(len(curved), 180)
        self.assertTrue(all(0 <= deg < 360.000000001 for deg in curved))

    def test_experimental_mesh_never_claims_gameplay_or_collisions(self):
        header = (Path(__file__).resolve().parents[2] /
                  "Source/Latebrake/Public/Track/RoundedCircuitSmoothMesh.h").read_text(
                      encoding="utf-8")
        source = (Path(__file__).resolve().parents[1] /
                  "Standalone/RoundedSmoothRoadObjExporter.cpp").read_text(
                      encoding="utf-8")
        for keyword in ("RoundedRoadCenterlineAt(", "RoundedSmoothHalfWidthCm",
                        "RoundedSmoothTopCm", "BuildRoundedSmoothRoadMesh"):
            self.assertIn(keyword, header)
        self.assertIn("visual-only", header)
        self.assertIn("collision disabled", source)
        self.assertNotIn("SpawnBlock(", header)
        self.assertNotIn("AddForce(", header)


if __name__ == "__main__":
    unittest.main()
