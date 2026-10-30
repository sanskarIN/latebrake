"""Combined arcade acceleration source contract; no Unreal compilation."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class ArcadeBoostArbitrationBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Vehicles/ArcadeBoostArbitration.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_source_policy_requires_finite_cap_and_nitro_priority(self):
        for fragment in ("CombinedMaxExtra = 0.40", "NitroMaxExtra = 0.38",
                         "DraftMaxExtra = 0.06", "std::isfinite(",
                         "std::clamp(", "std::min(", "nitroActive",
                         "requestedDraftExtra"):
            self.assertIn(fragment, self.policy)
        for forbidden in ("AddForce(", "SaveGame", "AwardCredits", "RaceProgress",
                          "UCLASS(", "PassCheckpoint(", "UE_LOG("):
            self.assertNotIn(forbidden, self.policy)

    def test_combined_production_force_path(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        for fragment in (
            '#include "Vehicles/ArcadeBoostArbitration.h"',
            "NitroBoost.Step(", "SlipstreamBoost.Observe({",
            "AllocateArcadeBoost(", "NitroDrive.ForceMultiplier",
            "DraftExtraForce, NitroDrive.Active",
            "DraftExtraForce = CombinedBoost.DraftExtra;",
            "CurrentDraftAppliedFraction = static_cast<float>(DraftExtraForce)",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
            "static_cast<float>(CombinedBoost.NitroExtra)",
            "static_cast<float>(DraftExtraForce)",
        ):
            self.assertIn(fragment, self.pawn if fragment.startswith("#include") else tick)
        self.assertEqual(tick.count("static_cast<float>(Surface.MotorMultiplier)"), 3)
        self.assertLess(tick.index("NitroBoost.Step("),
                        tick.index("AllocateArcadeBoost("))
        self.assertLess(tick.index("SlipstreamBoost.Observe({"),
                        tick.index("AllocateArcadeBoost("))
        self.assertLess(tick.index("AllocateArcadeBoost("),
                        tick.index("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *"))

    def test_report_only_actual_force_allocation_and_clear_on_reset(self):
        self.assertIn("GetAppliedDraftBoostFraction() const", self.pawn_h)
        self.assertIn("CurrentDraftAppliedFraction = 0.f;", self.pawn_h)
        self.assertIn("GetAppliedDraftBoostFraction()", self.hud)
        self.assertIn("GetAppliedDraftBoostFraction() > 0.f", self.hud)
        self.assertNotIn("GetSlipstreamBoost().ExtraForce() * 100.0", self.hud)
        clear = self.pawn.split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        self.assertIn("CurrentDraftAppliedFraction = 0.f;", clear)
        self.assertIn("NAME ArcadeBoostArbitration", self.cmake)
        self.assertIn("ArcadeBoostArbitrationTests.cpp", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)


if __name__ == "__main__":
    unittest.main()
