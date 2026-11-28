"""Window lineage and exact-source audit wiring, no Unreal gameplay claim."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load(path):
    return (ROOT / path).read_text(encoding="utf-8")


class CoachCsvWindowSourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = load("Tools/CoachTelemetry/CoachCsvWindowAudit.h")
        cls.cli = load("Tools/CoachTelemetry/CoachTelemetryInspector.cpp")
        cls.cmake = load("Tests/Standalone/CMakeLists.txt")
        cls.portable = load("Tests/Standalone/CoachCsvWindowAuditTests.cpp")

    def test_uses_validated_chassis_source_not_file_stamps(self):
        for key in ("BuildCoachCsvQualityReview(source,stats)",
                    "ValidCoachCsvQualityReview(quality,source,stats)",
                    "stats.GetCoachReview()", "SourceSamples", "RetainedSamples",
                    "SomeOldWindowsNotRetained", "ObservedEpochChanges",
                    "RetainedCoverageFraction", "ContinuityEpoch",
                    "ValidCoachCsvWindowRow("):
            with self.subTest(key=key):
                self.assertIn(key, self.policy)
        self.assertNotIn("GetElapsedSeconds", self.policy)
        self.assertNotIn("PassCheckpoint", self.policy)
        self.assertNotIn("SaveRecords(", self.policy)

    def test_cli_retains_only_analyzed_window_evidence(self):
        for symbol in ("#include \"CoachCsvWindowAudit.h\"",
                       "BuildCoachCsvWindowAudit(audit,stats)",
                       "ValidCoachCsvWindowAudit(windows)",
                       'PrintWindowAudit(audit,stats)',
                       "retained_window_audit", "older_windows_evicted"):
            with self.subTest(symbol=symbol):
                self.assertIn(symbol, self.cli)
        self.assertLess(self.cli.index("PrintQuality(audit,stats);"),
                        self.cli.index("PrintWindowAudit(audit,stats);"))

    def test_portable_coverage_limits_and_fuzz_exist(self):
        for name in ("ExactCompleteAndUnfinishedWindowTiming",
                     "SourceContinuityBreaksNeverCreateFakeWindows",
                     "RingEvictionRefusesWholeSessionCoverageClaim",
                     "CannotValidateFabricatedRowsOrImpossibleFlags",
                     "IndependentSampleSumAndWindowLineageFuzz"):
            self.assertIn(name, self.portable)
        self.assertIn("CoachCsvWindowAuditTests.cpp", self.cmake)
        self.assertIn("NAME CoachCsvWindowAudit", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)

    def test_no_authority_or_save_mutation(self):
        for required in ("SourceAuthenticityVerified=false",
                         "AuthenticLapTimingVerified=false",
                         "DrivesGamePhysics=false",
                         "WritesSaveData=false"):
            self.assertIn(required, self.policy)
        for forbidden in ("UnrealHeaderTool", "UGameplayStatics", "SaveGameToSlot",
                          "std::ofstream", "AwardCredits", "ApplyGarageToPlayer"):
            self.assertNotIn(forbidden, self.policy)


if __name__ == "__main__":
    unittest.main()
