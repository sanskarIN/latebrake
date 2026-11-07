"""Deterministic UE Editor ray-query QA plan for the ORIGINAL 1.70-km road.

NO UE import/cook, playable game, dynamic tire contact, or rendering is
performed by this source-only generator. Source coordinates are reconstructed
from the independent Python 1.70-km centreline used to verify the C++ OBJ.
"""
import json
import math
from pathlib import Path
import sys

from VerifyRoundedSmoothRoadObj import centerline_at, LAP

STATIONS = 256
EXTRA_SEAM_DISTANCES_CM = (0.25, LAP * 10.0 - 0.25)
# 9-m road is 450 cm from centre; the cosmetic ribbon only reaches 425 cm.
# The 550-cm probes must be off the physical mesh and hit lab ground instead.
OFFSETS_CM = (0, -400, 400, -550, 550)
ROAD_TOP_CM = -1.0
GROUND_TOP_CM = -37.5
HEIGHT_START_CM = 250.0
HEIGHT_END_CM = -200.0
EXPECTED_SAMPLES = (STATIONS + len(EXTRA_SEAM_DISTANCES_CM)) * len(OFFSETS_CM)


def _stations():
    for index in range(STATIONS):
        yield index, LAP * 10.0 * index / STATIONS
    for offset, distance in enumerate(EXTRA_SEAM_DISTANCES_CM):
        yield STATIONS + offset, distance


def make_plan():
    samples = []
    for index, distance in _stations():
        x, y, degrees = centerline_at(distance, world_scale=10.0)
        yaw = math.radians(degrees)
        nx, ny = -math.sin(yaw), math.cos(yaw)
        for lateral_cm in OFFSETS_CM:
            kind = "road" if abs(lateral_cm) <= 400 else "ground"
            sample = {
                "station": index,
                "distance_cm": distance,
                "offset_cm": lateral_cm,
                "x_cm": x + nx * lateral_cm,
                "y_cm": y + ny * lateral_cm,
                "expected_actor": kind,
                "expected_z_cm": ROAD_TOP_CM if kind == "road" else GROUND_TOP_CM,
            }
            if any(not math.isfinite(v) for k, v in sample.items()
                   if k in ("distance_cm", "x_cm", "y_cm", "expected_z_cm")):
                raise ValueError("Nonfinite 1.70-km road query station")
            samples.append(sample)
    assert len(samples) == EXPECTED_SAMPLES
    return {
        "schema_version": 1,
        "units": "cm",
        "circuit": "LatebrakeWorldScaleContinuousCollisionLab",
        "station_count": STATIONS,
        "extra_seam_stations": len(EXTRA_SEAM_DISTANCES_CM),
        "physical_road_width_cm": 900,
        "trace_start_z_cm": HEIGHT_START_CM,
        "trace_end_z_cm": HEIGHT_END_CM,
        "sample_count": EXPECTED_SAMPLES,
        "samples": samples,
    }


def validate_plan(data):
    """Verify each probe against independently regenerated source coordinates.

    The SHA256 digest passed by the wrapper protects file identity across the
    process boundary; this numeric contract independently checks each field.
    A plausible but tampered 9-m corridor cannot be signed as passing.
    """
    expected = make_plan()
    if not isinstance(data, dict) or set(data) != set(expected):
        raise ValueError("Unrecognized 1.70-km query plan schema")
    for key in expected.keys() - {"samples"}:
        if type(data[key]) is not type(expected[key]) or data[key] != expected[key]:
            raise ValueError(f"Invalid road trace plan metadata: {key}")
    samples = data["samples"]
    if not isinstance(samples, list) or len(samples) != EXPECTED_SAMPLES:
        raise ValueError("Missing or extra continuous-road probe points")
    for index, (actual, reference) in enumerate(zip(samples, expected["samples"])):
        if not isinstance(actual, dict) or set(actual) != set(reference):
            raise ValueError(f"Invalid source road trace probe schema {index}")
        for name, value in reference.items():
            got = actual[name]
            if name in ("distance_cm", "x_cm", "y_cm", "expected_z_cm"):
                if type(got) not in (float, int) or not math.isfinite(got):
                    raise ValueError(f"Nonfinite/non-numeric continuous probe {index}.{name}")
                if abs(got - value) > 1e-5:
                    raise ValueError(f"Displaced continuous collision probe {index}.{name}")
            elif type(got) is not type(value) or got != value:
                raise ValueError(f"Invalid continuous collision probe {index}.{name}")
    return len(samples)


def save_plan(destination):
    if destination.suffix.lower() != ".json":
        raise ValueError("Source probe plan must be a JSON document")
    plan = make_plan()
    validate_plan(plan)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(
        json.dumps(plan, sort_keys=True, separators=(",", ":"), allow_nan=False) + "\n",
        encoding="utf-8",
    )
    validate_plan(json.loads(destination.read_text(encoding="utf-8")))
    return destination


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: WorldScaleCollisionTracePlan.py <destination.json>")
    output = save_plan(Path(sys.argv[1]))
    print(f"Source-only 1.70-km collision-query plan: {EXPECTED_SAMPLES} "
          f"road/ground static ray probes, separate lap seam, NOT Chaos tire QA: {output}")


if __name__ == "__main__":
    main()
