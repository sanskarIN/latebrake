"""Static source contract between v0.5.31 Unreal local QA logs and parser.

These tests assert that the real Unreal actor emits the new optional
invalid-velocity count only in opt-in non-shipping diagnostics. They do not
compile Unreal, capture installed-game logs or authenticate QA evidence.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
ACTOR = ROOT / "Source/Latebrake/Private/Racing/LatebrakeCheckpoint.cpp"
PARSER = ROOT / "scripts/AnalyzeCheckpointQA.py"


class InvalidVelocityQASourceContractsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.actor = ACTOR.read_text(encoding="utf-8")
        cls.parser = PARSER.read_text(encoding="utf-8")

    def test_opt_in_shipping_excluded_diagnostics(self):
        self.assertIn("#if !UE_BUILD_SHIPPING", self.actor)
        self.assertIn('TEXT("latebrake.CheckpointDiagnostics")', self.actor)
        self.assertIn("if (DiagnosticLevel == 2)", self.actor)

    def test_actor_emits_new_label_with_matching_actual_counter(self):
        self.assertIn('"invalid-velocity=%u"', self.actor)
        self.assertIn("CircuitGateDecision::InvalidVelocity));", self.actor)
        label = self.actor.index('"invalid-velocity=%u"')
        value = self.actor.index("CircuitGateDecision::InvalidVelocity));")
        self.assertLess(label, value)

    def test_parser_accepts_old_without_mislabeling_missing_counter(self):
        self.assertIn('("invalid_velocity", "invalid-velocity")', self.parser)
        self.assertIn('(?P<invalid_velocity>', self.parser)
        self.assertIn('if match.group(name) is not None else 0', self.parser)


if __name__ == "__main__":
    unittest.main()
