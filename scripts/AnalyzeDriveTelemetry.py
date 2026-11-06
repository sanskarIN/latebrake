#!/usr/bin/env python3
"""Offline inspection of non-shipping LateBrake drive-force diagnostic lines.

This reads local text only; it never certifies that a log came from a
genuine Unreal build, measures physics, or makes a playable release claim.
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
MARKER = "LATEBRAKE_DRIVE v=1 "
NUMBER = r"(-?(?:[0-9]+)(?:\.[0-9]+)?)"
FIELDS = (
    "forward_cmps", "lateral_cmps", "yaw_radps", "throttle", "steer",
    "contact", "rear", "front", "motor_force", "opposing_force",
    "service_force", "lateral_force", "yaw_torque", "draft_pct",
)
RECORD = re.compile(
    r"^" + re.escape(MARKER) +
    r"driver=(player|autonomous) mode=(arcade|simulation) " +
    " ".join(re.escape(name) + "=" + NUMBER for name in FIELDS) + r"$"
)


class DriveTelemetryError(ValueError):
    """An untrusted handling observation is malformed or inconsistent."""


def parse_record(record: str) -> dict[str, object]:
    match = RECORD.fullmatch(record)
    if match is None:
        raise DriveTelemetryError("Malformed LATEBRAKE_DRIVE v=1 record")
    driver, mode, *numeric = match.groups()
    values = dict(zip(FIELDS, (float(value) for value in numeric)))
    for name, value in values.items():
        if not math.isfinite(value):
            raise DriveTelemetryError(f"Nonfinite {name}")
    for name in ("forward_cmps", "lateral_cmps"):
        if abs(values[name]) > 15000.1:
            raise DriveTelemetryError(f"{name} exceeds validated chassis speed")
    if abs(values["yaw_radps"]) > 12.01:
        raise DriveTelemetryError("yaw rate exceeds controller validation")
    for name in ("throttle", "steer"):
        if abs(values[name]) > 1.002:
            raise DriveTelemetryError(f"{name} exceeds bounded input")
    for name in ("contact", "rear", "front"):
        if not -0.0005 <= values[name] <= 1.001:
            raise DriveTelemetryError(f"{name} exceeds bounded traction")
    if (values["rear"] > values["contact"] + 0.002 or
            values["front"] > values["contact"] + 0.002):
        raise DriveTelemetryError("axle authority exceeds whole-car traction")
    if not -0.001 <= values["draft_pct"] <= 6.002:
        raise DriveTelemetryError("draft bonus exceeds motor authorization")
    for name in ("motor_force", "opposing_force", "service_force",
                 "lateral_force"):
        if abs(values[name]) > 1.0e8:
            raise DriveTelemetryError(f"{name} is implausibly large")
    if abs(values["yaw_torque"]) > 2.0e9:
        raise DriveTelemetryError("yaw torque is implausibly large")
    # These are mutually exclusive dispatch branches, not simultaneous
    # longitudinal force contributions.
    commanded = ("motor_force", "opposing_force", "service_force")
    if sum(abs(values[name]) > 0.11 for name in commanded) > 1:
        raise DriveTelemetryError("simultaneous exclusive drive/brake paths")
    if values["rear"] < 0.0005 and (
            abs(values["motor_force"]) > 0.11 or
            abs(values["opposing_force"]) > 0.11):
        raise DriveTelemetryError("unloaded rear axle applied drive force")
    if values["front"] < 0.0005 and abs(values["yaw_torque"]) > 0.11:
        raise DriveTelemetryError("unloaded front axle applied yaw torque")
    if values["contact"] < 0.0005 and (
            abs(values["service_force"]) > 0.11 or
            abs(values["lateral_force"]) > 0.11):
        raise DriveTelemetryError("unloaded chassis applied road grip")
    if (values["motor_force"] > 0.11 and values["throttle"] < -0.001 or
            values["motor_force"] < -0.11 and values["throttle"] > 0.001):
        raise DriveTelemetryError("motor force contradicts signed throttle")
    if values["opposing_force"] * values["forward_cmps"] > 1.0:
        raise DriveTelemetryError("opposite-pedal force accelerates current travel")
    if values["service_force"] * values["forward_cmps"] > 1.0:
        raise DriveTelemetryError("service brake accelerates current travel")
    if values["lateral_force"] * values["lateral_cmps"] > 1.0:
        raise DriveTelemetryError("lateral correction accelerates slip")
    return {"driver": driver, "mode": mode, **values}


def summarize_log(text: str) -> dict[str, object]:
    if "\x00" in text:
        raise DriveTelemetryError("Binary/NUL input is not a text log")
    maximums = {
        "abs_forward_cmps": 0.0, "abs_lateral_cmps": 0.0,
        "abs_yaw_radps": 0.0, "abs_motor_force": 0.0,
        "abs_service_force": 0.0, "abs_opposing_force": 0.0,
        "abs_lateral_force": 0.0, "abs_yaw_torque": 0.0,
        "draft_pct": 0.0,
    }
    drivers = {"player": 0, "autonomous": 0}
    modes = {"arcade": 0, "simulation": 0}
    flags = {
        "zero_contact_samples": 0, "rear_unloaded_samples": 0,
        "front_unloaded_samples": 0, "opposite_pedal_braking_samples": 0,
        "service_braking_samples": 0, "positive_draft_samples": 0,
    }
    samples = 0
    for lineno, line in enumerate(text.splitlines(), 1):
        if len(line) > MAX_LINE_CHARS:
            raise DriveTelemetryError(f"Overlong log line {lineno}")
        if MARKER not in line:
            continue
        if line.count(MARKER) != 1:
            raise DriveTelemetryError("Repeated drive marker")
        observation = parse_record(line[line.index(MARKER):])
        samples += 1
        drivers[str(observation["driver"])] += 1
        modes[str(observation["mode"])] += 1
        for key in maximums:
            field = key[4:] if key.startswith("abs_") else key
            value = float(observation[field])
            maximums[key] = max(maximums[key],
                                abs(value) if key.startswith("abs_") else value)
        for flag, key in (
            ("zero_contact_samples", "contact"),
            ("rear_unloaded_samples", "rear"),
            ("front_unloaded_samples", "front"),
        ):
            if float(observation[key]) < 0.0005:
                flags[flag] += 1
        for flag, key in (
            ("opposite_pedal_braking_samples", "opposing_force"),
            ("service_braking_samples", "service_force"),
            ("positive_draft_samples", "draft_pct"),
        ):
            threshold = 0.001 if key == "draft_pct" else 0.11
            if abs(float(observation[key])) > threshold:
                flags[flag] += 1
    if samples == 0:
        raise DriveTelemetryError("No LATEBRAKE_DRIVE v=1 records")
    return {
        "schema_version": 1,
        "status": "UNVERIFIED_OFFLINE_DRIVE_DIAGNOSTIC",
        "gameplay_verified": False,
        "ready_for_release": False,
        "samples": samples,
        "drivers": drivers,
        "modes": modes,
        "maximums": maximums,
        "flagged_samples": flags,
        "limitations": (
            "Editable local text is not proof of a genuine Unreal build or "
            "a cooked installed game. The at-most-5-Hz sampling may miss "
            "brief events. Logged forces are planned force submissions, "
            "not measured Chaos acceleration, grip or steering fidelity. "
            "A real UE5.8 Win64 installed driving session remains required."
        ),
    }


def main() -> int:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--log", type=Path, required=True)
    args = cli.parse_args()
    try:
        if (not args.log.is_file() or args.log.is_symlink() or
                args.log.stat().st_size > MAX_LOG_BYTES):
            raise DriveTelemetryError("Log missing, symlinked or exceeds 32 MiB")
        summary = summarize_log(args.log.read_text(encoding="utf-8-sig"))
    except (DriveTelemetryError, OSError, UnicodeError) as exc:
        print(f"Drive audit rejected: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(summary, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
