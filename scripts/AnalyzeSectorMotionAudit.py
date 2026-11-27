#!/usr/bin/env python3
"""Cross-check non-shipping sector motion observations with ordered race receipts.

Local log records are unauthenticated. This tool checks consistency, not
physical gameplay, executable origin, performance, or release readiness.
"""
import argparse
import hashlib
import json
import re
import sys
from decimal import Decimal
from pathlib import Path

from AnalyzeRaceAudit import (
    BEGIN_PATTERN,
    GATE_PATTERN,
    LINE_PATTERN,
    LOG_MARKER,
    MAX_LOG_BYTES,
    RaceAuditError,
    analyze_race_audit,
)

MARKER = "LATEBRAKE_SECTOR_AUDIT"
MAX_EVENTS = 100_000
MAX_LINE = 4096
MAX_COUNTER = (1 << 64) - 1
MAX_CLOCK = Decimal("604800")
MAX_SPEED = Decimal("15000")
MAX_RESIDUAL = Decimal("175")
EPS = Decimal("0.000002")
CLOCK_ROUND = Decimal("0.000001")
# All numeric fields use fixed decimal formatting in the native adapter.
# Bounds on the grammar avoid conversion of unbounded integer strings.
INT = r"(?:0|[1-9][0-9]{0,19})"
REAL = r"(?:0|[1-9][0-9]{0,16})\.[0-9]{9}"
HEAD = re.compile(
    rf"^.*?\b{MARKER} v=1 attempt=(?P<attempt>{INT}) "
    rf"ordinal=(?P<ordinal>{INT}) gate=(?P<gate>{INT}) "
    rf"elapsed=(?P<elapsed>{REAL}) penalty=(?P<penalty>{REAL}) "
    r"available=(?P<available>[01])(?P<rest>[^\r\n]*)$"
)
TIME_FIELDS = ("active", "fee", "official", "observed", "coverage")
COUNTER_FIELDS = ("samples", "intervals", "breaks", "rejected", "recovery", "jump")
MOTION_FIELDS = (
    "distance_cm", "speed_integral", "peak", "lateral_integral",
    "low", "slip", "reverse", "partial", "off",
)
DETAIL = re.compile(
    r" quality=(?P<quality>[1-4]) "
    + " ".join(
        [rf"{k}=(?P<{k}>{REAL})" for k in TIME_FIELDS]
        + [rf"{k}=(?P<{k}>{INT})" for k in COUNTER_FIELDS]
        + [rf"{k}=(?P<{k}>{REAL})" for k in MOTION_FIELDS]
    )
)
QUALITIES = {
    1: "insufficient_coverage",
    2: "interrupted",
    3: "penalized",
    4: "qualified",
}


class SectorAuditError(ValueError):
    """Malformed, incomplete, or contradictory sector observation stream."""


def require(condition, message):
    if not condition:
        raise SectorAuditError(message)


def integer(match, field, maximum=MAX_COUNTER):
    value = int(match[field])
    require(0 <= value <= maximum, f"{field} outside bound")
    return value


def decimal(match, field, maximum):
    value = Decimal(match[field])
    require(0 <= value <= maximum, f"{field} outside bound")
    return value


def same(a, b, tolerance=EPS):
    return abs(a - b) <= tolerance


def validate_quality(quality, active, fee, coverage, counters):
    # The native threshold is 1e-6. Fixed decimal logging can straddle
    # a threshold by half a last digit. Reject clear contradictions only.
    threshold = Decimal("0.000001")
    rounding = Decimal("0.000000001")
    interrupted = counters["breaks"] > 0 or counters["rejected"] > 0
    enough = (
        active >= Decimal("0.5") - rounding
        and counters["intervals"] > 0
        and coverage >= Decimal("0.85") - rounding
    )
    clearly_enough = (
        active > Decimal("0.5") + rounding
        and counters["intervals"] > 0
        and coverage > Decimal("0.85") + rounding
    )
    if quality == 3:
        require(fee >= threshold - rounding, "penalized sector lacks fee")
    else:
        require(fee <= threshold + rounding, "fee omitted from sector quality")
        if quality == 2:
            require(interrupted, "interrupted sector lacks break or rejection")
        elif quality == 4:
            require(not interrupted and enough, "unqualified motion reported qualified")
        elif quality == 1:
            require(not interrupted and not clearly_enough,
                    "insufficient coverage quality contradicts counters")


def parse_observation(found, pending, prior_elapsed, prior_penalty):
    attempt = integer(found, "attempt", MAX_EVENTS)
    ordinal = integer(found, "ordinal", 25_600)
    gate = integer(found, "gate", 255)
    require((attempt, ordinal, gate) == pending[:3], "sector does not match accepted gate")
    elapsed = decimal(found, "elapsed", MAX_CLOCK)
    penalty = decimal(found, "penalty", MAX_CLOCK)
    require(same(elapsed, pending[3], CLOCK_ROUND),
            "sector elapsed clock differs from race receipt")
    require(elapsed >= prior_elapsed and penalty >= prior_penalty and penalty <= elapsed,
            "sector cumulative clock or penalty invalid")
    active = elapsed - penalty - prior_elapsed + prior_penalty
    fee = penalty - prior_penalty
    require(active >= -EPS, "sector active clock moved backwards")
    row = {
        "ordinal": ordinal,
        "gate": gate,
        "elapsed_seconds": str(elapsed),
        "penalty_seconds": str(penalty),
        "standing_start": ordinal == 1,
        "available": found["available"] == "1",
    }
    if not row["available"]:
        require(found["rest"] == "", "unavailable sector has invented observations")
        row["quality"] = "unavailable"
        return row, elapsed, penalty

    detail = DETAIL.fullmatch(found["rest"])
    require(detail is not None, "malformed or extra sector motion fields")
    quality = int(detail["quality"])
    times = {
        k: decimal(detail, k, Decimal(1) if k == "coverage" else MAX_CLOCK)
        for k in TIME_FIELDS
    }
    counters = {k: integer(detail, k) for k in COUNTER_FIELDS}
    motion = {
        k: decimal(detail, k, MAX_SPEED if k == "peak" else (
            MAX_CLOCK if k in ("low", "slip", "reverse", "partial", "off")
            else Decimal("99999999999999999")
        ))
        for k in MOTION_FIELDS
    }
    observed = times["observed"]
    require(times["active"] > 0 and times["official"] > 0,
            "available sector has nonpositive clock")
    require(same(times["active"], active) and same(times["fee"], fee),
            "sector active time or fee does not reconcile cumulative clocks")
    require(same(times["official"], times["active"] + times["fee"]),
            "official split does not reconcile active time and fee")
    require(observed <= times["active"] + EPS, "observations exceed active time")
    expected_coverage = min(Decimal(1), observed / times["active"])
    # For tiny sectors, duration rounding is amplified by division.
    coverage_tolerance = min(Decimal(1), EPS / times["active"] + EPS)
    require(same(times["coverage"], expected_coverage, coverage_tolerance),
            "coverage does not match observation duration")
    samples, intervals = counters["samples"], counters["intervals"]
    require(intervals <= max(0, samples - 1), "interval count exceeds samples")
    require(counters["recovery"] + counters["jump"] <= counters["breaks"] <= samples,
            "break reason counts are inconsistent")
    require(observed <= Decimal(intervals) * Decimal("0.1") + EPS,
            "observed duration exceeds maximum interval budget")
    require(intervals > 0 or observed == 0, "observation without intervals")
    require(samples > 0 or motion["peak"] == 0, "peak speed without samples")
    integral_bound = motion["peak"] * observed + Decimal("0.03")
    require(motion["speed_integral"] <= integral_bound
            and motion["lateral_integral"] <= motion["speed_integral"] + Decimal("0.03"),
            "speed integral exceeds measured bound")
    distance_bound = MAX_SPEED * observed + MAX_RESIDUAL * intervals + Decimal("0.03")
    require(motion["distance_cm"] <= distance_bound, "distance exceeds continuity bound")
    for field in ("low", "slip", "reverse", "partial", "off"):
        require(motion[field] <= observed + EPS, f"{field} exceeds observed duration")
    require(motion["off"] <= motion["partial"] + EPS, "off-road time exceeds partial-road time")
    require(intervals > 0 or all(motion[k] == 0 for k in MOTION_FIELDS if k != "peak"),
            "motion accumulated without accepted intervals")
    validate_quality(quality, times["active"], times["fee"], times["coverage"], counters)
    row.update({
        "quality": QUALITIES[quality],
        "active_seconds": str(times["active"]),
        "fee_seconds": str(times["fee"]),
        "official_seconds": str(times["official"]),
        "observed_seconds": str(observed),
        "coverage": str(times["coverage"]),
        "counters": counters,
        "distance_meters": str(motion["distance_cm"] / 100),
        "peak_kmh": str(motion["peak"] * Decimal("0.036")),
        "mean_kmh": (str(motion["speed_integral"] / observed * Decimal("0.036"))
                     if observed > 0 else None),
        "mean_abs_lateral_kmh": (str(motion["lateral_integral"] / observed * Decimal("0.036"))
                                  if observed > 0 else None),
        "low_speed_seconds": str(motion["low"]),
        "high_slip_seconds": str(motion["slip"]),
        "reverse_seconds": str(motion["reverse"]),
        "partial_projected_road_seconds": str(motion["partial"]),
        "off_projected_road_seconds": str(motion["off"]),
    })
    return row, elapsed, penalty


def summarize_attempt(attempt):
    rows = attempt["sectors"]
    counts = {name: 0 for name in (*QUALITIES.values(), "unavailable")}
    gates = {}
    for row in rows:
        counts[row["quality"]] += 1
        # The first launch is not a rolling reference. Do not compare
        # against an unavailable, penalized, interrupted, or sparse row.
        if row["quality"] != "qualified" or row["standing_start"]:
            continue
        key = str(row["gate"])
        gate = gates.setdefault(key, {"qualified_rolling_sectors": 0, "best_ordinal": None,
                                     "best_active_seconds": None})
        gate["qualified_rolling_sectors"] += 1
        current = Decimal(row["active_seconds"])
        best = gate["best_active_seconds"]
        if best is None or current < Decimal(best):
            gate["best_active_seconds"] = str(current)
            gate["best_ordinal"] = row["ordinal"]
    attempt["quality_counts"] = counts
    attempt["rolling_gate_summary"] = gates
    attempt["matched_sector_records"] = len(rows)
    attempt["observed_active_seconds"] = str(sum(
        (Decimal(row["observed_seconds"]) for row in rows if row["available"]),
        Decimal(0),
    ))
    return attempt


def analyze_sector_audit(text, require_finish=False):
    require(isinstance(text, str) and len(text) <= MAX_LOG_BYTES, "log exceeds safety bound")
    try:
        # Independently validate the entire authoritative ordered-gate
        # stream before joining observational records to it.
        race = analyze_race_audit(text, require_finish=require_finish)
    except (RaceAuditError, ValueError) as error:
        raise SectorAuditError(f"base race audit rejected: {error}") from error
    require(MARKER in text, "no sector audit observations")
    attempts = []
    current = None
    pending = None
    count = 0
    ordinal = 0
    prior_elapsed = Decimal(0)
    prior_penalty = Decimal(0)
    for number, line in enumerate(text.splitlines(), 1):
        if LOG_MARKER not in line and MARKER not in line:
            continue
        try:
            require(len(line) <= MAX_LINE, "oversized audit record")
            if LOG_MARKER in line:
                require(line.count(LOG_MARKER) == 1 and MARKER not in line,
                        "ambiguous race audit record")
                require(pending is None, "accepted gate lacks matching sector record")
                base = LINE_PATTERN.fullmatch(line.strip())
                require(base is not None, "malformed race audit record")
                attempt_id = int(base["attempt"])
                if base["action"] == "begin":
                    begin = BEGIN_PATTERN.fullmatch(base["fields"])
                    require(begin is not None, "malformed begin")
                    current = {
                        "attempt": attempt_id,
                        "gates": int(begin["gates"]),
                        "laps_required": int(begin["laps"]),
                        "finished": False,
                        "sectors": [],
                    }
                    attempts.append(current)
                    ordinal = 0
                    prior_elapsed = prior_penalty = Decimal(0)
                else:
                    gate = GATE_PATTERN.fullmatch(base["fields"])
                    require(current is not None and gate is not None, "orphan race gate")
                    ordinal += 1
                    pending = (attempt_id, ordinal, int(gate["gate"]), Decimal(gate["elapsed"]))
                    current["finished"] = gate["finished"] == "1"
                continue

            require(line.count(MARKER) == 1, "ambiguous sector marker")
            found = HEAD.fullmatch(line.strip())
            require(found is not None, "malformed sector marker")
            require(pending is not None and current is not None, "orphan or duplicate sector")
            require(count < MAX_EVENTS, "too many observations")
            row, prior_elapsed, prior_penalty = parse_observation(
                found, pending, prior_elapsed, prior_penalty
            )
            current["sectors"].append(row)
            count += 1
            pending = None
        except SectorAuditError as error:
            raise SectorAuditError(f"line {number}: {error}") from error
    require(pending is None, "log ended before final accepted gate's sector record")
    require(count > 0, "no matched sector records")
    require(count == race["accepted_gate_events"], "incomplete gate observation coverage")
    for attempt in attempts:
        summarize_attempt(attempt)
    complete = [a for a in attempts if a["finished"] and any(
        row["available"] for row in a["sectors"]
    )]
    if require_finish:
        require(complete, "no reported finish with available sector timing")
    return {
        "schema": 1,
        "kind": "latebrake-local-sector-motion-audit",
        "matched_sector_records": count,
        "attempt_count": len(attempts),
        "reported_finished_attempts_with_motion": len(complete),
        "attempts": attempts,
        "source_authenticity_verified": False,
        "unreal_build_verified": False,
        "installed_gameplay_verified": False,
        "ready_for_release": False,
        "warning": (
            "Editable observations only. Projected road geometry is not physical tire contact. "
            "No installed playtest, competition result, or native build is certified."
        ),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--require-finish", action="store_true")
    args = parser.parse_args()
    try:
        require(args.log.is_file() and not args.log.is_symlink(),
                "log must be an existing non-symlink file")
        with args.log.open("rb") as stream:
            data = stream.read(MAX_LOG_BYTES + 1)
        require(len(data) <= MAX_LOG_BYTES, "log exceeds safety bound")
        report = analyze_sector_audit(data.decode("utf-8-sig"), args.require_finish)
        report["log_bytes"] = len(data)
        report["log_sha256"] = hashlib.sha256(data).hexdigest()
    except (OSError, UnicodeError, SectorAuditError) as error:
        print(f"LateBrake sector audit rejected: {error}", file=sys.stderr)
        return 2
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
