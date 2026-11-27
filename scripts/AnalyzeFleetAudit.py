#!/usr/bin/env python3
"""Check the structure of a local non-shipping six-car audit log.

Logs are editable, unauthenticated observations. This report never claims
native build, physical gameplay, shipping readiness or saved-data proof.
"""
import argparse
import hashlib
import json
import re
import sys
from decimal import Decimal
from pathlib import Path

MARKER = "LATEBRAKE_FLEET_AUDIT"
MAX_LOG_BYTES = 32 * 1024 * 1024
MAX_EVENTS = 300_000
MAX_ATTEMPTS = 10_000
MAX_TOKEN = (1 << 64) - 1
MAX_ACTIVE = Decimal("604800")
CLOCK_TOLERANCE = Decimal("0.000000001")
U = r"(?:0|[1-9][0-9]{0,19})"
P = r"(?:[1-9][0-9]{0,19})"
CLOCK = r"(?:0|[1-9][0-9]{0,6})\.[0-9]{9}"
HEADER = re.compile(
    rf"^.*?\b{MARKER} v=1 action=(?P<action>begin|roster|go|gate|recovery|refused|fault|finish) "
    rf"attempt=(?P<attempt>{P}) (?P<fields>[^\r\n]+)$"
)
PATTERNS = {
    "begin": re.compile(rf"gates=(?P<gates>{P}) laps=(?P<laps>{P})"),
    "roster": re.compile(
        rf"frame=(?P<frame>{P}) " + " ".join(rf"id{i}=(?P<id{i}>{P})" for i in range(6))
    ),
    "go": re.compile(
        rf"frame=(?P<frame>{P}) stable=(?P<stable>{CLOCK}) startup=(?P<startup>{CLOCK})"
    ),
    "gate": re.compile(
        rf"slot=(?P<slot>[0-5]) id=(?P<id>{P}) ordinal=(?P<ordinal>{P}) clock=(?P<clock>{CLOCK})"
    ),
    "recovery": re.compile(
        rf"slot=(?P<slot>[0-5]) id=(?P<id>{P}) ordinal=(?P<ordinal>{P}) verified=(?P<verified>[01])"
    ),
    "refused": re.compile(
        rf"kind=(?P<kind>gate|recovery) slot=(?P<slot>[1-5]) ordinal=(?P<ordinal>{U})"
    ),
    "fault": re.compile(r"reason=(?P<reason>[1-9]) slot=(?P<slot>-1|[0-5])"),
    "finish": re.compile(
        rf"clock=(?P<clock>{CLOCK}) fees=(?P<fees>{CLOCK}) timing=(?P<timing>[01]) "
        r"review=(?P<review>[01]) completed_routes=(?P<completed_routes>[0-5]) saved=(?P<saved>0)"
    ),
}


class FleetAuditError(ValueError):
    """An inconsistent, malformed, or insufficient fleet trace."""


def checked_int(fields, key, maximum=MAX_TOKEN):
    value = int(fields[key])
    if not 0 <= value <= maximum:
        raise FleetAuditError(f"{key} outside accepted range")
    return value


def clock(fields, key):
    value = Decimal(fields[key])
    if not Decimal(0) <= value <= MAX_ACTIVE:
        raise FleetAuditError(f"{key} outside active-clock range")
    return value


def require(condition, message):
    if not condition:
        raise FleetAuditError(message)


def new_attempt(attempt, fields):
    gates = checked_int(fields, "gates", 256)
    laps = checked_int(fields, "laps", 100)
    require(gates == 4 and laps >= 1, "unsupported experimental route")
    return {
        "attempt": attempt,
        "gates": gates,
        "laps": laps,
        "total_gates": gates * laps,
        "phase": "awaiting_roster",
        "roster_frame": None,
        "go_frame": None,
        "startup_seconds": None,
        "stable_seconds": None,
        "cars": [
            {
                "slot": slot,
                "actor_token": None,
                "accepted_gates": 0,
                "last_gate_clock": None,
                "recovery_attempts": 0,
                "successful_recoveries": 0,
                "failed_recoveries": 0,
            }
            for slot in range(6)
        ],
        "refused_receipts": [],
        "fault": None,
        "finish": None,
        "last_active_clock": None,
    }


def bind_roster(attempt, fields):
    require(attempt["phase"] == "awaiting_roster", "duplicate or late roster")
    frame = checked_int(fields, "frame")
    tokens = [checked_int(fields, f"id{i}") for i in range(6)]
    require(len(set(tokens)) == 6 and all(tokens), "roster contains duplicate identity")
    for row, token in zip(attempt["cars"], tokens):
        row["actor_token"] = token
    attempt["roster_frame"] = frame
    attempt["phase"] = "awaiting_go"


def accept_go(attempt, fields):
    require(attempt["phase"] == "awaiting_go", "GO without exact bound roster")
    frame = checked_int(fields, "frame")
    stable, startup = clock(fields, "stable"), clock(fields, "startup")
    require(frame >= attempt["roster_frame"], "GO predates roster")
    require(Decimal("0.299999999") <= stable <= Decimal("0.300000001"),
            "GO lacks continuous grid readiness")
    # Watchdog stability and real start lights may share one qualified
    # 0.10-second frame. Never claim more precise wall-time evidence.
    require(Decimal("3.199999999") <= startup < Decimal("15"),
            "GO outside configured qualified startup window")
    attempt["go_frame"] = frame
    attempt["startup_seconds"] = str(startup)
    attempt["stable_seconds"] = str(stable)
    attempt["phase"] = "running"


def actual_car(attempt, fields):
    require(attempt["phase"] == "running", "vehicle receipt outside running attempt")
    slot = checked_int(fields, "slot", 5)
    token = checked_int(fields, "id")
    row = attempt["cars"][slot]
    require(row["actor_token"] == token and token != 0, "actor identity changed")
    return row


def accept_gate(attempt, fields):
    row = actual_car(attempt, fields)
    ordinal = checked_int(fields, "ordinal", attempt["total_gates"])
    require(ordinal == row["accepted_gates"] + 1, "gate ordinal skipped or duplicated")
    active = clock(fields, "clock")
    prior = row["last_gate_clock"]
    if prior is not None:
        require(active >= Decimal(prior), "car gate clock moved backwards")
        if row["slot"] != 0:
            require(active > Decimal(prior), "rival gate clock must strictly advance")
    if row["slot"] != 0:
        require(active > 0, "rival receipt lacks positive active clock")
    previous_global = attempt["last_active_clock"]
    # Preserve a high-water mark: a tolerated rounding discrepancy must
    # never lower the baseline for the next independently logged car.
    high_water = Decimal(previous_global) if previous_global is not None else active
    require(active + CLOCK_TOLERANCE >= high_water,
            "shared active clock moved backwards")
    attempt["last_active_clock"] = str(max(active, high_water))
    row["last_gate_clock"] = str(active)
    row["accepted_gates"] = ordinal


def accept_recovery(attempt, fields):
    row = actual_car(attempt, fields)
    ordinal = checked_int(fields, "ordinal", 100_000)
    require(row["accepted_gates"] < attempt["total_gates"], "recovery after completed route")
    require(ordinal == row["recovery_attempts"] + 1, "recovery ordinal skipped or duplicated")
    verified = fields["verified"] == "1"
    require(row["slot"] != 0 or verified, "unverified player recovery cannot be charged")
    row["recovery_attempts"] = ordinal
    if verified:
        row["successful_recoveries"] += 1
    else:
        row["failed_recoveries"] += 1


def accept_refusal(attempt, fields):
    require(attempt["phase"] == "running", "refused receipt outside running attempt")
    slot = checked_int(fields, "slot", 5)
    ordinal = checked_int(fields, "ordinal", 100_000)
    attempt["refused_receipts"].append({
        "kind": fields["kind"], "slot": slot, "ordinal": ordinal,
    })


def accept_fault(attempt, fields):
    require(attempt["phase"] in ("awaiting_roster", "awaiting_go", "running"),
            "duplicate fault or fault after finish")
    attempt["fault"] = {"reason": int(fields["reason"]), "slot": int(fields["slot"])}
    attempt["phase"] = "faulted"


def accept_finish(attempt, fields):
    require(attempt["phase"] == "running", "finish outside running attempt")
    player = attempt["cars"][0]
    require(player["accepted_gates"] == attempt["total_gates"],
            "finish without all ordered player gates")
    active, fees = clock(fields, "clock"), clock(fields, "fees")
    require(player["last_gate_clock"] is not None and
            active == Decimal(player["last_gate_clock"]),
            "finish clock does not match last player gate")
    high_water = attempt["last_active_clock"]
    require(high_water is not None and active + CLOCK_TOLERANCE >= Decimal(high_water),
            "finish predates shared active-clock high-water mark")
    require(active + fees <= MAX_ACTIVE, "combined race clock outside review bound")
    require(fees >= Decimal(5 * player["successful_recoveries"]),
            "reported fees omit a verified player recovery")
    timing = fields["timing"] == "1"
    review = fields["review"] == "1"
    completed = checked_int(fields, "completed_routes", 5)
    logged_complete = sum(
        row["accepted_gates"] == attempt["total_gates"] for row in attempt["cars"][1:]
    )
    require(not timing or active > 0, "eligible finish lacks active time")
    require(not review or timing, "finish review claims ineligible timing")
    require(review or completed == 0, "invalid review reports completed routes")
    require(completed <= logged_complete, "finish invents opponent gate receipts")
    if review and not attempt["refused_receipts"]:
        require(completed == logged_complete, "review and recorded complete routes disagree")
    attempt["finish"] = {
        "clock": str(active),
        "fees": str(fees),
        "timing_eligible_reported": timing,
        "review_valid_reported": review,
        "observed_completed_routes": completed,
        "saved_rewards_reported": False,
    }
    attempt["phase"] = "finished"


HANDLERS = {
    "roster": bind_roster,
    "go": accept_go,
    "gate": accept_gate,
    "recovery": accept_recovery,
    "refused": accept_refusal,
    "fault": accept_fault,
    "finish": accept_finish,
}


def complete_candidate(attempt):
    finish = attempt["finish"]
    return (
        attempt["phase"] == "finished" and finish is not None
        and finish["timing_eligible_reported"] and finish["review_valid_reported"]
        and not attempt["refused_receipts"] and attempt["fault"] is None
    )


def all_rivals_moved(attempt):
    return all(row["accepted_gates"] >= 1 for row in attempt["cars"][1:])


def analyze_fleet_audit(text, require_finish=False, require_all_rivals=False):
    require(isinstance(text, str) and len(text) <= MAX_LOG_BYTES, "audit text exceeds bound")
    attempts = []
    event_count = 0
    current = None
    for line_no, line in enumerate(text.splitlines(), 1):
        if MARKER not in line:
            continue
        try:
            require(len(line) <= 1024 and line.count(MARKER) == 1,
                    "oversized or ambiguous audit marker")
            event = HEADER.fullmatch(line.strip())
            require(event is not None, "malformed fleet audit marker")
            action = event["action"]
            fields = PATTERNS[action].fullmatch(event["fields"])
            require(fields is not None, f"malformed or extra {action} fields")
            attempt_id = checked_int(event, "attempt")
            event_count += 1
            require(event_count <= MAX_EVENTS, "too many audit events")
            if action == "begin":
                require(len(attempts) < MAX_ATTEMPTS, "too many experimental attempts")
                require(current is None or attempt_id > current["attempt"],
                        "attempt ID did not increase")
                # Solo/career attempts intentionally leave gaps. The
                # experimental stream only includes its own attempts.
                current = new_attempt(attempt_id, fields)
                attempts.append(current)
                continue
            require(current is not None and current["attempt"] == attempt_id,
                    "receipt does not belong to current attempt")
            HANDLERS[action](current, fields)
        except FleetAuditError as error:
            raise FleetAuditError(f"line {line_no}: {error}") from error
    require(attempts, "no experimental fleet begin record")
    completed = [a for a in attempts if complete_candidate(a)]
    moved = [a for a in completed if all_rivals_moved(a)]
    if require_finish:
        require(completed, "no complete timing-eligible fleet finish without refused receipts")
    if require_all_rivals:
        require(moved, "no complete reported finish with accepted gates from all five rivals")
    return {
        "schema": 1,
        "kind": "latebrake-local-experimental-fleet-audit",
        "attempt_count": len(attempts),
        "event_count": event_count,
        "reported_complete_attempts": len(completed),
        "reported_all_rivals_moved_attempts": len(moved),
        "attempts": attempts,
        "source_authenticity_verified": False,
        "unreal_build_verified": False,
        "installed_gameplay_verified": False,
        "saved_data_integrity_verified": False,
        "ready_for_release": False,
        "warning": (
            "Editable diagnostic text provides structural observations only. "
            "It cannot prove authentic build, native physical motion, saved-data "
            "integrity, competitive placement, or release readiness."
        ),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="Original local Unreal log")
    parser.add_argument("--require-finish", action="store_true")
    parser.add_argument("--require-all-rivals", action="store_true")
    args = parser.parse_args()
    try:
        require(args.log.is_file() and not args.log.is_symlink(),
                "log must be an existing non-symlink file")
        with args.log.open("rb") as stream:
            data = stream.read(MAX_LOG_BYTES + 1)
        require(len(data) <= MAX_LOG_BYTES, "log exceeds 32 MiB limit")
        report = analyze_fleet_audit(
            data.decode("utf-8-sig"), args.require_finish, args.require_all_rivals
        )
        report["log_bytes"] = len(data)
        report["log_sha256"] = hashlib.sha256(data).hexdigest()
    except (FleetAuditError, OSError, UnicodeError) as error:
        print(f"LateBrake fleet audit rejected: {error}", file=sys.stderr)
        return 2
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
