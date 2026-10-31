#!/usr/bin/env python3
"""Parse local non-shipping LateBrake player race-audit event logs.

Log text is unauthenticated and can be forged or edited. A valid report is
structural source-level evidence only, NEVER proof of an Unreal package,
installed gameplay, hardware performance or release readiness.
"""
import argparse
import json
import re
import sys
from decimal import Decimal, InvalidOperation
from pathlib import Path

MAX_LOG_BYTES = 32 * 1024 * 1024
MAX_ATTEMPTS = 100000
MAX_GATE_EVENTS = 100000
MAX_INT = 2147483647
LOG_MARKER = "LATEBRAKE_RACE_AUDIT"
LINE_PATTERN = re.compile(
    r"^.*?\bLATEBRAKE_RACE_AUDIT v=1 "
    r"action=(?P<action>begin|gate) attempt=(?P<attempt>[1-9][0-9]*) "
    r"(?P<fields>[^\r\n]*)$"
)
BEGIN_PATTERN = re.compile(
    r"gates=(?P<gates>[1-9][0-9]*) "
    r"laps=(?P<laps>[1-9][0-9]*) "
    r"track=(?P<track>[01]) grid=(?P<grid>-1|[01])"
)
GATE_PATTERN = re.compile(
    r"gate=(?P<gate>0|[1-9][0-9]*) "
    r"lap=(?P<lap>0|[1-9][0-9]*) "
    r"next=(?P<next>0|[1-9][0-9]*) "
    r"elapsed=(?P<elapsed>(?:0|[1-9][0-9]*)\.[0-9]{6}) "
    r"active=(?P<active>0|[1-9][0-9]*) "
    r"finished=(?P<finished>[01])"
)


class RaceAuditError(ValueError):
    """The supplied text is not a complete, internally consistent audit."""


def _checked_int(value: str, line_no: int, field: str, maximum: int = MAX_INT) -> int:
    number = int(value)
    if number > maximum:
        raise RaceAuditError(f"line {line_no}: {field} exceeds bound")
    return number


def analyze_race_audit(text: str, require_finish: bool = False) -> dict:
    attempts = []
    current = None
    total_gate_events = 0
    for line_no, line in enumerate(text.splitlines(), 1):
        if LOG_MARKER not in line:
            continue
        found = LINE_PATTERN.fullmatch(line.strip())
        if not found:
            raise RaceAuditError(f"line {line_no}: malformed race audit marker")
        attempt_id = _checked_int(found["attempt"], line_no, "attempt")
        action = found["action"]
        pattern = BEGIN_PATTERN if action == "begin" else GATE_PATTERN
        values = pattern.fullmatch(found["fields"])
        if not values:
            raise RaceAuditError(f"line {line_no}: malformed or extra {action} fields")

        if action == "begin":
            if len(attempts) >= MAX_ATTEMPTS:
                raise RaceAuditError(f"line {line_no}: too many attempts")
            if attempt_id != len(attempts) + 1:
                raise RaceAuditError(
                    f"line {line_no}: attempts must increase consecutively from one"
                )
            gates = _checked_int(values["gates"], line_no, "gates", 256)
            laps = _checked_int(values["laps"], line_no, "laps", 100)
            current = {
                "attempt": attempt_id,
                "gates": gates,
                "laps_required": laps,
                "circuit_ready_at_begin": values["track"] == "1",
                "grid_at_begin": int(values["grid"]),  # -1 means initial spawn unobserved
                "accepted_gates": 0,
                "completed_laps": 0,
                "active_timed_laps": 0,
                "last_elapsed_seconds": None,
                "finished": False,
            }
            if current["grid_at_begin"] == -1 and attempt_id != 1:
                raise RaceAuditError(
                    f"line {line_no}: only initial attempt may have unknown grid"
                )
            attempts.append(current)
            continue

        if current is None or current["attempt"] != attempt_id:
            raise RaceAuditError(f"line {line_no}: gate has no matching live attempt")
        if total_gate_events >= MAX_GATE_EVENTS:
            raise RaceAuditError(f"line {line_no}: too many gate events")
        if current["finished"]:
            raise RaceAuditError(f"line {line_no}: gate after finished race")
        if not current["circuit_ready_at_begin"] or current["grid_at_begin"] == 0:
            raise RaceAuditError(f"line {line_no}: scored gate in blocked attempt")
        gate = _checked_int(values["gate"], line_no, "gate")
        lap = _checked_int(values["lap"], line_no, "lap")
        next_gate = _checked_int(values["next"], line_no, "next")
        active = _checked_int(values["active"], line_no, "active")
        if gate != current["accepted_gates"] % current["gates"]:
            raise RaceAuditError(f"line {line_no}: out-of-order or duplicated gate")
        count = current["accepted_gates"] + 1
        completed = count // current["gates"]
        expected_next = count % current["gates"]
        if (next_gate != expected_next or lap != completed or
                lap > current["laps_required"]):
            raise RaceAuditError(f"line {line_no}: lap or next-gate state mismatch")
        # Active timed laps can only grow when an entire ordered lap ends,
        # and a lap must add at most one active-timing count.
        prior_active = current["active_timed_laps"]
        max_active = prior_active + (1 if lap > current["completed_laps"] else 0)
        if active < prior_active or active > max_active or active > lap:
            raise RaceAuditError(f"line {line_no}: active lap count mismatch")
        finished = values["finished"] == "1"
        if finished != (lap == current["laps_required"]):
            raise RaceAuditError(f"line {line_no}: finish flag mismatches lap target")
        try:
            elapsed = Decimal(values["elapsed"])
        except InvalidOperation as error:
            raise RaceAuditError(f"line {line_no}: invalid elapsed time") from error
        prior_elapsed = current["last_elapsed_seconds"]
        if prior_elapsed is not None and elapsed < Decimal(prior_elapsed):
            raise RaceAuditError(f"line {line_no}: race clock moved backwards")
        if elapsed > Decimal("604800"):
            raise RaceAuditError(f"line {line_no}: elapsed time outside review window")
        current["accepted_gates"] = count
        current["completed_laps"] = lap
        current["active_timed_laps"] = active
        current["last_elapsed_seconds"] = str(elapsed)
        current["finished"] = finished
        total_gate_events += 1

    if not attempts:
        raise RaceAuditError("no complete race audit attempt-start records found")
    finished_attempts = sum(1 for a in attempts if a["finished"])
    if require_finish and finished_attempts == 0:
        raise RaceAuditError("no complete ordered race finish reported")
    return {
        "schema": 1,
        "kind": "latebrake-local-race-audit",
        "attempt_count": len(attempts),
        "accepted_gate_events": total_gate_events,
        "reported_finished_attempts": finished_attempts,
        "attempts": attempts,
        "source_authenticity_verified": False,
        "unreal_build_verified": False,
        "installed_gameplay_verified": False,
        "ready_for_release": False,
        "warning": (
            "Self-reported editable runtime text cannot prove original Unreal "
            "build, physical directional crossing, actual play or installed QA."
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="Existing real Unreal runtime log")
    parser.add_argument(
        "--require-finish", action="store_true",
        help="Reject logs lacking any fully ordered reported finish",
    )
    args = parser.parse_args()
    try:
        if not args.log.is_file():
            raise RaceAuditError("log path must be a readable existing file")
        if args.log.stat().st_size > MAX_LOG_BYTES:
            raise RaceAuditError("log exceeds 32 MiB safety bound")
        report = analyze_race_audit(
            args.log.read_text(encoding="utf-8"),
            require_finish=args.require_finish,
        )
    except (RaceAuditError, OSError, UnicodeError) as error:
        print(f"LateBrake race audit rejected: {error}", file=sys.stderr)
        return 2
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
