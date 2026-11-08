#!/usr/bin/env python3
"""Count authored tracked LateBrake source without confusing tests with game.

Counts physical and nonblank lines. This is NOT a quality, build, or
release gate and makes no claim about shipping a playable game.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path, PurePosixPath
import subprocess

GOAL_LINES = 200_000
CATEGORIES = {
    "game_source": ("Source/", frozenset({".h", ".cpp", ".cs"})),
    "experimental_preview": ("Experiments/", frozenset({".h", ".cpp", ".py", ".ps1"})),
    "test_code": ("Tests/", frozenset({".h", ".cpp", ".py"})),
    "automation_code": ("scripts/", frozenset({".py", ".ps1"})),
    "ci_workflows": (".github/workflows/", frozenset({".yml", ".yaml"})),
}

def category(relative: str) -> str | None:
    """Use exact paths/extensions; docs and non-game assets never inflate LOC."""
    rel = PurePosixPath(relative)
    if relative.startswith("/") or ".." in rel.parts or "\\" in relative:
        raise ValueError(f"Unsafe tracked path: {relative!r}")
    for name, (prefix, extensions) in CATEGORIES.items():
        if relative.startswith(prefix) and rel.suffix.lower() in extensions:
            return name
    return None

def tracked_paths(root: Path) -> list[str]:
    output = subprocess.run(
        ["git", "-C", str(root), "ls-files", "--cached", "-z"],
        check=True, capture_output=True,
    ).stdout
    return [entry.decode("utf-8") for entry in output.split(b"\0") if entry]

def metrics(root: Path) -> dict:
    root = root.resolve(strict=True)
    totals: dict[str, dict[str, int]] = {
        name: {"files": 0, "physical_lines": 0, "nonblank_lines": 0}
        for name in CATEGORIES
    }
    skipped_links = 0
    for relative in tracked_paths(root):
        group = category(relative)
        if group is None:
            continue
        path = root / relative
        # A tracked symlink could point outside the tree. Never traverse it
        # for a size audit (nor count an unhydrated/nonexistent checkout file).
        if path.is_symlink():
            skipped_links += 1
            continue
        if not path.is_file():
            continue
        # Decode explicitly to prevent random/binary files from counting
        # as authoritative code. Original CRLF and lone final lines count.
        text = path.read_bytes().decode("utf-8")
        lines = text.splitlines()
        group_count = totals[group]
        group_count["files"] += 1
        group_count["physical_lines"] += len(lines)
        group_count["nonblank_lines"] += sum(bool(line.strip()) for line in lines)
    production = totals["game_source"]["physical_lines"]
    return {
        "schema_version": 1,
        "scope": "tracked authored files only; generated outputs excluded",
        "definition": "physical source lines; nonblank includes comments",
        "goal_game_source_lines": GOAL_LINES,
        "remaining_game_source_lines": max(0, GOAL_LINES - production),
        "goal_reached": production >= GOAL_LINES,
        "skipped_tracked_symlinks": skipped_links,
        "categories": totals,
    }

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path,
                        default=Path(__file__).resolve().parents[1])
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    result = metrics(args.root)
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print("LateBrake authored source (tracked files, not release quality)")
        for name, values in result["categories"].items():
            print(f"{name:22} {values['files']:4} files  "
                  f"{values['physical_lines']:7} lines  "
                  f"{values['nonblank_lines']:7} nonblank")
        print(f"Game-source goal: {GOAL_LINES:,}; "
              f"remaining: {result['remaining_game_source_lines']:,}")
    # Intentional exit 0 regardless of target. LOC is NOT a merge gate.

if __name__ == "__main__":
    main()
