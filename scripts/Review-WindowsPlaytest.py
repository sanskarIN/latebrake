#!/usr/bin/env python3
"""Offline, source-only recorder for hands-on LateBrake Windows playtest QA.

This tool NEVER launches or qualifies a game. It links user-authored manual
observations to a previously captured private runtime receipt and recomputes
candidate EXE/manifest SHA256. It cannot prove genuine Unreal provenance,
rendering, input handling, physical hardware, or release readiness.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

SCHEMA = 1
CHECKS = (
    "scene_and_circuit_visible",
    "car_physics_and_forward_motion",
    "brake_and_reverse",
    "steering_and_camera",
    "go_lights_and_checkpoint_order",
    "pause_focus_and_restart",
    "recovery_and_time_penalty",
    "results_and_save_reload",
    "windows_framerate_and_stability",
)
RESULTS = frozenset(("pass", "fail", "not_tested"))
MAX_JSON_BYTES = 128 * 1024
WARN = (
    "SELF-REPORTED, UNVERIFIED: this file is not engine build provenance, "
    "independent gameplay qualification or a playable release certificate."
)


def fail(message: str) -> None:
    raise ValueError(message)


def unique_pairs(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            fail(f"Duplicate JSON property: {key}")
        result[key] = value
    return result


def read_json(path: Path) -> dict:
    if not path.is_file() or path.stat().st_size > MAX_JSON_BYTES:
        fail(f"Missing or oversized JSON input: {path}")
    data = json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique_pairs)
    if not isinstance(data, dict):
        fail("Expected a JSON object.")
    return data


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require_hash(value: object, label: str) -> str:
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", value):
        fail(f"{label} must be a SHA256 hexadecimal digest.")
    return value.lower()


def candidate_context(receipt_file: Path) -> tuple[dict, Path, Path, Path, str]:
    receipt = read_json(receipt_file)
    if receipt.get("schema_version") != 1 or receipt.get("status") != "UNVERIFIED_PERSONAL_RUNTIME_PROBE":
        fail("Only private Win64 runtime-probe receipts are accepted.")
    if receipt.get("gameplay_verified") is not False:
        fail("Runtime receipts must NEVER claim verified gameplay.")
    if receipt.get("source") != "locally_launched_packaged_win64_candidate":
        fail("Unrecognized candidate source.")
    if not isinstance(receipt.get("process_exit_code"), (int, type(None))) or isinstance(receipt.get("process_exit_code"), bool):
        fail("Invalid process exit code.")
    if not isinstance(receipt.get("unreal_log_present"), bool) or not isinstance(receipt.get("startup_log_markers_pass"), bool):
        fail("Runtime log status must be explicit booleans.")
    raw_root, raw_exe = receipt.get("candidate_directory"), receipt.get("executable_path")
    if not isinstance(raw_root, str) or not isinstance(raw_exe, str):
        fail("Candidate and EXE paths are required.")
    root, exe = Path(raw_root).resolve(), Path(raw_exe).resolve()
    if not root.is_dir() or not exe.is_file() or exe.name.lower() != "latebrake.exe":
        fail("The candidate and its genuine-named EXE must exist locally.")
    if exe == root or not exe.is_relative_to(root):
        fail("EXE escapes the candidate directory.")
    manifest = (root / "Latebrake-candidate-manifest.json").resolve()
    if not manifest.is_file() or not manifest.is_relative_to(root):
        fail("Candidate manifest must exist within its own directory.")
    if sha256(exe) != require_hash(receipt.get("executable_sha256"), "Receipt EXE hash"):
        fail("EXE SHA256 no longer matches the runtime receipt.")
    if sha256(manifest) != require_hash(receipt.get("candidate_manifest_sha256"), "Receipt manifest hash"):
        fail("Manifest SHA256 no longer matches the runtime receipt.")
    # Re-run the PowerShell full-manifest guard separately to validate every
    # packaged file. This reviewer only seals the EXE, manifest and receipt.
    return receipt, root, exe, manifest, sha256(receipt_file)


def write_new(path: Path, value: dict, candidate_root: Path) -> None:
    destination = path.resolve(strict=False)
    if destination == candidate_root or destination.is_relative_to(candidate_root):
        fail("QA notes must stay OUTSIDE the sealed game candidate directory.")
    if destination.exists():
        fail(f"Refusing to overwrite existing private QA evidence: {destination}")
    if not destination.parent.is_dir():
        fail("Create the parent QA folder explicitly before writing evidence.")
    with destination.open("x", encoding="utf-8", newline="\n") as output:
        json.dump(value, output, indent=2, ensure_ascii=False)
        output.write("\n")


def new_template(args: argparse.Namespace) -> None:
    receipt, root, _, _, receipt_hash = candidate_context(args.receipt)
    template = {
        "schema_version": SCHEMA,
        "status": "MANUAL_WIN64_QA_UNVERIFIED",
        "gameplay_verified": False,
        "receipt_sha256": receipt_hash,
        "executable_sha256": require_hash(receipt["executable_sha256"], "EXE hash"),
        "candidate_manifest_sha256": require_hash(receipt["candidate_manifest_sha256"], "Manifest hash"),
        "environment": {"windows_version": "", "cpu": "", "gpu": "", "controller": ""},
        "checks": {name: {"result": "not_tested", "notes": ""} for name in CHECKS},
        "warning": WARN,
    }
    write_new(args.output, template, root)
    print(f"Unverified manual QA checklist created: {args.output}")


def evaluate(args: argparse.Namespace) -> None:
    receipt, root, _, _, receipt_hash = candidate_context(args.receipt)
    checklist = read_json(args.checklist)
    if checklist.get("schema_version") != SCHEMA or checklist.get("status") != "MANUAL_WIN64_QA_UNVERIFIED":
        fail("Unsupported manual QA checklist schema/status.")
    if checklist.get("gameplay_verified") is not False:
        fail("A manually edited checklist cannot assert gameplay certification.")
    if require_hash(checklist.get("receipt_sha256"), "Checklist receipt hash") != receipt_hash:
        fail("QA checklist belongs to a different runtime session.")
    for key in ("executable_sha256", "candidate_manifest_sha256"):
        if require_hash(checklist.get(key), key) != require_hash(receipt[key], key):
            fail(f"Checklist references a different game candidate: {key}")
    environment = checklist.get("environment")
    if not isinstance(environment, dict) or set(environment) != {"windows_version", "cpu", "gpu", "controller"}:
        fail("QA environment must include Windows, CPU, GPU and controller fields.")
    if any(not isinstance(v, str) or len(v) > 250 for v in environment.values()):
        fail("Environment entries must be short text.")
    checks = checklist.get("checks")
    if not isinstance(checks, dict) or set(checks) != set(CHECKS):
        fail("QA checks cannot be missing, duplicated or replaced.")
    counts = {"pass": 0, "fail": 0, "not_tested": 0}
    for name in CHECKS:
        check = checks[name]
        if not isinstance(check, dict) or set(check) != {"result", "notes"}:
            fail(f"Malformed QA check entry: {name}")
        result, notes = check["result"], check["notes"]
        if not isinstance(result, str) or result not in RESULTS:
            fail(f"Invalid manual QA result for {name}")
        if not isinstance(notes, str) or len(notes) > 2000:
            fail(f"Invalid notes for {name}")
        if result in ("pass", "fail") and len(notes.strip()) < 12:
            fail(f"Manual {result} for {name} requires a descriptive observation (at least 12 characters).")
        counts[result] += 1
    evaluation = {
        "schema_version": SCHEMA,
        "status": "SELF_REPORTED_WIN64_QA_UNVERIFIED",
        "gameplay_verified": False,
        "ready_for_release": False,
        "receipt_sha256": receipt_hash,
        "executable_sha256": receipt["executable_sha256"],
        "candidate_manifest_sha256": receipt["candidate_manifest_sha256"],
        "runtime_exit_ok": receipt["process_exit_code"] == 0,
        "runtime_log_present": receipt["unreal_log_present"],
        "startup_log_markers_pass": receipt["startup_log_markers_pass"],
        "manual_counts": counts,
        "all_checks_self_reported_pass": counts["pass"] == len(CHECKS),
        "environment": environment,
        "checks": checks,
        "warning": WARN,
        "next_gate": (
            "Independently re-run full candidate manifest policy, verify UE/UHT "
            "build and cook provenance, inspect installed hardware driving, "
            "and review issue #148 before publishing any playable game."
        ),
    }
    write_new(args.output, evaluation, root)
    print(f"Unverified self-reported QA summary created: {args.output}")
    print(f"Observed pass={counts['pass']}, fail={counts['fail']}, not_tested={counts['not_tested']}.")
    print("This never certifies a playable game or enables a release.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for command in ("init", "evaluate"):
        step = commands.add_parser(command)
        step.add_argument("--receipt", type=Path, required=True, help="Private JSON receipt from Run-WindowsPlaytest.ps1")
        step.add_argument("--output", type=Path, required=True, help="New JSON path outside the candidate game")
        if command == "evaluate":
            step.add_argument("--checklist", type=Path, required=True, help="Edited private manual test checklist")
    args = parser.parse_args()
    try:
        (new_template if args.command == "init" else evaluate)(args)
        return 0
    except (ValueError, OSError, TypeError, KeyError) as error:
        print(f"Private Win64 QA review rejected: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
