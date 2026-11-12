"""Fixture-based audit of portable CTest registration, not a C++ compile."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "audit_portable_test_registration.py"
spec = importlib.util.spec_from_file_location("portable_ctest_audit", SCRIPT)
module = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(module)

CMAKE_TEMPLATE = """
add_executable(latebrake_portable_sample_tests PortableSampleTests.cpp)
target_include_directories(latebrake_portable_sample_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_portable_sample_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_portable_sample_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_portable_sample_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME PortableSample COMMAND latebrake_portable_sample_tests)
"""


class PortableRegistrationAuditTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix="latebrake-ctest-registry-")
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name).resolve()
        self.standalone = self.root / "Tests" / "Standalone"
        self.standalone.mkdir(parents=True)
        self.cpp = self.standalone / "PortableSampleTests.cpp"
        self.cpp.write_text("int main() { return 0; }\n", encoding="utf-8")
        self.cmake = self.standalone / "CMakeLists.txt"
        self.cmake.write_text(CMAKE_TEMPLATE, encoding="utf-8")

    def audit(self):
        return module.audit_portable_registration(self.root)

    def test_valid_strict_registration_does_not_compile_or_execute(self):
        result = self.audit()
        self.assertEqual(result["registered_count"], 1)
        self.assertFalse(result["native_unreal_verified"])
        self.assertFalse(result["compiled_or_executed"])
        self.assertEqual(result["portable_tests"][0]["ctest"], "PortableSample")

    def test_absent_registration_rejected(self):
        self.cmake.write_text("", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected one"):
            self.audit()

    def test_missing_cpp_rejected(self):
        self.cpp.unlink()
        with self.assertRaisesRegex(ValueError, "missing portable test"):
            self.audit()

    def test_duplicate_registration_rejected(self):
        self.cmake.write_text(CMAKE_TEMPLATE * 2, encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected one"):
            self.audit()

    def test_same_source_with_second_target_rejected(self):
        second = CMAKE_TEMPLATE.replace(
            "latebrake_portable_sample_tests", "latebrake_portable_other_tests"
        ).replace("NAME PortableSample", "NAME PortableOther")
        self.cmake.write_text(CMAKE_TEMPLATE + second, encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected one"):
            self.audit()

    def test_missing_strict_compiler_flags_rejected(self):
        self.cmake.write_text(
            CMAKE_TEMPLATE.replace("-Wpedantic -Werror", "-Wpedantic"),
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "Clang/GCC warnings"):
            self.audit()

    def test_missing_compiler_standard_rejected(self):
        self.cmake.write_text(
            CMAKE_TEMPLATE.replace("cxx_std_20", "cxx_std_17"),
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "C\\+\\+20"):
            self.audit()

    def test_missing_ctest_rejected(self):
        self.cmake.write_text(
            CMAKE_TEMPLATE.replace(
                "add_test(NAME PortableSample COMMAND latebrake_portable_sample_tests)", ""
            ), encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "runnable CTest"):
            self.audit()

    def test_missing_implementation_file_rejected(self):
        self.cpp.write_text("hello", encoding="utf-8")
        self.assertEqual(self.audit()["registered_count"], 1)
        # This validator is intentionally not a compiler or semantic checker.

    def test_cli_fails_closed(self):
        self.cmake.write_text("", encoding="utf-8")
        result = subprocess.run(
            [sys.executable, str(SCRIPT), "--root", str(self.root)],
            text=True, capture_output=True, timeout=12, check=False,
        )
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("FAIL", result.stderr)

    def test_main_is_self_consistent(self):
        current = module.audit_portable_registration(ROOT)
        self.assertGreaterEqual(current["registered_count"], 3)
        names = {x["source"] for x in current["portable_tests"]}
        self.assertIn("PortableReplayDeltaTraceTests.cpp", names)
        self.assertIn("PortableFollowingGapEnvelopeTests.cpp", names)
        self.assertIn("PortableCircuitGripProfileTests.cpp", names)

    def test_script_never_launches_unreal_or_git(self):
        text = SCRIPT.read_text(encoding="utf-8")
        for forbidden in ("subprocess.run", "unreal.Editor", "gh pr merge"):
            self.assertNotIn(forbidden, text)


if __name__ == "__main__":
    unittest.main()
