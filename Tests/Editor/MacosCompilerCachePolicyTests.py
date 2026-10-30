"""Source-only CI guards for optional macOS compiler object caching.

Tests inspect the published workflow structure, not host runner speed.
The cache is never a substitute for build+CTest or Unreal qualification.
"""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = ROOT / ".github/workflows/core-tests.yml"


class MacosCompilerCachePolicyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workflow = WORKFLOW.read_text(encoding="utf-8")
        cls.matrix = cls.workflow.split("  portable-tests:", 1)[1].split(
            "  packaging-script-syntax:", 1
        )[0]

    def test_all_original_platforms_still_compile_and_test(self):
        for phrase in (
            "os: [ubuntu-latest, windows-latest, macos-latest]",
            "fail-fast: false",
            "cmake --build Tests/Standalone/build --config Release --parallel",
            "ctest --test-dir Tests/Standalone/build -C Release --output-on-failure",
        ):
            self.assertIn(phrase, self.matrix)
        self.assertEqual(self.matrix.count("name: Test"), 1)

    def test_cache_is_optional_macos_only(self):
        for step in (
            "name: Prepare macOS compiler cache",
            "name: Restore macOS compiler object cache",
            "name: Configure macOS with reusable C++ objects",
            "name: Report macOS cache statistics",
        ):
            self.assertIn(step, self.matrix)
        self.assertEqual(self.matrix.count("if: matrix.os == 'macos-latest'"), 4)
        self.assertIn("if: matrix.os != 'macos-latest'", self.matrix)
        self.assertIn("uses: actions/cache@v4", self.matrix)
        self.assertIn("-DCMAKE_CXX_COMPILER_LAUNCHER=ccache", self.matrix)
        self.assertIn("CCACHE_MAXSIZE: 1G", self.matrix)
        self.assertIn("CCACHE_COMPRESS: 'true'", self.matrix)
        self.assertIn("ccache --show-stats", self.matrix)

    def test_cache_owns_only_portable_build_objects(self):
        for fragment in (
            "path: ${{ runner.temp }}/latebrake-ccache",
            "CCACHE_DIR: ${{ runner.temp }}/latebrake-ccache",
            "hashFiles('Tests/Standalone/CMakeLists.txt'",
            "'Tests/Standalone/*.cpp'",
            "'Source/Latebrake/Public/**/*.h'",
            "'Source/Latebrake/Private/**/*.cpp'",
        ):
            self.assertIn(fragment, self.matrix)
        self.assertNotIn("upload-artifact", self.matrix)
        self.assertNotIn("BuildCookRun", self.matrix)
        self.assertNotIn("Latebrake.exe", self.matrix)
        self.assertNotIn("  source-prerelease:", self.matrix)
        self.assertLess(self.matrix.index("Restore macOS compiler object cache"),
                        self.matrix.index("Configure macOS with reusable C++ objects"))
        self.assertLess(self.matrix.index("cmake --build Tests/Standalone/build"),
                        self.matrix.index("name: Test"))


if __name__ == "__main__":
    unittest.main()
