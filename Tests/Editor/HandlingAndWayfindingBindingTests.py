"""Source integration for v0.5.53 handling and HUD guidance.

These checks inspect C++ Unreal source and portable test wiring. They do
not compile the Unreal Editor, run Chaos or verify packaged touch rendering.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PAWN = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
HUD_H = ROOT / "Source/Latebrake/Public/UI/LatebrakeRaceHUD.h"
TRACTION = ROOT / "Source/Latebrake/Public/Vehicles/CornerTractionAssist.h"
WRONG = ROOT / "Source/Latebrake/Public/Racing/WrongWayIndicator.h"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"
TRACTION_TESTS = ROOT / "Tests/Standalone/CornerTractionAssistTests.cpp"
WRONG_TESTS = ROOT / "Tests/Standalone/WrongWayIndicatorTests.cpp"


class HandlingAndWayfindingBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.hud_h = HUD_H.read_text(encoding="utf-8")
        cls.traction = TRACTION.read_text(encoding="utf-8")
        cls.wrong = WRONG.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")
        cls.traction_tests = TRACTION_TESTS.read_text(encoding="utf-8")
        cls.wrong_tests = WRONG_TESTS.read_text(encoding="utf-8")

    def test_pawn_applies_assist_after_physics_velocity_guard(self):
        self.assertIn('#include "Vehicles/CornerTractionAssist.h"', self.pawn)
        tick = self.pawn.split(
            "void ALatebrakeCarPawn::Tick(float DeltaSeconds)", 1
        )[1].split("bool ALatebrakeCarPawn::IsRacePhysicsReady()", 1)[0]
        for text in (
            "const float RawThrottle = SmoothedThrottle;",
            "const float LongitudinalSpeed =",
            "const float LateralSpeed =",
            "latebrake::StabilizeCornerThrottle(",
            "SelectHandbrakeDriveProfile(",
            "static_cast<double>(RawThrottle)",
            "BrakeProfile.Throttle, LongitudinalSpeed, LateralSpeed, bArcade",
            "const float EffectiveThrottle =",
            "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *",
        ):
            self.assertIn(text, tick)
        self.assertLess(tick.index("SelectHandbrakeDriveProfile("),
                        tick.index("latebrake::StabilizeCornerThrottle("))
        self.assertLess(tick.index("if (!FMath::IsFinite(Speed)"),
                        tick.index("latebrake::StabilizeCornerThrottle("))
        self.assertLess(tick.index("latebrake::StabilizeCornerThrottle("),
                        tick.index("Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *"))
        self.assertEqual(tick.count("static_cast<float>(Surface.MotorMultiplier)"), 3)
        self.assertNotIn("const float EffectiveThrottle = SmoothedThrottle;", tick)

    def test_corner_assist_preserves_brake_reverse_and_validates_motion(self):
        self.assertIn("std::isfinite(demand)", self.traction)
        self.assertIn("std::isfinite(longitudinalCmPerSecond)", self.traction)
        self.assertIn("std::isfinite(lateralCmPerSecond)", self.traction)
        self.assertIn("if (bounded <= 0.0)", self.traction)
        self.assertIn("return bounded;", self.traction)
        self.assertIn("return bounded * CornerThrottleFactor(", self.traction)
        self.assertIn("CornerThrottleFactor(1800.0, 950.0", self.traction_tests)
        self.assertIn("StabilizeCornerThrottle(-0.75", self.traction_tests)

    def test_hud_clears_stale_wrong_way_across_discontinuity(self):
        self.assertIn('#include "Racing/WrongWayIndicator.h"', self.hud_h)
        self.assertIn("latebrake::WrongWayIndicator WrongWayHint;", self.hud_h)
        self.assertIn("TWeakObjectPtr<APawn> LastWayfindingPawn;", self.hud_h)
        for required in (
            "Mode->GetCheckpointSessionRevision()",
            "Car->GetTeleportRevision()",
            "LastWayfindingPawn.Get() != Car",
            "WrongWayHint.Reset();",
            "bWayfindingEligible",
            "!Mode->IsFocusLossGuardActive()",
            "!Mode->IsManualRacePauseActive()",
            "Mode->HasLocalPlayerDriver()",
            "WrongWayHint.Update(",
            "WRONG WAY - TURN AROUND",
        ):
            self.assertIn(required, self.hud)
        self.assertLess(self.hud.index("LastWayfindingPawn.Get() != Car"),
                        self.hud.index("WrongWayHint.Update("))
        self.assertIn("AssessCircuitTravel(", self.wrong)
        self.assertIn("CircuitTravelAssessment::AgainstRoute", self.wrong)

    def test_no_gameplay_scoring_or_physics_side_effect_from_warning(self):
        self.assertNotIn("RegisterCheckpoint(", self.hud)
        self.assertNotIn("Race.PassCheckpoint(", self.hud)
        self.assertNotIn("Chassis->AddForce(", self.hud)
        self.assertNotIn("TryChargePlayerRecovery", self.wrong)
        self.assertIn("ExpectedCircuitHeading(", self.wrong)
        self.assertIn("CircuitTravelHeading::Unavailable", self.wrong)

    def test_standalone_suites_register_both_policies(self):
        for required in (
            "CornerTractionAssistTests.cpp", "WrongWayIndicatorTests.cpp",
            "NAME CornerTractionAssist", "NAME WrongWayIndicator",
        ):
            self.assertIn(required, self.cmake)
        self.assertIn("ExpectedCircuitHeading(", self.wrong_tests)
        self.assertIn("std::numeric_limits<double>::quiet_NaN()", self.wrong_tests)
        self.assertIn("std::numeric_limits<double>::quiet_NaN()", self.traction_tests)

    def test_mutation_removing_active_physics_use_detectable(self):
        mutant = self.pawn.replace(
            "latebrake::StabilizeCornerThrottle(",
            "RawThrottle /* disabled traction */",
        )
        self.assertNotIn("latebrake::StabilizeCornerThrottle(", mutant)
        other = self.hud.replace(
            "Car->GetTeleportRevision()",
            "0 /* stale HUD state */",
        )
        self.assertNotIn("Car->GetTeleportRevision()", other)


if __name__ == "__main__":
    unittest.main()
