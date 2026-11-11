"""Source-only binding checks for the independent racing telemetry toolkit.

No runnable Unreal Engine, licensed asset or actual Chaos sensor is
inferred by the existence of portable headers.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADERS = (
    "TelemetryMath.h",
    "TelemetryTypes.h",
    "TelemetryValidation.h",
    "TelemetryFilters.h",
    "TelemetryKinematics.h",
    "TelemetryTrack.h",
    "TelemetryBraking.h",
    "TelemetryCornering.h",
    "TelemetryGrip.h",
    "TelemetryOpponents.h",
)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class RaceEngineerTelemetryBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sources = {
            name: read("Source/Latebrake/Public/Telemetry/" + name)
            for name in HEADERS
        }
        cls.tests = read("Tests/Standalone/RaceEngineerTelemetryTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.doc = read("docs/RACE_ENGINEER_TELEMETRY_QA.md")
        cls.nav = read("mkdocs.yml")

    def test_all_ten_production_components_exist_and_are_linked(self):
        self.assertEqual(len(self.sources), 10)
        for name in HEADERS:
            with self.subTest(header=name):
                self.assertIn('#pragma once', self.sources[name])
                self.assertIn('#include "Telemetry/' + name + '"',
                              self.tests)
                self.assertIn(name, self.doc)
        self.assertIn("RaceEngineerTelemetryTests.cpp", self.cmake)
        self.assertIn("NAME RaceEngineerTelemetry", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)
        self.assertIn("-Wall -Wextra -Wpedantic -Werror", self.cmake)

    def test_source_only_does_not_modify_gameplay_authority(self):
        forbidden = (
            "UCLASS(", "UPROPERTY(", "GENERATED_BODY()",
            "SpawnActor(", "SetActorLocation(", "AddForce(",
            "AddTorque(", "SaveRecords(", "PassCheckpoint(",
            "GrantCredits(", "SetAutonomousControls(",
            "GetWorld(", "UnrealEditor.exe",
        )
        for name, source in self.sources.items():
            with self.subTest(header=name):
                for token in forbidden:
                    self.assertNotIn(token, source)
        self.assertIn("NOT a claim", self.doc)
        self.assertIn("source-only", self.doc.lower())
        self.assertIn("RACE_ENGINEER_TELEMETRY_QA.md", self.nav)

    def test_sensor_integrity_and_real_track_are_explicit(self):
        self.assertIn("WrongSession", self.sources["TelemetryTypes.h"])
        self.assertIn("ClockReversed", self.sources["TelemetryValidation.h"])
        self.assertIn("ImpossibleJump", self.sources["TelemetryValidation.h"])
        self.assertIn("SaturatingIncrement", self.sources["TelemetryMath.h"])
        self.assertIn("latebrake::EastX", self.sources["TelemetryTrack.h"])
        self.assertIn("StartNewSegment", self.sources["TelemetryTypes.h"])
        self.assertIn("Throttle", self.sources["TelemetryBraking.h"])
        self.assertIn("WheelContactFraction", self.sources["TelemetryTypes.h"])
        self.assertIn("DetermineCornerDirection", self.sources["TelemetryCornering.h"])
        self.assertIn("EvaluateGripTelemetry", self.sources["TelemetryGrip.h"])
        self.assertIn("EvaluateOpponentEncounter",
                      self.sources["TelemetryOpponents.h"])
        self.assertIn("TestLargeDeterministicFrameSweep()", self.tests)
        self.assertIn("i <= 30000", self.tests)

    def test_count_cannot_include_qa_in_production_lines(self):
        counter = read("scripts/MeasureSourceScale.py")
        self.assertIn('"game_source"', counter)
        self.assertIn('"test_code"', counter)
        self.assertIn("nonblank_noncomment_lines", counter)
        self.assertIn("174,840", self.doc)
        self.assertIn("+5,359", self.doc)


if __name__ == "__main__":
    unittest.main()
