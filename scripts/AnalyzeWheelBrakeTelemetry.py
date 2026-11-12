#!/usr/bin/env python3
"""Audit MODELED wheel-brake observations; never certify Unreal gameplay.

Unrelated log lines are ignored. A recognized record must match the complete
v1 schema and finite domains. Editable/synthetic logs are not evidence of an
engine build, an original cooked map, installed driving or physical ABS.
"""
from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
import json
import math
from pathlib import Path
import re

MARKER = 'LATEBRAKE_WHEEL_BRAKES '
CORNERS = ('fl', 'fr', 'rl', 'rr')
BASE_FIELDS = {'v', 'driver', 'source', 'actor_id', 'receipt', 'steps',
               'iterations', 'vehicle', 'active_mask', 'dt_s'}
WHEEL_PREFIXES = ('road_', 'omega_', 'slip_', 'pressure_', 'normal_est_', 'command_')
FIELDS = BASE_FIELDS | {
    prefix + corner + ('_cmps' if prefix == 'road_' else '_n' if prefix in
                      ('normal_est_', 'command_') else '')
    for corner in CORNERS for prefix in WHEEL_PREFIXES
}


@dataclass(frozen=True)
class WheelBrakeRecord:
    driver: str
    actor_id: int
    receipt: int
    vehicle: int
    active_mask: int
    seconds: float
    values: dict[str, float]


def integer(value: str, name: str, low: int, high: int) -> int:
    if not re.fullmatch(r'[0-9]+', value):
        raise ValueError(f'{name}: expected unsigned decimal integer')
    number = int(value)
    if not low <= number <= high:
        raise ValueError(f'{name}: out of range')
    return number


def parse_record(line: str) -> WheelBrakeRecord | None:
    at = line.find(MARKER)
    if at < 0:
        return None
    tokens = line[at + len(MARKER):].strip().split()
    fields: dict[str, str] = {}
    for token in tokens:
        if token.count('=') != 1:
            raise ValueError('malformed wheel-brake token')
        key, value = token.split('=', 1)
        if key in fields:
            raise ValueError(f'duplicate field {key}')
        fields[key] = value
    if set(fields) != FIELDS:
        raise ValueError(f'incomplete/unknown wheel-brake schema: {sorted(set(fields) ^ FIELDS)}')
    if fields['v'] != '1' or fields['source'] != 'modeled':
        raise ValueError('unknown schema version or false measurement label')
    if fields['driver'] not in ('player', 'autonomous'):
        raise ValueError('unknown driver role')
    actor = integer(fields['actor_id'], 'actor_id', 0, 2**32 - 1)
    receipt = integer(fields['receipt'], 'receipt', 1, 2**64 - 1)
    steps = integer(fields['steps'], 'steps', 1, 61)
    integer(fields['iterations'], 'iterations', 0, 4 * steps * 36)
    vehicle = integer(fields['vehicle'], 'vehicle', 0, 5)
    mask = integer(fields['active_mask'], 'active_mask', 0, 15)
    values: dict[str, float] = {}
    for key in ('dt_s', *(FIELDS - BASE_FIELDS)):
        try:
            value = float(fields[key])
        except ValueError as exc:
            raise ValueError(f'{key}: invalid number') from exc
        if not math.isfinite(value):
            raise ValueError(f'{key}: nonfinite number')
        values[key] = value
    if not 0.000001 <= values['dt_s'] <= 0.1:
        raise ValueError('invalid modeled frame duration')
    for index, corner in enumerate(CORNERS):
        speed = values[f'road_{corner}_cmps']
        omega = values[f'omega_{corner}']
        slip = values[f'slip_{corner}']
        pressure = values[f'pressure_{corner}']
        load = values[f'normal_est_{corner}_n']
        force = values[f'command_{corner}_n']
        if (abs(speed) > 100000.0 or abs(omega) > 10000.0 or abs(slip) > 4.0 or
                not 0.0 <= pressure <= 1.0 or not 0.0 <= load <= 1000000.0 or
                abs(force) > 1000000.0):
            raise ValueError(f'{corner}: numeric domain violation')
        if not mask & (1 << index):
            if any(value != 0.0 for value in (omega, slip, pressure, load, force)):
                raise ValueError(f'{corner}: inactive channel carries stale modeled output')
        if force * speed * 0.01 > 0.00001:
            raise ValueError(f'{corner}: commanded brake adds local body energy')
        # Global profile domain is mu <= 3 and grip <= 4. Include printed
        # 4-decimal rounding in this conservative bound; this is not a
        # measurement of actual friction or validation of a tire setup.
        if abs(force) > 12.0 * (load + 0.000051) + 0.000051:
            raise ValueError(f'{corner}: commanded force exceeds profile-domain load ceiling')
    return WheelBrakeRecord(fields['driver'], actor, receipt, vehicle, mask,
                            values['dt_s'], values)


def analyze(lines: list[str], *, require_player: bool = False) -> dict:
    actors: dict[int, tuple[str, int]] = {}
    driver_counts: Counter = Counter()
    pressure_counts: Counter = Counter()
    active_counts: Counter = Counter()
    records = 0
    sampled_work = 0.0
    for number, line in enumerate(lines, 1):
        try:
            record = parse_record(line)
            if record is None:
                continue
            previous = actors.get(record.actor_id)
            if previous and (record.driver != previous[0] or record.receipt <= previous[1]):
                raise ValueError('actor identity changed or receipt did not advance; use a fresh log per run')
            actors[record.actor_id] = (record.driver, record.receipt)
            records += 1
            driver_counts[record.driver] += 1
            for index, corner in enumerate(CORNERS):
                if record.active_mask & (1 << index):
                    active_counts[corner] += 1
                    if record.values[f'pressure_{corner}'] < 0.9999:
                        pressure_counts[corner] += 1
                sampled_work += record.values[f'command_{corner}_n'] *\
                    record.values[f'road_{corner}_cmps'] * 0.01 * record.seconds
        except ValueError as exc:
            raise ValueError(f'line {number}: {exc}') from exc
    if records == 0 or (require_player and driver_counts['player'] == 0):
        raise ValueError('no qualifying modeled wheel-brake records for requested driver scope')
    return {
        'schema_version': 1,
        'kind': 'modeled-wheel-brake-log-audit',
        'unreal_runtime_verified': False,
        'records': records,
        'actors': len(actors),
        'records_by_driver': dict(sorted(driver_counts.items())),
        'active_channel_samples': dict(sorted(active_counts.items())),
        'modeled_pressure_reduction_samples': dict(sorted(pressure_counts.items())),
        'sampled_command_work_joules': sampled_work,
        'scope': 'sparse logged frames only; not total race work or measured tire forces',
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    parser.add_argument('--require-player', action='store_true')
    args = parser.parse_args()
    if not args.log.is_file() or args.log.stat().st_size > 32 * 1024 * 1024:
        parser.error('provide a regular UTF-8 log of at most 32 MiB')
    try:
        result = analyze(args.log.read_text(encoding='utf-8').splitlines(),
                         require_player=args.require_player)
    except (ValueError, UnicodeError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
