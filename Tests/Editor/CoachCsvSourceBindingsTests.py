"""Source contracts: opt-in local QA, not game source authenticity."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT/path).read_text(encoding="utf-8")


class CoachCsvSourceBindingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy=read("Tools/CoachTelemetry/CoachCsvPolicy.h")
        cls.executable=read("Tools/CoachTelemetry/CoachTelemetryInspector.cpp")
        cls.build=read("Tests/Standalone/CMakeLists.txt")
        cls.regressions=read("Tests/Standalone/CoachCsvPolicyTests.cpp")

    def test_csv_parser_uses_unchanged_existing_race_chassis_stats(self):
        self.assertIn('#include "Racing/DrivingCoachIntegrity.h"',self.policy)
        self.assertIn("DrivingSessionStats staged;",self.policy)
        self.assertIn("staged.Observe({dt,velocity,lateral",self.policy)
        self.assertIn("staged.Suspend();",self.policy)
        self.assertIn("VerifyDrivingCoachIntegrity(staged)",self.policy)
        self.assertIn("destination=staged;",self.policy)
        self.assertLess(self.policy.index("if(!report.Integrity.Valid)"),
                        self.policy.index("destination=staged;"),
                        "reject invalid telemetry before replacing the output")
        self.assertIn("BuildDrivingCoachFocusAreas(snapshot)",self.policy)
        self.assertIn("BuildDrivingCoachGoals(snapshot)",self.policy)

    def test_file_permissions_security_and_authenticity_disclosure(self):
        self.assertIn("MaximumLineBytes=128",self.policy)
        self.assertIn("MaximumTotalBytes=16u*1024u*1024u",self.policy)
        self.assertIn("MaximumDataRows=200000u",self.policy)
        self.assertIn("SourceIsAuthenticated=false",self.policy)
        self.assertIn("NativeUnrealVerified=false",self.policy)
        self.assertIn("SaveDataWritten=false",self.policy)
        self.assertIn("fs::is_symlink(path,error)",self.executable)
        self.assertIn("fs::is_regular_file(path,error)",self.executable)
        self.assertIn("std::ios::binary",self.executable)
        self.assertIn('"saved_data_written\\\": false',self.executable)
        self.assertNotIn("std::ofstream",self.executable)

    def test_standalone_and_abuse_testing_in_portable_matrix(self):
        self.assertIn("CoachCsvPolicyTests.cpp",self.build)
        self.assertIn("CoachTelemetryInspector.cpp",self.build)
        self.assertIn("NAME CoachCsvPolicy",self.build)
        self.assertIn("SeededRegression",self.regressions)
        self.assertIn("FailClosedAndTransactional",self.regressions)


if __name__=="__main__":
    unittest.main()
