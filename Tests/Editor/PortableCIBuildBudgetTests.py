"""Source-only CI build-budget contract. No Unreal engine build is implied."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CORE = (ROOT / ".github/workflows/core-tests.yml").read_text(encoding="utf-8")
PUBLISHER = (ROOT / ".github/workflows/publish-v0.5.89-source.yml").read_text(
    encoding="utf-8"
)


class PortableCIBuildBudgetTests(unittest.TestCase):
    def test_portable_matrix_remains_complete_and_bounded(self):
        for marker in (
            "os: [ubuntu-latest, windows-latest, macos-latest]",
            "cmake --build Tests/Standalone/build --config Release --parallel 4",
            "ctest --test-dir Tests/Standalone/build -C Release --output-on-failure",
            "Prepare optional macOS compiler cache",
            "Restore macOS compiler object cache",
            "Report macOS cache statistics",
            "name: Unreal editor-only Python syntax (not Unreal runtime)",
        ):
            self.assertIn(marker, CORE)
        self.assertNotIn("cmake --build Tests/Standalone/build --config Release --parallel\n", CORE)

    def test_published_versioned_release_is_not_edited_to_fix_performance(self):
        # Versioned publishers belong to their immutable source snapshots.
        # Rewriting the old release after publication can cause a stale
        # tag/HEAD identity check to fail in a completely unrelated run.
        self.assertIn("const tag = 'v0.5.89';", PUBLISHER)
        self.assertIn("if (main !== sha)", PUBLISHER)
        self.assertIn("existingRef.sha !== sha", PUBLISHER)
        self.assertIn("published.assets.length !== 0", PUBLISHER)
        self.assertIn("needs: [source-tests, editor-tests, packaging-policy, documentation]",
                      PUBLISHER)


if __name__ == "__main__":
    unittest.main()
