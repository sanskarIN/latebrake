#!/usr/bin/env python3
"""Offline, unverified inspection of real UE Development suspension logs.

This is NOT an Unreal Engine runtime, physics simulation, game-playability
certifier, or release gate. It never launches a process, accesses the web,
imports Unreal, or alters a game archive. A forged or edited text file could
produce indistinguishable output.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import re
import sys

MAX_LOG_BYTES = 32 * 1024 * 1024
MAX_LINE_CHARS = 8192
MARKER = "LATEBRAKE_SUSPENSION v=1 "
NUMBER = r"(-?(?:[0-9]+)(?:\.[0-9]+)?)"
RECORD = re.compile(
    r"^" + re.escape(MARKER) +
    r"driver=(player|autonomous) phase=(grid|go) "
    r"contacts=([0-4]) weight_ratio=" + NUMBER +
    r" traction=" + NUMBER +
    r" drive_rear=" + NUMBER + r" steer_front=" + NUMBER +
    r" front_load=" + NUMBER + r" rear_load=" + NUMBER +
    r" arb_front_pct=" + NUMBER + r" arb_rear_pct=" + NUMBER +
    r" body_z_cm=" + NUMBER + r" vz_cmps=" + NUMBER +
    r" pitch_deg=" + NUMBER + r" roll_deg=" + NUMBER +
    r" fl_cm=" + NUMBER + r" fr_cm=" + NUMBER +
    r" rl_cm=" + NUMBER + r" rr_cm=" + NUMBER + r"$"
)


class SuspensionAuditError(ValueError):
    """Untrusted or inconsistent suspension trace input."""


def finite_fraction(value: float, label: str, upper: float = 1.001) -> None:
    if not math.isfinite(value) or not 0.0 <= value <= upper:
        raise SuspensionAuditError(f"{label} outside bounded physical range")


def parse_record(source: str) -> dict[str, object]:
    match = RECORD.fullmatch(source)
    if match is None:
        raise SuspensionAuditError("Malformed LATEBRAKE_SUSPENSION v=1 record")
    values = match.groups()
    driver, phase, contacts_text = values[:3]
    fields = (
        "weight_ratio", "traction", "drive_rear", "steer_front",
        "front_load", "rear_load", "arb_front_pct", "arb_rear_pct",
        "body_z_cm", "vz_cmps", "pitch_deg", "roll_deg",
        "fl_cm", "fr_cm", "rl_cm", "rr_cm",
    )
    numbers = dict(zip(fields, (float(value) for value in values[3:])))
    for name, number in numbers.items():
        if not math.isfinite(number):
            raise SuspensionAuditError(f"Nonfinite {name}")
    contacts = int(contacts_text)
    finite_fraction(numbers["weight_ratio"], "weight_ratio", 3.405)
    for name in ("traction", "drive_rear", "steer_front",
                 "front_load", "rear_load"):
        finite_fraction(numbers[name], name)
    for name in ("arb_front_pct", "arb_rear_pct"):
        if abs(numbers[name]) > 8.02:
            raise SuspensionAuditError(f"{name} exceeded axle transfer ceiling")
    for name in ("fl_cm", "fr_cm", "rl_cm", "rr_cm"):
        distance = numbers[name]
        if distance != -1.0 and not 0.0 <= distance <= 280.1:
            raise SuspensionAuditError(f"{name} outside supported road trace")
    if (numbers["drive_rear"] > numbers["traction"] + 0.002 or
            numbers["steer_front"] > numbers["traction"] + 0.002):
        raise SuspensionAuditError("axle grip exceeds verified whole-car limit")
    if (numbers["rear_load"] < 0.001 and numbers["drive_rear"] > 0.0005):
        raise SuspensionAuditError("unloaded rear axle reported active drive grip")
    if (numbers["front_load"] < 0.001 and numbers["steer_front"] > 0.0005):
        raise SuspensionAuditError("unloaded front axle reported active steering")
    if contacts < 2 and (
        numbers["traction"] > 0.0005 or
        numbers["drive_rear"] > 0.0005 or
        numbers["steer_front"] > 0.0005
    ):
        raise SuspensionAuditError("single wheel granted playable traction")
    if contacts == 0 and numbers["weight_ratio"] > 0.002:
        raise SuspensionAuditError("airborne chassis reported weight-bearing force")
    if abs(numbers["vz_cmps"]) > 15000.2:
        raise SuspensionAuditError("reported velocity exceeded physics sample gate")
    return {"driver": driver, "phase": phase, "contacts": contacts, **numbers}


def summarize_log(text: str) -> dict[str, object]:
    if "\x00" in text:
        raise SuspensionAuditError("Binary/NUL input is not a text log")
    samples = 0
    drivers = {"player": 0, "autonomous": 0}
    phases = {"grid": 0, "go": 0}
    contacts = {str(n): 0 for n in range(5)}
    flags = {
        "grid_settle_concerns": 0,
        "large_body_pitch_or_roll": 0,
        "hard_vertical_motion_with_road_contact": 0,
        "airborne_during_go": 0,
        "rear_only_support": 0,
        "front_only_support": 0,
    }
    maximums = {
        "abs_pitch_deg": 0.0,
        "abs_roll_deg": 0.0,
        "abs_vertical_speed_cmps": 0.0,
        "spring_weight_ratio": 0.0,
        "rear_drive_scale": 0.0,
        "front_steering_scale": 0.0,
    }
    for line_number, line in enumerate(text.splitlines(), 1):
        if len(line) > MAX_LINE_CHARS:
            raise SuspensionAuditError(
                f"Overlong log line {line_number} (bounded input required)")
        if MARKER not in line:
            continue
        if line.count(MARKER) != 1:
            raise SuspensionAuditError("Repeated suspension marker on one log line")
        # Unreal standard prefixes are preserved but never treated as proof
        # the line actually came from a genuine Unreal session.
        start = line.index(MARKER)
        record = parse_record(line[start:])
        samples += 1
        driver = str(record["driver"])
        phase = str(record["phase"])
        count = int(record["contacts"])
        drivers[driver] += 1
        phases[phase] += 1
        contacts[str(count)] += 1
        pitch = abs(float(record["pitch_deg"]))
        roll = abs(float(record["roll_deg"]))
        speed = abs(float(record["vz_cmps"]))
        maximums["abs_pitch_deg"] = max(maximums["abs_pitch_deg"], pitch)
        maximums["abs_roll_deg"] = max(maximums["abs_roll_deg"], roll)
        maximums["abs_vertical_speed_cmps"] = max(
            maximums["abs_vertical_speed_cmps"], speed)
        maximums["spring_weight_ratio"] = max(
            maximums["spring_weight_ratio"], float(record["weight_ratio"]))
        maximums["rear_drive_scale"] = max(
            maximums["rear_drive_scale"], float(record["drive_rear"]))
        maximums["front_steering_scale"] = max(
            maximums["front_steering_scale"], float(record["steer_front"]))
        if phase == "grid" and (speed > 50.0 or pitch > 10.0 or roll > 10.0):
            flags["grid_settle_concerns"] += 1
        if pitch > 20.0 or roll > 20.0:
            flags["large_body_pitch_or_roll"] += 1
        if count > 0 and speed > 350.0:
            flags["hard_vertical_motion_with_road_contact"] += 1
        if phase == "go" and count == 0:
            flags["airborne_during_go"] += 1
        if (float(record["front_load"]) < 0.001 and
                float(record["rear_load"]) >= 0.001):
            flags["rear_only_support"] += 1
        if (float(record["rear_load"]) < 0.001 and
                float(record["front_load"]) >= 0.001):
            flags["front_only_support"] += 1
    if samples == 0:
        raise SuspensionAuditError(
            "No genuine-format suspension records; cannot assess physics")
    return {
        "schema_version": 1,
        "status": "UNVERIFIED_OFFLINE_SUSPENSION_DIAGNOSTIC",
        "gameplay_verified": False,
        "ready_for_release": False,
        "samples": samples,
        "drivers": drivers,
        "phases": phases,
        "contact_counts": contacts,
        "maximums": maximums,
        "flagged_samples": flags,
        "limitations": (
            "Editable local log text; source cadence limits event records to "
            "one per 0.2 seconds per vehicle and emits a 2-second heartbeat. "
            "Brief transitions can be missed. Logs cannot prove authentic "
            "source, Chaos solver, frame rate, lap completion, road clearance "
            "or real player experience. "
            "Requires UE5.8 hardware gameplay plus build/manifest verification."
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", type=Path, required=True,
                        help="Unreal Development game log; not a release certificate")
    args = parser.parse_args()
    try:
        if (not args.log.is_file() or args.log.is_symlink() or
                args.log.stat().st_size > MAX_LOG_BYTES):
            raise SuspensionAuditError("Log missing, symlinked or exceeds 32 MiB")
        content = args.log.read_text(encoding="utf-8-sig")
        result = summarize_log(content)
    except (OSError, UnicodeError, SuspensionAuditError) as exc:
        print(f"Suspension audit rejected: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(result, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
