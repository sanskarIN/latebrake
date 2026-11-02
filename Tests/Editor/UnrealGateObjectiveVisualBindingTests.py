"""Source-level binding for the actual UE checkpoint marker.

These checks do NOT invoke licensed UE 5.8 or prove rendered game visuals.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CPP = ROOT / "Source/Latebrake/Private/Racing/LatebrakeCheckpoint.cpp"
HEADER = ROOT / "Source/Latebrake/Public/Racing/LatebrakeCheckpoint.h"
POLICY = ROOT / "Source/Latebrake/Public/Racing/GateObjectiveVisual.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class UnrealGateObjectiveVisualBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = CPP.read_text(encoding="utf-8")
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_live_checkpoint_actor_drives_cosmetic_plan(self):
        for required in (
            "PlanGateObjectiveVisual(", "Mode->GetRace().GetNextCheckpoint()",
            "Mode->GetRace().GetCheckpointCount()", "Mode->IsCircuitReadyForGameplay()",
            "Mode->GetRace().IsFinished()", "RefreshObjectiveMarker();",
            "Marker->SetVisibility(Plan.Visible);",
            "Marker->SetRelativeScale3D(", "ObjectiveBeacon->SetVisibility(",
        ):
            self.assertIn(required, self.cpp)
        self.assertIn("TObjectPtr<UPointLightComponent> ObjectiveBeacon", self.header)

    def test_actor_visuals_are_non_colliding(self):
        for marker in (
            "Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision)",
            "Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision)",
            "ObjectiveBeacon->SetVisibility(false);",
            "ObjectiveBeacon->SetCastShadows(false);",
        ):
            self.assertIn(marker, self.cpp)
        self.assertNotIn("RegisterCheckpoint(", self.policy)
        self.assertNotIn("Save", self.policy.replace("save", ""))
        self.assertIn("if (Result.Crossed())", self.cpp)
        self.assertIn("Mode->RegisterCheckpoint(CheckpointIndex);", self.cpp)

    def test_visual_policy_is_in_portable_ctest(self):
        self.assertIn("GateObjectiveVisualTests.cpp", self.cmake)
        self.assertIn("add_test(NAME GateObjectiveVisual", self.cmake)
        self.assertIn("checkpointCount != CircuitGateCount", self.policy)


if __name__ == "__main__":
    unittest.main()
