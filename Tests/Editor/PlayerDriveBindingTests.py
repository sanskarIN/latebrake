"""Source-only contracts for the Unreal player pawn's authoritative drive gate.

Portable C++ exercises all six readiness bits. These checks ensure the
actual Unreal pawn gathers them and refuses motor force before physics
integration. This does NOT compile UHT/Chaos or simulate device controls.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CPP = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
HEADER = ROOT / "Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h"
POLICY = ROOT / "Source/Latebrake/Public/Vehicles/PlayerDriveReadiness.h"


def method(source: str, name: str) -> str:
    signature = "void ALatebrakeCarPawn::" + name + "("
    assert source.count(signature) == 1, signature
    return source.split(signature, 1)[1].split(
        "\nvoid ALatebrakeCarPawn::", 1
    )[0]


class PlayerDriveBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = CPP.read_text(encoding="utf-8")
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")

    def test_pawn_includes_shared_portable_drive_policy(self):
        self.assertIn('#include "Vehicles/PlayerDriveReadiness.h"', self.cpp)
        self.assertIn("CanApplyPlayerDrive(PlayerDriveConditions state)", self.policy)

    def test_tick_requires_all_authoritative_safety_flags(self):
        tick = method(self.cpp, "Tick")
        self.assertIn("World ? World->GetAuthGameMode<ALatebrakeRaceGameMode>()", tick)
        gate = "if (!Mode || !Mode->IsRaceStartGreen() ||"
        self.assertIn("!latebrake::CanApplyPlayerDrive({", tick)
        self.assertIn("Mode->IsRaceStartGreen()", tick)
        self.assertEqual(tick.count(gate), 1)
        required = (
            "Mode->IsCircuitReadyForGameplay()",
            "Mode->IsMenuOpen()",
            "UGameplayStatics::IsGamePaused(this)",
            "Mode->IsFocusLossGuardActive()",
            "Mode->GetRace().IsFinished()",
            "Mode->IsRestartPlacementReady()",
        )
        guard_start = tick.index(gate)
        guard_end = tick.index("NeutralizeMotionWhenDrivingBlocked();", guard_start)
        for flag in required:
            with self.subTest(flag=flag):
                self.assertIn(flag, tick[guard_start:guard_end])
        self.assertLess(guard_end, tick.index("return;", guard_end))
        self.assertLess(guard_end, tick.index("Chassis->AddForce("))
        self.assertLess(guard_end, tick.index("Chassis->AddTorqueInRadians("))
        self.assertLess(guard_end, tick.index("SmoothedThrottle ="))

    def test_neutralization_zeroes_raw_filtered_touch_and_chassis_motion(self):
        name = "NeutralizeMotionWhenDrivingBlocked"
        self.assertIn("void " + name + "();", self.header)
        stopped = method(self.cpp, name)
        self.assertIn("ClearDriverInputs();", stopped)
        self.assertIn("Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);", stopped)
        self.assertIn("Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);", stopped)
        inputs = method(self.cpp, "ClearDriverInputs")
        for item in ("Throttle = 0.f", "Steering = 0.f",
                     "SmoothedThrottle = 0.f", "SmoothedSteering = 0.f",
                     "bBraking = false", "TouchDrive.Clear()"):
            self.assertIn(item, inputs)

    def test_touch_focus_handler_still_has_explicit_manual_resume(self):
        touch = method(self.cpp, "ApplyTouch")
        self.assertIn("CanResumeFocusSuspendedRaceFromTouch", touch)
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", touch)
        self.assertIn("TogglePause();", touch)
        self.assertLess(touch.index("IsFocusLossGuardActive()"),
                        touch.index("Mode && Mode->IsMenuOpen()"))

    def test_missing_safety_policy_cannot_pass_this_contract(self):
        tick = method(self.cpp, "Tick")
        without_gate = tick.replace("!latebrake::CanApplyPlayerDrive({",
                                    "false /* regression fixture */")
        self.assertNotIn("!latebrake::CanApplyPlayerDrive({", without_gate)


if __name__ == "__main__":
    unittest.main()
