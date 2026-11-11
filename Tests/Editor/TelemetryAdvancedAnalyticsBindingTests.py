"""Read-only source contracts for 22 additional LateBrake telemetry modules."""
from pathlib import Path
import hashlib
import unittest

ROOT = Path(__file__).resolve().parents[2]
NAMES = (
    "TelemetrySessionRecorder",
    "TelemetryReplaySampler",
    "TelemetrySignalSeries",
    "TelemetryLapJournal",
    "TelemetrySectorMatrix",
    "TelemetryLapComparison",
    "TelemetryDistancePace",
    "TelemetryBrakeTrace",
    "TelemetryRaceLineProfile",
    "TelemetryCornerSequence",
    "TelemetryFuelStint",
    "TelemetryTireStint",
    "TelemetryWeatherTrend",
    "TelemetryPitReview",
    "TelemetryVehicleBalance",
    "TelemetrySuspensionReview",
    "TelemetryTransmissionReview",
    "TelemetryIncidentJournal",
    "TelemetryFleetGapReview",
    "TelemetryDriverCoach",
    "TelemetrySessionIntegrity",
    "TelemetryReplayBookmarks",
)


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class TelemetryAdvancedAnalyticsBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.modules = {
            name: read("Source/Latebrake/Public/Telemetry/" + name + ".h")
            for name in NAMES
        }
        cls.advanced = read(
            "Tests/Standalone/TelemetryAdvancedAnalyticsTests.cpp")
        cls.extended = read(
            "Tests/Standalone/TelemetryExtendedSubsystemTests.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.docs = read("docs/RACE_ENGINEER_ADVANCED_ANALYTICS_QA.md")
        cls.nav = read("mkdocs.yml")

    def test_all_22_unique_substantive_headers_exist(self):
        hashes = set()
        self.assertEqual(len(self.modules), 22)
        for name, source in self.modules.items():
            with self.subTest(module=name):
                self.assertIn("#pragma once", source)
                self.assertIn("namespace latebrake::engineering", source)
                self.assertGreater(len(source.splitlines()), 200)
                digest = hashlib.sha256(source.encode()).digest()
                self.assertNotIn(digest, hashes)
                hashes.add(digest)
                self.assertIn(name + ".h", self.docs)

    def test_all_modules_are_compiled_with_portable_cxx20(self):
        for name in NAMES:
            with self.subTest(module=name):
                include = '#include "Telemetry/' + name + '.h"'
                self.assertTrue(
                    include in self.advanced or include in self.extended)
        for marker in (
            "TelemetryAdvancedAnalyticsTests.cpp",
            "TelemetryExtendedSubsystemTests.cpp",
            "NAME TelemetryAdvancedAnalytics",
            "NAME TelemetryExtendedSubsystem",
            "cxx_std_20", "/W4 /WX",
            "-Wall -Wextra -Wpedantic -Werror",
        ):
            self.assertIn(marker, self.cmake)
        self.assertIn("RACE_ENGINEER_ADVANCED_ANALYTICS_QA.md", self.nav)

    def test_analytic_source_never_writes_gameplay_authority(self):
        forbidden = (
            "UCLASS(", "UFUNCTION(", "GENERATED_BODY(",
            "SpawnActor(", "SetActorLocation(", "AddForce(",
            "AddTorque(", "GrantCredits(", "PassCheckpoint(",
            "SaveGameToSlot(", "SetAutonomousControls(",
        )
        for name, source in self.modules.items():
            for marker in forbidden:
                with self.subTest(module=name, marker=marker):
                    self.assertNotIn(marker, source)
        for marker in ("25,165", "35,257", "+10,092",
                       "164,743", "source-only", "Win64",
                       "UHT", "issue #148"):
            self.assertIn(marker.lower(), self.docs.lower())

    def test_fail_closed_measured_evidence_contracts(self):
        markers = {
            "TelemetrySessionRecorder": "RecordingStopReason::ActorChanged",
            "TelemetryReplaySampler": "ReplayFrameIdentityMatches(",
            "TelemetrySignalSeries": "ThresholdCrossings(",
            "TelemetryLapJournal": "Settings.RequireForwardGateEvidence",
            "TelemetrySectorMatrix": "MostInconsistentSector(",
            "TelemetryLapComparison": "LapDeltaQuality::MixedConditions",
            "TelemetryDistancePace": "Settings.RejectReverseMovement",
            "TelemetryBrakeTrace": "ClassifyBrakeTrace(",
            "TelemetryRaceLineProfile": "MinimumRoadEdgeMarginCm",
            "TelemetryCornerSequence": "result.BrakeLinked",
            "TelemetryFuelStint": "Settings.AllowPitLaneRefuel",
            "TelemetryTireStint": "Settings.RequireFourContactSensors",
            "TelemetryWeatherTrend": "PhysicallyMeasuredWetness",
            "TelemetryPitReview": "result.IssuedPitCommand = false",
            "TelemetryVehicleBalance": "PhysicalWheelContact",
            "TelemetrySuspensionReview": "FourWheelSuspensionAvailable",
            "TelemetryTransmissionReview": "TransmissionEngaged",
            "TelemetryIncidentJournal": "settings.RequirePhysicalContactForCollision",
            "TelemetryFleetGapReview": "settings.RequireExactSixMemberRoster",
            "TelemetryDriverCoach": "c.CommandsVehicle = false",
            "TelemetrySessionIntegrity": "SessionIntegrityFault::DuplicateFrame",
            "TelemetryReplayBookmarks": "clip.ExportedMovie = false",
        }
        for name, marker in markers.items():
            with self.subTest(module=name):
                self.assertIn(marker, self.modules[name])


if __name__ == "__main__":
    unittest.main()
