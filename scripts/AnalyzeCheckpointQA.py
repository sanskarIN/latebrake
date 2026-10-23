#!/usr/bin/env python3
"""Summarize local LateBrake v0.5.4 checkpoint QA logs.

This tool parses text supplied by a tester. It CANNOT attest log provenance,
prove Unreal gameplay or validate a shipping executable. Test fixtures are
synthetic and must never be passed off as installed-game evidence.
No telemetry, networking, raw location export or third-party dependencies.
"""

import argparse
import json
import re
import sys
from pathlib import Path

LIMIT = (1 << 32) - 1
FIELDS = (
    ("samples", "qa-samples"),
    ("crossed", "crossed"),
    ("teleport_rebase", "teleport-rebase"),
    ("invalid_position", "invalid-position"),
    ("wrong_way", "wrong-way"),
    ("frame_travel", "frame-travel"),
    ("lane_miss", "lane-miss"),
    ("height_miss", "height-miss"),
    ("no_plane", "no-plane"),
    ("invalid_velocity", "invalid-velocity"),
)
# New v0.5.31 Unreal QA logs add an invalid-velocity count. Legacy
# v0.5.4 samples without this trailing field remain valid and report zero
# in that bucket, so historical captured logs can still be inspected.
PATTERN = re.compile(
    r"\bLatebrake checkpoint=(?P<gate>\d+) "
    + r" ".join(re.escape(label) + rf"=(?P<{field}>\d+)"
                 for field, label in FIELDS[:-1])
    + r"(?: invalid-velocity=(?P<invalid_velocity>\d+))?(?=\s|$)"
)


class QALogError(ValueError):
    """A supplied local log does not contain valid diagnostic summaries."""


def summarize_log(text: str, require_all_gates: bool = False) -> dict:
    """Aggregate only structurally valid per-checkpoint summary lines.

    A counted event belongs to at most one displayed category, so the sum
    of displayed buckets must not exceed that window's qa-samples. Initial
    baselines and rarer rejection reasons are intentionally not displayed.
    """
    per_gate = {}
    windows = 0

    for line_number, line in enumerate(text.splitlines(), start=1):
        if "Latebrake checkpoint=" not in line or "qa-samples=" not in line:
            continue  # Ignore other standard Unreal and mode-1 event lines.
        match = PATTERN.search(line)
        if match is None:
            raise QALogError(f"line {line_number}: malformed checkpoint QA summary")
        # A corrupt new-format field must not be mistaken for a harmless
        # omitted legacy column. Reject duplicated counters as well.
        if (("invalid-velocity=" in line and
             match.group("invalid_velocity") is None) or
                line.count("invalid-velocity=") > 1):
            raise QALogError(
                f"line {line_number}: malformed invalid-velocity QA column"
            )

        gate = int(match.group("gate"))
        if gate not in range(4):
            raise QALogError(f"line {line_number}: checkpoint index must be 0..3")
        counts = {
            name: int(match.group(name)) if match.group(name) is not None else 0
            for name, _ in FIELDS
        }
        if any(value > LIMIT for value in counts.values()):
            raise QALogError(f"line {line_number}: uint32 counter exceeded")
        if counts["samples"] < 10:
            raise QALogError(
                f"line {line_number}: fewer than ten samples for a "
                "ten-second bounded-frame summary"
            )
        if sum(counts[key] for key in counts if key != "samples") > counts["samples"]:
            raise QALogError(
                f"line {line_number}: classified counts exceed sampled frames"
            )

        if gate not in per_gate:
            per_gate[gate] = {"gate": gate, "windows": 0, **{key: 0 for key, _ in FIELDS}}
        state = per_gate[gate]
        state["windows"] += 1
        for key, _ in FIELDS:
            state[key] += counts[key]
        windows += 1

    if not windows:
        raise QALogError("no v0.5.4 checkpoint QA summary records found")
    if require_all_gates and set(per_gate) != {0, 1, 2, 3}:
        missing = sorted({0, 1, 2, 3} - set(per_gate))
        raise QALogError(f"missing summaries for checkpoint gates: {missing}")

    return {
        "kind": "latebrake-local-checkpoint-qa",
        "version": "v0.5.4-source-development",
        "source_authenticity_verified": False,
        "unreal_build_or_gameplay_verified": False,
        "windows": windows,
        "gate_summaries": [per_gate[gate] for gate in sorted(per_gate)],
        "warning": (
            "Parsed self-reported log lines only; verify the installed "
            "Unreal game, actual devices and source SHA separately."
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="Existing captured Unreal log file")
    parser.add_argument(
        "--require-all-gates", action="store_true",
        help="Reject a log missing any of the four circuit gate indices",
    )
    args = parser.parse_args()
    try:
        if not args.log.is_file():
            raise QALogError("log path must be a readable existing file")
        if args.log.stat().st_size > 32 * 1024 * 1024:
            raise QALogError("log exceeds 32 MiB local parsing safety limit")
        report = summarize_log(
            args.log.read_text(encoding="utf-8"),
            require_all_gates=args.require_all_gates,
        )
    except (QALogError, OSError, UnicodeError) as error:
        print(f"Latebrake QA log rejected: {error}", file=sys.stderr)
        return 2
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
