#!/usr/bin/env python3
"""Verify structural completeness of a saved, untrusted GitHub CI check list.

This is a no-network source-matrix receipt checker. Local JSON can be forged:
even an all-green report does not authenticate GitHub, licensed Unreal,
installed hardware, playable game quality, or a protected-branch approval.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys

MAX_INPUT_BYTES = 2 * 1024 * 1024
MAX_CHECKS = 100
SHA_PATTERN = re.compile(r"^[a-fA-F0-9]{40}$")
REQUIRED_CHECKS = (
    "Portable C++20 tests / windows-latest",
    "Portable C++20 tests / ubuntu-latest",
    "Portable C++20 tests / macos-latest",
    "Unreal editor-only Python syntax (not Unreal runtime)",
    "Release PowerShell syntax",
    "Offline source-release publication guards (NO GitHub writes)",
    "Build and verify docs",
)
OPTIONAL_CHECKS = (
    "Deploy official Pages documentation",
)
COMPLETED = "completed"
SUCCESS = "success"
OPTIONAL_CONCLUSIONS = frozenset(("success", "skipped"))


class SourceMatrixAuditError(ValueError):
    """Invalid source-only check list or unreviewable evidence structure."""


def unique_properties(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise SourceMatrixAuditError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def read_saved_checks(path: Path) -> dict[str, object]:
    if path.is_symlink() or not path.is_file():
        raise SourceMatrixAuditError("source check JSON must be an existing plain file")
    if path.stat().st_size > MAX_INPUT_BYTES:
        raise SourceMatrixAuditError("source check JSON exceeds 2 MiB")
    try:
        value = json.loads(
            path.read_text(encoding="utf-8-sig"),
            object_pairs_hook=unique_properties,
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise SourceMatrixAuditError("unreadable or malformed check-run JSON") from error
    if not isinstance(value, dict):
        raise SourceMatrixAuditError("GitHub check runs must have a JSON object root")
    return value


def analyze_saved_source_matrix(
    payload: dict[str, object], expected_head: str
) -> dict[str, object]:
    if not isinstance(expected_head, str) or not SHA_PATTERN.fullmatch(expected_head):
        raise SourceMatrixAuditError("expected exact 40-digit Git head SHA is required")
    head = expected_head.lower()
    items = payload.get("check_runs")
    if not isinstance(items, list) or len(items) > MAX_CHECKS:
        raise SourceMatrixAuditError("expected bounded check_runs JSON array")
    seen: set[str] = set()
    pending: list[str] = []
    failed: list[str] = []
    unknown: list[str] = []
    wrong_head: list[str] = []
    counted: dict[str, dict[str, object]] = {}
    required = set(REQUIRED_CHECKS)
    optional = set(OPTIONAL_CHECKS)
    for index, item in enumerate(items):
        if not isinstance(item, dict):
            raise SourceMatrixAuditError(f"check {index} is not an object")
        name = item.get("name")
        status = item.get("status")
        conclusion = item.get("conclusion")
        check_head = item.get("head_sha")
        if not isinstance(name, str) or not name or len(name) > 200:
            raise SourceMatrixAuditError(f"check {index} has invalid name")
        if name in seen:
            raise SourceMatrixAuditError(f"duplicate check name: {name}")
        seen.add(name)
        if not isinstance(check_head, str) or not SHA_PATTERN.fullmatch(check_head):
            raise SourceMatrixAuditError(f"{name}: missing or malformed exact check SHA")
        if status not in ("queued", "in_progress", COMPLETED):
            raise SourceMatrixAuditError(f"{name}: unsupported check status")
        if status == COMPLETED:
            if conclusion not in ("success", "failure", "cancelled", "timed_out",
                                  "neutral", "skipped", "action_required", "stale"):
                raise SourceMatrixAuditError(f"{name}: invalid completed conclusion")
        elif conclusion is not None:
            raise SourceMatrixAuditError(
                f"{name}: unfinished check cannot claim a completed conclusion"
            )
        if check_head.lower() != head:
            wrong_head.append(name)
        if name not in required and name not in optional:
            unknown.append(name)
            # Unknown checks are neither counted as required nor a pass.
            continue
        counted[name] = item
        if status != COMPLETED:
            pending.append(name)
        elif conclusion != SUCCESS and (name in required or conclusion not in OPTIONAL_CONCLUSIONS):
            failed.append(name)
    missing = sorted(required - seen)
    passed = (
        not missing and not pending and not failed and
        not wrong_head and all(
            counted.get(name, {}).get("conclusion") == SUCCESS
            for name in REQUIRED_CHECKS
        )
    )
    return {
        "schema": 1,
        "kind": "latebrake-saved-source-check-matrix-audit",
        "expected_head_sha": head,
        "required_checks": list(REQUIRED_CHECKS),
        "reported_checks": len(items),
        "missing_required": missing,
        "pending": sorted(pending),
        "failed_or_not_success": sorted(failed),
        "wrong_source_commit": sorted(wrong_head),
        "unrecognized_checks": sorted(unknown),
        "source_report_structurally_all_green": passed,
        "ci_authenticity_verified": False,
        "native_unreal_build_verified": False,
        "installed_gameplay_verified": False,
        "warning": (
            "Saved JSON can be altered. Verify GitHub source URL and exact "
            "commit independently; real Unreal/installed Win64 QA is separate."
        ),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checks", type=Path, required=True,
                        help="Saved GitHub check-runs REST JSON object")
    parser.add_argument("--expected-head", required=True,
                        help="Exact expected 40-character Git commit SHA")
    args = parser.parse_args(argv)
    try:
        checks = read_saved_checks(args.checks)
        report = analyze_saved_source_matrix(checks, args.expected_head)
    except (SourceMatrixAuditError, OSError) as error:
        print(f"Source matrix receipt FAIL: {error}", file=sys.stderr)
        return 2
    print(json.dumps(report, sort_keys=True, indent=2))
    return 0 if report["source_report_structurally_all_green"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
