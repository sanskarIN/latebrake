#!/usr/bin/env python3
"""Audit one real Unreal Development log for optional visual guide events.

Text-only evidence: passing NEVER proves Unreal rendering, gameplay, hardware
performance, engine origin, or package integrity.
"""
import argparse
from pathlib import Path
import re
import sys

GUIDE = "LATEBRAKE_RACING_LINE_READY marks=88/88 straight=48 corner=40 collision=off"
START = re.compile(r"LATEBRAKE_STARTUP_READY map=(\S+) checkpoints=4/4\Z")
VIS = re.compile(r"LATEBRAKE_RACING_LINE_VISIBILITY visible=(on|off)\Z")
# Accept normal Unreal timestamps/bracketed thread IDs, but reject a marker
# merely quoted by an unrelated LogTemp message or diagnostic text.
CATEGORY = re.compile(r"^(?:\[[^]\r\n]*\]\s*)*LogLatebrakeStartup:\s*")
WORLD = re.compile(r"(?:^|[/\\]|_)LatebrakeCircuit\Z")


def verify(text, require_toggle=False):
    events = []
    for n, line in enumerate(text.splitlines(), 1):
        category = CATEGORY.match(line)
        if category is None:
            continue
        msg = line[category.end():].strip()
        for level in ("Display: ", "Warning: ", "Error: ", "Log: "):
            if msg.startswith(level):
                msg = msg[len(level):].strip()
                break
        events.append((n, msg))

    def find(prefix):
        return [(n, msg) for n, msg in events if msg.startswith(prefix)]

    if find("LATEBRAKE_RACING_LINE_INCOMPLETE") or find("LATEBRAKE_STARTUP_BLOCKED"):
        raise ValueError("Runtime reported guide failure or blocked circuit.")
    ready = find("LATEBRAKE_RACING_LINE_READY")
    if len(ready) != 1 or ready[0][1] != GUIDE:
        raise ValueError("Expected one guide with exactly 88/88 (48+40) non-colliding instances.")
    start = find("LATEBRAKE_STARTUP_READY")
    if len(start) != 1:
        raise ValueError("Expected exactly one four-gate circuit startup marker.")
    match = START.fullmatch(start[0][1])
    if not match or not WORLD.search(match.group(1)):
        raise ValueError("Incorrect circuit world or checkpoint count.")
    if ready[0][0] >= start[0][0]:
        raise ValueError("Guide creation must precede startup readiness.")
    toggles = []
    for n, msg in find("LATEBRAKE_RACING_LINE_VISIBILITY"):
        parsed = VIS.fullmatch(msg)
        if not parsed or n <= start[0][0]:
            raise ValueError("Malformed or premature visibility event.")
        expected = "off" if len(toggles) % 2 == 0 else "on"
        if parsed.group(1) != expected:
            raise ValueError("Visibility toggles must alternate from default ON.")
        toggles.append(expected)
    if require_toggle and len(toggles) < 2:
        raise ValueError("Missing observed L-key OFF/ON toggle cycle.")
    return match.group(1), toggles


def main():
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument("log", type=Path)
    args.add_argument("--require-toggle", action="store_true")
    options = args.parse_args()
    try:
        if not options.log.is_file() or options.log.stat().st_size > 64 * 1024 * 1024:
            raise ValueError("Log missing or exceeds 64 MiB.")
        world, toggles = verify(
            options.log.read_text(encoding="utf-8", errors="replace"),
            require_toggle=options.require_toggle,
        )
    except (ValueError, OSError) as error:
        print(f"Racing-line source-log FAIL: {error}", file=sys.stderr)
        return 1
    print(f"Racing-line markers OK: {world} 88/88 (48+40); OFF/ON events: {toggles}")
    print("SOURCE-LOG ONLY: no in-engine visual or gameplay qualification.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
