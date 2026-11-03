"""Checks race HUD wiring without pretending to run Unreal or Chaos."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class RivalSameSegmentOrderBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Racing/RivalSameSegmentOrder.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.tests = read("Tests/Standalone/RivalSameSegmentOrderTests.cpp")

    def test_policy_refuses_ambiguous_geometry_or_gate_authority(self):
        for required in (
            "RivalGateGap.h", "CircuitLayout.h", "RaceActive",
            "SameCreditedGateCount", "CreditedGateDelta != 0",
            "IsCircuitGate(sample.NextGate)", "IsOnPavedSurface(",
            "PlayerCreditedGates % CircuitGateCount",
            "along < length * 0.035", "along > length * 0.965",
            "lateral > 500.0", "std::abs(deltaMeters) < 3.0",
        ):
            self.assertIn(required, self.policy)
        for forbidden in (
            "UCLASS", "AddForce(", "AwardCredits(", "PassCheckpoint(",
            "SaveRecords(", "SetActorLocation(", "SetPhysics",
        ):
            self.assertNotIn(forbidden, self.policy)

    def test_read_only_hud_uses_physical_locations_and_authorized_gate_counts(self):
        section = self.hud.split(
            "// Read-only discrete comparison of already accepted directional gate",
            1)[1].split("// Optional SAME-GATE verified split timing", 1)[0]
        for required in (
            "!bCompact", "VerticalPlan.MaxTelemetryLines >= 28",
            "bRivalPositionVerified",
            "Mode->TryGetRivalCreditedGateGap(GateGap)",
            "CompareRivalSameSegment({",
            "Race.GetNextCheckpoint()", "WayfindingPosition.X",
            "WayfindingPosition.Y", "RivalWorldPosition.X",
            "RivalWorldPosition.Y", "bWayfindingEligible",
            "RivalSegmentLeader::Player", "RivalSegmentLeader::Rival",
            "NOT RACE PLACE", "SAME VERIFIED COUNT",
        ):
            self.assertIn(required, section)
        for forbidden in ("SetActor", "PassCheckpoint(", "SaveRecords(",
                          "Chassis->", "AwardCredits("):
            self.assertNotIn(forbidden, section)

    def test_portable_strict_compiler_and_frame_fixtures(self):
        self.assertIn("RivalSameSegmentOrderTests.cpp", self.cmake)
        self.assertIn("NAME RivalSameSegmentOrder", self.cmake)
        self.assertIn("latebrake_rival_same_segment_tests PRIVATE /W4 /WX",
                      self.cmake)
        self.assertIn("20000", self.tests)
        self.assertIn("std::numeric_limits<double>::quiet_NaN()", self.tests)


if __name__ == "__main__":
    unittest.main()
