"""Portable source-only imports cannot silently become UE runtime code."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class SourceOnlyImportContract(unittest.TestCase):
    def test_imported_headers_have_no_unreal_dependencies(self):
        root = ROOT / "Source/Latebrake/Public"
        for group, expected in (("Racecraft", 64), ("Telemetry", 32)):
            paths = sorted((root / group).glob("*.h"))
            self.assertGreaterEqual(len(paths), expected)
            for path in paths:
                text = path.read_text(encoding="utf-8")
                for forbidden in ('#include "CoreMinimal.h"',
                                  'UCLASS(', 'UPROPERTY(',
                                  '.generated.h"', 'GEngine->'):
                    self.assertNotIn(forbidden, text, str(path))
                self.assertIn("namespace latebrake", text)

    def test_original_fixture_registry_is_complete(self):
        registry = (ROOT / "Tests/Standalone/PortableRacecraftImport.cmake"
                    ).read_text(encoding="utf-8")
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt"
                 ).read_text(encoding="utf-8")
        self.assertIn("PortableRacecraftImport.cmake", cmake)
        for marker in ("cxx_std_20", "/W4 /WX",
                       "-Wall -Wextra -Wpedantic -Werror",
                       "RacecraftFoundations", "RacecraftExtendedSystems",
                       "PhysicalRaceOperationsEvidence",
                       "TelemetryAdvancedAnalytics",
                       "TelemetryExtendedSubsystem"):
            self.assertIn(marker, registry)
        assert registry.count("    ") > 24

if __name__ == "__main__":
    unittest.main()
