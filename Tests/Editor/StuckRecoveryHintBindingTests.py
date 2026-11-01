"""Source contract for the advisory-only player recovery hint.

Portable C++ tests cover the policy. This source check does not run Unreal
Editor, compile UHT, package a Win64 game or validate installed driving.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class StuckRecoveryHintBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.hud_header = read("Source/Latebrake/Public/UI/LatebrakeRaceHUD.h")
        cls.pawn = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.policy = read("Source/Latebrake/Public/Racing/StuckRecoveryHint.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_physics_eligible_player_intent_not_punished(self):
        for marker in ("bool IsAttemptingForwardDrive() const noexcept",
                       "Throttle > 0.55f", "TouchDrive.ThrottleActive()",
                       "!bBraking", "!TouchDrive.BrakeActive()",
                       "!bHandbrakeHeld"):
            self.assertIn(marker, self.pawn)
        draw = self.hud.split("void ALatebrakeRaceHUD::DrawHUD()", 1)[1]
        for marker in ("Mode->IsRaceStartGreen()",
                       "Mode->HasLocalPlayerDriver()",
                       "Mode->IsRestartPlacementReady()",
                       "Car->IsRacePhysicsReady()",
                       "Car->IsAttemptingForwardDrive()",
                       "Mode->GetCheckpointSessionRevision()",
                       "Car->GetTeleportRevision()",
                       "StuckHint.Reset();",
                       "StuckHint.Observe({", "PRESS R TO RECOVER",
                       "TAP RECOVER"):
            self.assertIn(marker, draw)
        self.assertIn("StuckRecoveryHint StuckHint;", self.hud_header)

    def test_advisory_has_no_gameplay_or_saved_authority(self):
        for forbidden in ("TryRecoverTo", "RecoverPlayer",
                          "SaveRecords", "RegisterCheckpoint",
                          "AddForce", "Award", "GetWorld"):
            self.assertNotIn(forbidden, self.policy)
        for marker in ("std::isfinite(sample.DeltaSeconds)",
                       "std::hypot(", "MaximumAnchorDisplacementCm",
                       "RequiredSeconds = 4.0", "void Reset()"):
            self.assertIn(marker, self.policy)
        self.assertIn("StuckRecoveryHintTests.cpp", self.cmake)
        self.assertIn("NAME StuckRecoveryHint", self.cmake)
        self.assertIn("/W4 /WX", self.cmake)


if __name__ == "__main__":
    unittest.main()
