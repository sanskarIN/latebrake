"""Mutation tests for the independent physical-road JSON contract.

No Unreal Python import or licensed engine is needed for these fixtures.
"""
import copy
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Standalone"))
from VerifyRoundedCircuitLabExport import (
    EXPECTED_PIECES, expected_piece, validate_document, parse_and_validate
)


def valid_document():
    return {
        "schema_version": 1, "units": "cm",
        "road_top_cm": -1.0, "piece_count": EXPECTED_PIECES,
        "pieces": [expected_piece(i) for i in range(EXPECTED_PIECES)]
    }


class RoundedCircuitExporterContractTests(unittest.TestCase):
    def test_known_canonical_roundtrip_is_complete(self):
        document = valid_document()
        self.assertEqual(validate_document(document), EXPECTED_PIECES)
        self.assertEqual(parse_and_validate(json.dumps(document)), EXPECTED_PIECES)
        self.assertEqual(document["pieces"][0]["rotation_yaw"], 0.0)
        self.assertEqual(document["pieces"][1]["rotation_yaw"], 90.0)
        self.assertAlmostEqual(document["pieces"][4]["rotation_yaw"], 2.8125)
        self.assertAlmostEqual(document["pieces"][-1]["rotation_yaw"], 357.1875)

    def test_corrupt_but_well_formed_road_is_rejected(self):
        cases = (
            ("shift-road", lambda d: d["pieces"][15]["position"].__setitem__(0, 5.0)),
            ("rotate-corner", lambda d: d["pieces"][16].__setitem__("rotation_yaw", 12.0)),
            ("change-width", lambda d: d["pieces"][8]["scale"].__setitem__(1, 9.01)),
            ("change-height", lambda d: d["pieces"][0]["position"].__setitem__(2, -21.0)),
            ("change-length", lambda d: d["pieces"][3]["scale"].__setitem__(0, 25.0)),
            ("swap-two", lambda d: d["pieces"].__setitem__(7, d["pieces"][8])),
            ("change-units", lambda d: d.__setitem__("units", "m")),
            ("wrong-road-top", lambda d: d.__setitem__("road_top_cm", 0.0)),
            ("extra-key", lambda d: d["pieces"][2].__setitem__("fake_visual_road", True)),
            ("boolean-index", lambda d: d["pieces"][9].__setitem__("index", True)),
            ("boolean-count", lambda d: d.__setitem__("piece_count", True)),
            ("nan-position", lambda d: d["pieces"][11]["position"].__setitem__(0, float("nan"))),
            ("infinite-yaw", lambda d: d["pieces"][25].__setitem__("rotation_yaw", float("inf"))),
            ("negative-scale", lambda d: d["pieces"][13]["scale"].__setitem__(0, -1.0)),
            ("missing-piece", lambda d: d["pieces"].pop()),
        )
        for name, mutation in cases:
            with self.subTest(name=name):
                document = copy.deepcopy(valid_document())
                mutation(document)
                with self.assertRaises(ValueError):
                    validate_document(document)

    def test_invalid_json_tokens_and_double_documents_rejected(self):
        for source in (
            '{"schema_version":NaN}', '{"schema_version":Infinity}',
            json.dumps(valid_document()) + json.dumps(valid_document()),
            "garbage",
        ):
            with self.subTest(source=source[:40]):
                with self.assertRaises(ValueError):
                    parse_and_validate(source)


if __name__ == "__main__":
    unittest.main()
