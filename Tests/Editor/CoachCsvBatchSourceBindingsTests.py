"""Non-runtime source contracts for batch CSV diagnostics and their CLI wiring.

These tests can catch accidental bypasses of the standalone validator or
incorrectly presented provenance, but never certify real Unreal sessions.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class CoachCsvBatchSourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.batch = read("Tools/CoachTelemetry/CoachCsvBatch.h")
        cls.cli = read("Tools/CoachTelemetry/CoachTelemetryInspector.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.tests = read("Tests/Standalone/CoachCsvBatchTests.cpp")

    def test_batch_uses_shared_independently_validated_samples(self):
        for expected in ("#include \"CoachCsvQuality.h\"",
                         "BuildCoachCsvBatch(",
                         "BuildCoachCsvQualityReview(",
                         "GetCoachReview()", "MaximumCoachBatchFiles",
                         "RepeatedFingerprintPairs", "AllClaimSameContext"):
            with self.subTest(expected=expected):
                self.assertIn(expected, self.batch)
        self.assertNotIn("SaveRecords(", self.batch)
        self.assertNotIn("AwardCredits(", self.batch)

    def test_cli_validates_every_file_before_any_batch_output(self):
        process = self.cli.split("int main(int argc,char** argv)", 1)[1]
        self.assertIn('std::string_view(argv[1])=="--batch"', process)
        self.assertIn("coachcsv::MaximumCoachBatchFiles", process)
        self.assertIn("LoadCsvFile(argv[index+2]", process)
        self.assertLess(process.index("LoadCsvFile(argv[index+2]"),
                        process.index("PrintBatch(report,pairs)"))
        self.assertIn("ValidCoachCsvBatchReview(report)", process)
        self.assertIn("return 3;", process)
        self.assertIn("return 64;", process)
        self.assertIn(r'\"source_authenticated\": false', self.cli)
        self.assertIn(r'\"official_race_ranking\": false', self.cli)

    def test_registered_with_portable_cpp20_warnings_as_errors(self):
        self.assertIn("CoachCsvBatchTests.cpp", self.cmake)
        self.assertIn("NAME CoachCsvBatch", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)
        self.assertIn("TwoConsistentFilesHaveDescriptiveOnlyReport", self.tests)
        self.assertIn("MaximumBatchRetainsEveryFileWithoutOverflow", self.tests)
        self.assertIn("SeededInputVariationChangesStudyBounds", self.tests)

    def test_no_offline_claim_of_authenticated_built_or_ranked_play(self):
        for expected in ("EstablishesAuthenticSourceFiles=false",
                         "EstablishesIdenticalRaceConditions=false",
                         "RanksOfficialRacePerformance=false",
                         "CanModifyGameProfile=false"):
            self.assertIn(expected, self.batch)
        for forbidden in ("FPlatformTime", "UGameplayStatics", "LoadGameFromSlot",
                          "SaveGameToSlot", "UE_LOG", "std::ofstream"):
            self.assertNotIn(forbidden, self.batch)
            self.assertNotIn(forbidden, self.tests)


if __name__ == "__main__":
    unittest.main()
