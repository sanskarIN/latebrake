"""Static Unreal pawn wiring tests; NOT UHT, Unreal compilation or device QA.

The standalone C++ policy suite validates the resume allowlist. These tests
verify that Unreal calls the policy and prevents driving while its world
pause fails after app deactivation.
"""

from pathlib import Path
import unittest

PAWN = (
    Path(__file__).resolve().parents[2]
    / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
)


def method(source: str, name: str) -> str:
    marker = "void ALatebrakeCarPawn::" + name + "("
    return source.split(marker, 1)[1].split(
        "\nvoid ALatebrakeCarPawn::", 1
    )[0]


class FocusSuspendedDriverBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = PAWN.read_text(encoding="utf-8")

    def test_app_loss_stops_motion_after_guard_latched(self):
        handler = method(self.source, "HandleAppFocusLost")
        self.assertLess(handler.index("HandleAppInputInterruption();"),
                        handler.index("Mode->PauseForAppInterruption();"))
        self.assertLess(handler.index("Mode->PauseForAppInterruption();"),
                        handler.index("Mode->IsFocusLossGuardActive()"))
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", handler)

    def test_driver_neutralization_resets_filtered_and_physical_motion(self):
        neutralize = method(self.source, "NeutralizeMotionWhenDrivingBlocked")
        self.assertIn("ClearDriverInputs();", neutralize)
        self.assertIn("Chassis->IsSimulatingPhysics()", neutralize)
        self.assertIn("Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);", neutralize)
        self.assertIn("Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);", neutralize)

    def test_tick_returns_before_force_or_steering_when_guard_is_active(self):
        tick = method(self.source, "Tick")
        self.assertIn("Mode->IsFocusLossGuardActive()", tick)
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", tick)
        self.assertLess(tick.index("Mode->IsFocusLossGuardActive()"),
                        tick.index("Chassis->AddForce("))
        self.assertLess(tick.index("NeutralizeMotionWhenDrivingBlocked();"),
                        tick.index("Chassis->AddTorqueInRadians("))
        self.assertIn("!latebrake::CanApplyPlayerDrive({", tick)
        blocked = tick.split("if (!Mode || !latebrake::CanApplyPlayerDrive({", 1)[1]
        blocked = blocked.split("#if PLATFORM_ANDROID", 1)[0]
        self.assertIn("return;", blocked)
        self.assertIn("Mode->IsCircuitReadyForGameplay()", blocked)
        self.assertIn("Mode->IsMenuOpen()", blocked)
        self.assertIn("UGameplayStatics::IsGamePaused(this)", blocked)
        self.assertIn("Mode->GetRace().IsFinished()", blocked)

    def test_touch_focus_guard_precedes_menu_world_pause_and_driving(self):
        touch = method(self.source, "ApplyTouch")
        self.assertIn("Mode && Mode->IsFocusLossGuardActive()", touch)
        self.assertIn("CanResumeFocusSuspendedRaceFromTouch(", touch)
        self.assertIn("bAllowAcquire, Region", touch)
        self.assertLess(touch.index("Mode && Mode->IsFocusLossGuardActive()"),
                        touch.index("Mode && Mode->IsMenuOpen()"))
        self.assertLess(touch.index("Mode && Mode->IsFocusLossGuardActive()"),
                        touch.index("UGameplayStatics::IsGamePaused(this)"))
        focus = touch.split("if (Mode && Mode->IsFocusLossGuardActive())", 1)[1]
        focus = focus.split("if (Mode && Mode->IsMenuOpen())", 1)[0]
        self.assertIn("NeutralizeMotionWhenDrivingBlocked();", focus)
        self.assertIn("TogglePause();", focus)
        self.assertIn("return;", focus)
        self.assertNotIn("RecoverPressed();", focus)
        self.assertNotIn("RestartPressed();", focus)

    def test_a_missing_focus_gate_would_be_caught(self):
        broken = self.source.replace(
            "if (Mode && Mode->IsFocusLossGuardActive())",
            "if (false /* regression fixture */)",
        )
        touch = method(broken, "ApplyTouch")
        # The later CAM-command admission deliberately repeats this
        # condition. This mutation must remove the *early* global guard,
        # even when a safe secondary camera check still references focus.
        self.assertNotIn(
            "if (Mode && Mode->IsFocusLossGuardActive())", touch)
        self.assertIn("if (false /* regression fixture */)", touch)
        self.assertIn(
            "Mode && Mode->IsFocusLossGuardActive()", touch)
        self.assertLess(
            touch.index("if (false /* regression fixture */)"),
            touch.index("if (Mode && Mode->IsMenuOpen())"))


if __name__ == "__main__":
    unittest.main()
