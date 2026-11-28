"""Offline QA quality warnings are derived from the same approved samples.

These source-only tests cannot establish installed Unreal 5.8 authenticity.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT/relative).read_text(encoding="utf-8")


class CoachCsvQualitySourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy=read("Tools/CoachTelemetry/CoachCsvQuality.h")
        cls.parser=read("Tools/CoachTelemetry/CoachCsvPolicy.h")
        cls.cli=read("Tools/CoachTelemetry/CoachTelemetryInspector.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")

    def test_quality_observes_validated_source_only(self):
        for source in ('#include "CoachCsvPolicy.h"',
                       "BuildCoachCsvQualityReview(",
                       "VerifyDrivingCoachIntegrity(stats).Valid",
                       "ValidDrivingCoachReadout(stats.GetCoachReview())",
                       "stats.GetSpeedDistribution()",
                       "distribution.Rows", "SampledSpeedBandCount",
                       "coach.MovingFraction", "coach.HighSlipFraction"):
            self.assertIn(source,self.policy)
        self.assertIn('"CoachCsvQuality.h"',self.cli)
        self.assertIn("PrintQuality(audit,stats);",self.cli)

    def test_warnings_do_not_claim_cheating_or_valid_racing_result(self):
        for marker in ("SourceAuthenticated=false",
                       "UnrealBuildVerified=false", "RaceTimingVerified=false",
                       "CanProveRaceImprovement=false", "CanAlterGameProgress=false",
                       "ExtremelyConcentratedSpeedBand",
                       "MissingEndMarker", "PredominantlyStationary",
                       "InterruptedObservation"):
            self.assertIn(marker,self.policy)
        for forbidden in ("EarnCredits(", "PassCheckpoint(",
                          "SaveRecords(", "SetGarageTuning(",
                          "ClassifyCheater(", "FlagForBan("):
            self.assertNotIn(forbidden,self.policy)
        self.assertIn("Samples",self.parser)

    def test_source_output_is_local_and_bounded(self):
        self.assertIn("MaximumCsvQualityNotices=12",self.policy)
        self.assertIn("MaximumDataRows=200000u",self.parser)
        self.assertIn("MaximumTotalBytes=16u*1024u*1024u",self.parser)
        self.assertIn('"official_race_evidence\\\": false',self.cli)
        self.assertIn("CoachCsvQualityTests.cpp",self.cmake)
        self.assertIn("NAME CoachCsvQuality",self.cmake)


if __name__=="__main__":
    unittest.main()
