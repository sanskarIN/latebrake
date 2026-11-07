"""Portable tests of all exact geometry and corruption guards for trace QA.

No game, Unreal Editor, Chaos physics or hardware contact is emulated.
"""
import copy
import json
import math
import sys
import tempfile
import unittest
from pathlib import Path

TESTS = Path(__file__).resolve().parents[1] / "Standalone"
sys.path.insert(0, str(TESTS))
from WorldScaleCollisionTracePlan import (  # noqa: E402
    EXTRA_SEAM_DISTANCES_CM,
    EXPECTED_SAMPLES,
    OFFSETS_CM,
    STATIONS,
    make_plan,
    save_plan,
    validate_plan,
)


class WorldScaleCollisionTracePlanTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.plan = make_plan()

    def test_full_lap_covers_all_256_stations_and_both_seam_sides(self):
        self.assertEqual(validate_plan(self.plan), EXPECTED_SAMPLES)
        self.assertEqual(EXPECTED_SAMPLES, 1290)
        self.assertEqual(len(self.plan["samples"]), 1290)
        for station in range(STATIONS + 2):
            cases = self.plan["samples"][5*station:5*station+5]
            self.assertEqual([p["station"] for p in cases], [station]*5)
            self.assertEqual([p["offset_cm"] for p in cases], list(OFFSETS_CM))
            self.assertEqual([p["expected_actor"] for p in cases],
                             ["road", "road", "road", "ground", "ground"])
            for point in cases:
                self.assertTrue(math.isfinite(point["x_cm"]))
                self.assertTrue(math.isfinite(point["y_cm"]))
        self.assertAlmostEqual(self.plan["samples"][-10]["distance_cm"],
                               EXTRA_SEAM_DISTANCES_CM[0])
        self.assertAlmostEqual(self.plan["samples"][-5]["distance_cm"],
                               EXTRA_SEAM_DISTANCES_CM[1])

    def test_road_top_and_offroad_ground_are_distinct(self):
        for p in self.plan["samples"]:
            self.assertEqual(p["expected_z_cm"], -1.0 if p["expected_actor"] == "road"
                             else -37.5)
        self.assertEqual(self.plan["trace_start_z_cm"], 250.0)
        self.assertEqual(self.plan["trace_end_z_cm"], -200.0)
        self.assertEqual(self.plan["physical_road_width_cm"], 900)

    def test_stable_round_trip_and_source_identity(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "trace-plan.json"
            save_plan(path)
            before = path.read_bytes()
            self.assertEqual(before[-1:], b"\n")
            data = json.loads(before)
            self.assertEqual(validate_plan(data), EXPECTED_SAMPLES)
            save_plan(path)
            self.assertEqual(path.read_bytes(), before)

    def test_mutated_geometry_is_refused(self):
        for field, wrong in (
            ("x_cm", 200.0), ("y_cm", -200.0),
            ("distance_cm", 33.0), ("expected_z_cm", 7.0),
        ):
            with self.subTest(field=field):
                mutant = copy.deepcopy(self.plan)
                mutant["samples"][500][field] += wrong
                with self.assertRaisesRegex(ValueError, "Displaced continuous collision probe"):
                    validate_plan(mutant)

    def test_mutated_expected_hit_is_refused(self):
        mutant = copy.deepcopy(self.plan)
        mutant["samples"][2]["expected_actor"] = "ground"
        with self.assertRaisesRegex(ValueError, "Invalid continuous collision probe"):
            validate_plan(mutant)

    def test_mutated_seam_or_sample_order_is_refused(self):
        mutant = copy.deepcopy(self.plan)
        mutant["samples"][-1], mutant["samples"][-2] = (
            mutant["samples"][-2], mutant["samples"][-1]
        )
        with self.assertRaises(ValueError):
            validate_plan(mutant)

    def test_bad_road_width_and_metadata_rejected(self):
        for key, value in (("physical_road_width_cm", 9000),
                           ("station_count", 512),
                           ("sample_count", 1289),
                           ("units", "m")):
            mutant = copy.deepcopy(self.plan)
            mutant[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_plan(mutant)

    def test_malformed_nonfinite_and_boolean_values_refused(self):
        for altered in (math.nan, math.inf, True, "400.0"):
            mutant = copy.deepcopy(self.plan)
            mutant["samples"][17]["x_cm"] = altered
            with self.subTest(altered=str(altered)), self.assertRaises(ValueError):
                validate_plan(mutant)

    def test_duplicate_unknown_or_missing_samples_refused(self):
        for mutation in ("missing", "extra", "schema", "unknown"):
            mutant = copy.deepcopy(self.plan)
            if mutation == "missing": mutant["samples"].pop()
            if mutation == "extra": mutant["samples"].append(mutant["samples"][-1])
            if mutation == "schema": mutant["samples"][3].pop("offset_cm")
            if mutation == "unknown": mutant["unexpected"] = "accepted?"
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                validate_plan(mutant)


if __name__ == "__main__":
    unittest.main()
