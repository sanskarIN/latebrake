#!/usr/bin/env python3
"""Count authored tracked LateBrake source without confusing tests with game.

Counts physical, nonblank and nonblank/noncomment C++ lines separately.
The v0.6.0 quantity gate is opt-in; source size never proves playability.
"""
from __future__ import annotations

import argparse
import bisect
import json
import hashlib
import re
from pathlib import Path, PurePosixPath
import subprocess

GOAL_LINES = 200_000
CATEGORIES = {
    "game_source": ("Source/", frozenset({".h", ".hpp", ".cpp", ".cc", ".cxx", ".inl"})),
    "unreal_build_rules": ("Source/", frozenset({".cs"})),
    "experimental_preview": ("Experiments/", frozenset({".h", ".cpp", ".py", ".ps1"})),
    "test_code": ("Tests/", frozenset({".h", ".cpp", ".py"})),
    "automation_code": ("scripts/", frozenset({".py", ".ps1"})),
    "ci_workflows": (".github/workflows/", frozenset({".yml", ".yaml"})),
}

def count_code_lines(text: str, *, csharp: bool = False) -> int:
    """Count physical lines with noncomment tokens, preserving literal content.

    This is lexical counting, not a compiler or a claim of useful behavior.
    C++ line splices are removed with physical-line mapping before scanning.
    Strings/characters, raw literals and C# verbatim strings protect comment
    delimiters inside literal data. Blank raw-string lines still do not count.
    """
    text = text.removeprefix("\ufeff").replace("\r\n", "\n").replace("\r", "\n")
    chars: list[str] = []
    origins: list[int] = []
    positions: list[int] = []
    line = 0
    i = 0
    while i < len(text):
        if not csharp and text.startswith("\\\n", i):
            i += 2
            line += 1
            continue
        chars.append(text[i])
        origins.append(line)
        positions.append(i)
        if text[i] == "\n":
            line += 1
        i += 1
    source = "".join(chars)
    code_lines: set[int] = set()

    def mark(begin: int, end: int) -> None:
        for j in range(begin, end):
            if not source[j].isspace():
                code_lines.add(origins[j])

    i = 0
    while i < len(source):
        if source.startswith("//", i):
            end = source.find("\n", i + 2)
            i = len(source) if end < 0 else end + 1
        elif source.startswith("/*", i):
            end = source.find("*/", i + 2)
            i = len(source) if end < 0 else end + 2
        elif not csharp and source.startswith('R"', i):
            # C++ raw delimiters are at most 16 nonspace, non-paren,
            # non-backslash characters. Prefix u8/u/U/L was marked already.
            raw = re.match(r'R"([^\s()\\]{0,16})\(', source[i:])
            if raw is None:
                mark(i, i + 1)
                i += 1
                continue
            closing = ")" + raw.group(1) + '"'
            # C++ reverts phase-two splices INSIDE raw literals. Search
            # the original bytes there, so a spliced fake terminator cannot
            # expose literal // text as a comment or drop a backslash line.
            original_start = positions[i]
            original_open = re.match(r'R"([^\s()\\]{0,16})\(', text[original_start:])
            if original_open is not None:
                original_end = text.find(closing, original_start + original_open.end())
                original_end = (len(text) if original_end < 0 else
                                original_end + len(closing))
                for offset, literal_line in enumerate(
                        text[original_start:original_end].split("\n")):
                    if literal_line.strip():
                        code_lines.add(origins[i] + offset)
                i = bisect.bisect_left(positions, original_end)
            else:
                end = source.find(closing, i + raw.end())
                end = len(source) if end < 0 else end + len(closing)
                mark(i, end)
                i = end
        elif csharp and source.startswith('@"', i):
            end = i + 2
            while end < len(source):
                if source.startswith('""', end):
                    end += 2
                elif source[end] == '"':
                    end += 1
                    break
                else:
                    end += 1
            mark(i, end)
            i = end
        elif csharp and source.startswith('"""', i):
            quotes = len(source[i:]) - len(source[i:].lstrip('"'))
            end = source.find('"' * quotes, i + quotes)
            end = len(source) if end < 0 else end + quotes
            mark(i, end)
            i = end
        elif source[i].isdigit() or (source[i] == "." and
                i + 1 < len(source) and source[i + 1].isdigit()):
            # Digit separators in 1'000 and 0xF'F are not char literals.
            end = i + 1
            while end < len(source):
                c = source[end]
                if c.isalnum() or c in "_.'":
                    end += 1
                elif c in "+-" and source[end - 1] in "eEpP":
                    end += 1
                else:
                    break
            mark(i, end)
            i = end
        elif source[i] in "\"'":
            quote = source[i]
            end = i + 1
            while end < len(source):
                if source[end] == "\\":
                    end = min(len(source), end + 2)
                elif source[end] == quote:
                    end += 1
                    break
                else:
                    end += 1
            mark(i, end)
            i = end
        else:
            mark(i, i + 1)
            i += 1
    return len(code_lines)

def category(relative: str) -> str | None:
    """Use exact paths/extensions; docs and non-game assets never inflate LOC."""
    rel = PurePosixPath(relative)
    if relative.startswith("/") or ".." in rel.parts or "\\" in relative:
        raise ValueError(f"Unsafe tracked path: {relative!r}")
    # Explicit generated/vendor/test folders cannot count as original game
    # implementation even if accidentally committed under Source/.
    excluded = {"generated", "intermediate", "binaries", "saved", "build",
                "thirdparty", "third_party", "vendor", "vendored", "tests", "test"}
    if relative.startswith("Source/") and (
        any(part.lower() in excluded for part in rel.parts[1:-1]) or
        rel.name.lower().endswith((".generated.h", ".gen.cpp"))
    ):
        return None
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
    for name in ("game_source", "unreal_build_rules"):
        totals[name]["nonblank_noncomment_lines"] = 0
    skipped_links = 0
    duplicate_game_source_files = 0
    source_digests: set[bytes] = set()
    for relative in tracked_paths(root):
        group = category(relative)
        if group is None:
            continue
        path = root / relative
        # A tracked symlink could point outside the tree. Never traverse it
        # for a size audit (nor count an unhydrated/nonexistent checkout file).
        if any(parent.is_symlink() for parent in (path, *path.parents)
               if parent != root and root in parent.parents):
            skipped_links += 1
            continue
        if not path.is_file():
            continue
        # Decode explicitly to prevent random/binary files from counting
        # as authoritative code. Original CRLF and lone final lines count.
        raw = path.read_bytes()
        text = raw.decode("utf-8")
        if group == "game_source":
            # The same authored C++ copied with Windows CRLF, legacy CR
            # line endings or a UTF-8 BOM is still the same implementation.
            # Hash the canonical Unicode text instead of raw bytes so
            # platform-specific copies cannot inflate the source milestone.
            # Do NOT strip intra-line whitespace or comments: those can
            # change macro and literal semantics and need human review.
            canonical = (
                text.removeprefix("\ufeff")
                .replace("\r\n", "\n")
                .replace("\r", "\n")
            )
            digest = hashlib.sha256(canonical.encode("utf-8")).digest()
            if digest in source_digests:
                duplicate_game_source_files += 1
                continue
            source_digests.add(digest)
        lines = text.splitlines()
        group_count = totals[group]
        group_count["files"] += 1
        group_count["physical_lines"] += len(lines)
        group_count["nonblank_lines"] += sum(bool(line.strip()) for line in lines)
        if group in ("game_source", "unreal_build_rules"):
            group_count["nonblank_noncomment_lines"] += count_code_lines(
                text, csharp=group == "unreal_build_rules")
    production = totals["game_source"]["nonblank_noncomment_lines"]
    return {
        "schema_version": 2,
        "scope": "tracked authored files only; generated outputs excluded",
        "definition": "milestone counts nonblank noncomment C++ production lines only",
        "goal_metric": "game_source.nonblank_noncomment_lines",
        "milestone": "v0.6.0 pre-release",
        "goal_game_source_lines": GOAL_LINES,
        "remaining_game_source_lines": max(0, GOAL_LINES - production),
        "goal_reached": production >= GOAL_LINES,
        "skipped_tracked_symlinks": skipped_links,
        "excluded_exact_duplicate_game_source_files": duplicate_game_source_files,
        "limitations": "lexical LOC; meaningfulness, partial duplication and runtime require review",
        "categories": totals,
    }

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path,
                        default=Path(__file__).resolve().parents[1])
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--require-milestone", action="store_true",
                        help="fail below the v0.6.0 production-code quantity gate")
    args = parser.parse_args()
    result = metrics(args.root)
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print("LateBrake authored source (tracked files, not release quality)")
        for name, values in result["categories"].items():
            print(f"{name:22} {values['files']:4} files  "
                  f"{values['physical_lines']:7} lines  "
                  f"{values['nonblank_lines']:7} nonblank" +
                  (f"  {values['nonblank_noncomment_lines']:7} noncomment"
                   if "nonblank_noncomment_lines" in values else ""))
        print(f"Game-source goal: {GOAL_LINES:,}; "
              f"remaining: {result['remaining_game_source_lines']:,}")
    # Regular source CI remains informational. v0.6.0 publication must
    # explicitly invoke this quantity check in addition to gameplay gates.
    if args.require_milestone and not result["goal_reached"]:
        raise SystemExit(1)

if __name__ == "__main__":
    main()
