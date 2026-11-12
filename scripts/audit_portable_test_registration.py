#!/usr/bin/env python3
"""Offline check that Portable*Tests.cpp actually compiles under strict CTest.

This is a registry consistency audit. It never invokes an engine, compiler,
CMake or GitHub API; passing the audit is NOT a C++ test result.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys

MAX_CMAKE_BYTES = 4 * 1024 * 1024
MAX_PORTABLE_TESTS = 128
TEST_PREFIX = "Portable"
SOURCE_PATTERN = re.compile(r"^Portable[A-Za-z0-9_]+Tests[.]cpp$")
EXECUTABLE_PATTERN = re.compile(
    r"add_executable\s*\(\s*(latebrake_[a-zA-Z0-9_]+_tests)"
    r"\s+(Portable[a-zA-Z0-9_]+Tests[.]cpp)\s*\)",
    re.MULTILINE,
)
IDENTIFIER_PATTERN = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")


class PortableTestAuditError(ValueError):
    """Invalid or inconsistent source-only test registry."""


def require_plain_file(path: Path, maximum: int) -> str:
    if path.is_symlink() or not path.is_file():
        raise PortableTestAuditError(f"missing or linked file: {path}")
    if path.stat().st_size > maximum:
        raise PortableTestAuditError(f"oversized registry input: {path}")
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise PortableTestAuditError(f"cannot read text input: {path}") from error


def audited_portable_tests(standalone: Path) -> list[Path]:
    if standalone.is_symlink() or not standalone.is_dir():
        raise PortableTestAuditError("missing or linked Tests/Standalone directory")
    tests = sorted(standalone.glob("Portable*Tests.cpp"), key=lambda p: p.name)
    if len(tests) > MAX_PORTABLE_TESTS:
        raise PortableTestAuditError("portable source fixture limit exceeded")
    names: set[str] = set()
    for path in tests:
        if not SOURCE_PATTERN.fullmatch(path.name):
            raise PortableTestAuditError(f"unrecognized portable test filename: {path.name}")
        if path.name.casefold() in names:
            raise PortableTestAuditError(f"case-colliding portable test: {path.name}")
        names.add(path.name.casefold())
        require_plain_file(path, 2 * 1024 * 1024)
    return tests


def check_registration_block(cmake: str, source: str, target: str) -> str:
    # Every portable CTest must be registered exactly once; the commands
    # need a real source file target, not only a CMake comment mentioning it.
    if not IDENTIFIER_PATTERN.fullmatch(target):
        raise PortableTestAuditError(f"invalid CMake target: {target}")
    executable = re.compile(
        r"add_executable\s*\(\s*" + re.escape(target) +
        r"\s+" + re.escape(source) + r"\s*\)"
    )
    match = executable.search(cmake)
    if not match:
        raise PortableTestAuditError(f"unregistered executable: {source}")
    after = cmake[match.end():]
    boundary = re.search(r"\badd_executable\s*\(", after)
    block = after if boundary is None else after[:boundary.start()]
    checks = {
        "include path": r"target_include_directories\s*\(\s*" +
                        re.escape(target) + r"\s+PRIVATE\s+[.][.]/[.][.]/Source/Latebrake/Public\s*\)",
        "C++20": r"target_compile_features\s*\(\s*" +
                 re.escape(target) + r"\s+PRIVATE\s+cxx_std_20\s*\)",
        "MSVC warnings": r"target_compile_options\s*\(\s*" +
                         re.escape(target) + r"\s+PRIVATE\s+/W4\s+/WX\s*\)",
        "Clang/GCC warnings": r"target_compile_options\s*\(\s*" +
                              re.escape(target) + r"\s+PRIVATE\s+-Wall\s+-Wextra\s+-Wpedantic\s+-Werror\s*\)",
    }
    for label, pattern in checks.items():
        if re.search(pattern, block) is None:
            raise PortableTestAuditError(f"{source} lacks {label}")
    test_match = re.search(
        r"add_test\s*\(\s*NAME\s+([A-Za-z0-9_]+)\s+COMMAND\s+" +
        re.escape(target) + r"\s*\)", block
    )
    if test_match is None:
        raise PortableTestAuditError(f"{source} is not a runnable CTest")
    return test_match.group(1)


def audit_portable_registration(root: Path) -> dict[str, object]:
    standalone = root / "Tests" / "Standalone"
    cmake = require_plain_file(standalone / "CMakeLists.txt", MAX_CMAKE_BYTES)
    test_paths = audited_portable_tests(standalone)
    registrations: dict[str, list[str]] = {}
    for match in EXECUTABLE_PATTERN.finditer(cmake):
        target, name = match.groups()
        registrations.setdefault(name, []).append(target)
    sources = {path.name for path in test_paths}
    for name in registrations:
        if name not in sources:
            raise PortableTestAuditError(f"CMake references a missing portable test: {name}")
    targets: set[str] = set()
    test_names: set[str] = set()
    report: list[dict[str, str]] = []
    for path in test_paths:
        entries = registrations.get(path.name, [])
        if len(entries) != 1:
            raise PortableTestAuditError(
                f"{path.name}: expected one CMake registration, found {len(entries)}"
            )
        target = entries[0]
        if target in targets:
            raise PortableTestAuditError(f"duplicate executable target: {target}")
        targets.add(target)
        name = check_registration_block(cmake, path.name, target)
        if name in test_names:
            raise PortableTestAuditError(f"duplicate CTest name: {name}")
        test_names.add(name)
        report.append({"source": path.name, "executable": target, "ctest": name})
    return {
        "schema": 1,
        "kind": "latebrake-portable-ctest-registration-source-audit",
        "source_only": True,
        "native_unreal_verified": False,
        "compiled_or_executed": False,
        "portable_tests": report,
        "registered_count": len(report),
        "warning": "Registry consistency only; run strict portable CTest and real UE QA separately.",
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root", type=Path, default=Path(__file__).resolve().parents[1],
        help="LateBrake repository root; only reads Tests/Standalone",
    )
    arguments = parser.parse_args(argv)
    try:
        report = audit_portable_registration(arguments.root)
    except (PortableTestAuditError, OSError) as error:
        print(f"Portable CTest registry FAIL: {error}", file=sys.stderr)
        return 2
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
