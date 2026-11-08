"""Independent, stdlib-only audit of the ACTUAL compiled C++ road JSON.

The CTest for RoundedCircuitLabExport alone only checks the process exit code.
This second test checks every number in the emitted 68-piece file against an
independent circle/straight computation, not another direct call to the C++
header. It does not execute Unreal or certify wheel/road contacts.
"""

import json
import math
from pathlib import Path
import subprocess
import sys

EXPECTED_PIECES = 68
RADIUS_CM = 800.0
HALF_WIDTH_CM = 450.0
CORNER_SECTIONS = 16
HALF_PI = math.pi / 2.0
ROAD_TOP_CM = -1.0
ROAD_CENTER_Z_CM = -22.0
ROAD_DEPTH_SCALE = 0.42
EAST_X_CM = 2800.0
NORTH_Y_CM = 1800.0
TANGENT_X_CM = 2000.0
TANGENT_Y_CM = 1000.0
STRAIGHT_OVERLAP_CM = 60.0
ARC_OVERLAP_CM = 20.0
FLOAT_TOL = 0.000002


def expected_piece(index, scale_factor=1.0):
    if not isinstance(index, int) or isinstance(index, bool) or not 0 <= index < EXPECTED_PIECES:
        raise ValueError("invalid canonical physical-road piece index")
    width = 2.0 * HALF_WIDTH_CM / 100.0
    if index < 4:
        horizontal = (2.0 * TANGENT_X_CM + 2.0 * STRAIGHT_OVERLAP_CM) / 100.0
        vertical = (2.0 * TANGENT_Y_CM + 2.0 * STRAIGHT_OVERLAP_CM) / 100.0
        positions = ((0.0, -NORTH_Y_CM),
                     (EAST_X_CM, 0.0),
                     (0.0, NORTH_Y_CM),
                     (-EAST_X_CM, 0.0))
        x, y = positions[index]
        yaw = index * 90.0
        length = horizontal if index % 2 == 0 else vertical
    else:
        bend, segment = divmod(index - 4, CORNER_SECTIONS)
        centers = ((TANGENT_X_CM, -TANGENT_Y_CM),
                   (TANGENT_X_CM, TANGENT_Y_CM),
                   (-TANGENT_X_CM, TANGENT_Y_CM),
                   (-TANGENT_X_CM, -TANGENT_Y_CM))
        cx, cy = centers[bend]
        step = HALF_PI / CORNER_SECTIONS
        angle = -HALF_PI + bend * HALF_PI + (segment + 0.5) * step
        x = cx + RADIUS_CM * math.cos(angle)
        y = cy + RADIUS_CM * math.sin(angle)
        yaw = math.degrees(angle + HALF_PI)
        # The oriented slab extends beyond its chord to prevent gaps.
        half_step = step * 0.5
        length = 2.0 * ((RADIUS_CM + HALF_WIDTH_CM) * math.sin(half_step)
                        + ARC_OVERLAP_CM) / 100.0
    return {
        "index": index,
        "position": [x * scale_factor, y * scale_factor, ROAD_CENTER_Z_CM],
        "rotation_yaw": yaw,
        "scale": [length * scale_factor, width, ROAD_DEPTH_SCALE],
    }


def numeric(value):
    return type(value) in (int, float) and math.isfinite(value)


def close(actual, expected, where):
    if not numeric(actual) or not math.isclose(
        actual, expected, rel_tol=0.0, abs_tol=FLOAT_TOL
    ):
        raise ValueError(f"Incorrect physical collision geometry at {where}: {actual!r}")


def validate_document(document, scale_factor=1.0):
    if scale_factor not in (1.0, 10.0):
        raise ValueError("Unsupported rounded road profile factor")
    if not isinstance(document, dict) or set(document) != {
        "schema_version", "units", "road_top_cm", "piece_count", "pieces"
    }:
        raise ValueError("Unexpected canonical rounded-road JSON schema")
    if type(document["schema_version"]) is not int or document["schema_version"] != 1:
        raise ValueError("Wrong canonical rounded road schema version")
    if document["units"] != "cm":
        raise ValueError("Unreal collision-road positions must use centimetres")
    if type(document["piece_count"]) is not int or document["piece_count"] != EXPECTED_PIECES:
        raise ValueError("Wrong number of physical rounded road slabs")
    close(document["road_top_cm"], ROAD_TOP_CM, "asphalt top")
    pieces = document["pieces"]
    if not isinstance(pieces, list) or len(pieces) != EXPECTED_PIECES:
        raise ValueError("Missing, duplicated or overbudget physical road pieces")
    for i, item in enumerate(pieces):
        expect = expected_piece(i, scale_factor)
        if not isinstance(item, dict) or set(item) != set(expect):
            raise ValueError(f"Unexpected road piece structure {i}")
        if type(item["index"]) is not int or item["index"] != i:
            raise ValueError(f"Missing/reordered/duplicate physical road index {i}")
        if (not isinstance(item["position"], list) or
            not isinstance(item["scale"], list) or
            len(item["position"]) != 3 or len(item["scale"]) != 3):
            raise ValueError(f"Invalid physical road dimensions {i}")
        for field in ("position", "scale"):
            for axis, (actual, baseline) in enumerate(zip(item[field], expect[field])):
                close(actual, baseline, f"piece {i} {field}[{axis}]")
        close(item["rotation_yaw"], expect["rotation_yaw"], f"piece {i} yaw")
        close(item["position"][2] + 50.0 * item["scale"][2],
              ROAD_TOP_CM, f"piece {i} physical asphalt top")
    return EXPECTED_PIECES


def _reject_nonstandard_float(word):
    raise ValueError(f"Non-JSON floating-point constant: {word}")


def parse_and_validate(raw, scale_factor=1.0):
    data = json.loads(raw, parse_constant=_reject_nonstandard_float)
    return validate_document(data, scale_factor)


def check_exporter(executable, scale_factor=1.0):
    completed = subprocess.run([str(Path(executable))], capture_output=True,
                               text=True, check=True, timeout=15)
    if completed.stderr:
        raise ValueError("C++ canonical road exporter emitted unexpected diagnostics")
    return parse_and_validate(completed.stdout, scale_factor)


def main():
    if len(sys.argv) not in (2, 3) or (len(sys.argv) == 3 and sys.argv[2] != "10"):
        raise SystemExit("Usage: VerifyRoundedCircuitLabExport.py <compiled-exporter> [10]")
    scale_factor = 10.0 if len(sys.argv) == 3 else 1.0
    count = check_exporter(sys.argv[1], scale_factor)
    print(f"Verified {count} compiled C++ rounded road transforms, XY/length factor={scale_factor}, width=9 m")


if __name__ == "__main__":
    main()
