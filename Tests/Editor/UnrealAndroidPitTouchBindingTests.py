"""Source-only Android PIT HOLD binding, visibility and cancellation checks.

Unreal Engine is not loaded; installed Android multi-touch/Chaos QA remains
required before describing the prototype as playable on a device.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealAndroidPitTouchBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.car = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.layout = read("Source/Latebrake/Public/UI/TouchControlLayout.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_exact_same_visible_and_hit_tested_pit_area(self):
        self.assertIn("TouchRect PitService;", self.layout)
        self.assertIn("return TouchControl::PitService;", self.layout)
        self.assertIn("DrawPad(Pads.PitService, *PitPadLabel, Car->IsTouchPitHeld());",
                      self.hud)
        self.assertIn("MobilePit.ProgressFraction() * 100.0", self.hud)
        self.assertIn("MobilePit.IsInBay()", self.hud)
        self.assertIn('TEXT("PIT HOLD")', self.hud)
        self.assertIn("IsTouchPitHeld() const noexcept", self.header)
        self.assertIn("TouchPit.IsHeld()", self.header)

    def test_pit_hold_is_authorized_by_real_chassis_service(self):
        self.assertIn('UI/TouchPitHold.h', self.header)
        self.assertIn("latebrake::TouchPitHold TouchPit;", self.header)
        observe = self.car.split("if (PitService.Observe({", 1)[1].split("}))", 1)[0]
        self.assertIn("(bPitServiceHeld || TouchPit.IsHeld())", observe)
        self.assertIn("Mode->HasLocalPlayerDriver()", observe)
        self.assertIn("bPitNeedsService", observe)
        touch = self.car.split("void ALatebrakeCarPawn::ApplyTouch(", 1)[1].split(
            "void ALatebrakeCarPawn::TouchReleased(", 1)[0]
        self.assertIn("case latebrake::TouchControl::PitService:", touch)
        self.assertIn("TouchPit.Begin(FingerId, Region, true);", touch)
        self.assertIn("Mode->IsCircuitReadyForGameplay()", touch)
        self.assertIn("Mode->IsRaceStartGreen()", touch)
        self.assertIn("!Mode->GetRace().IsFinished()", touch)
        self.assertIn("IsRacePhysicsReady()", touch)

    def test_interruptions_cancel_without_stealing_pedal_owners(self):
        self.assertIn("PitService.Reset(); bPitServiceHeld = false; TouchPit.Clear();", self.header)
        self.assertIn("TouchPit.FingerOwner() == FingerId", self.car)
        self.assertIn("TouchPit.Move(FingerId, Region,", self.car)
        self.assertIn("TouchPit.Release(FingerId);", self.car)
        self.assertIn("if (bViewportInvalidated)", self.car)
        clear = self.car.split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1].split(
            "void ALatebrakeCarPawn::", 1)[0]
        self.assertIn("TouchPit.Clear();", clear)
        self.assertIn("PitService.Suspend();", clear)
        for method in ("TouchPressed", "ApplyTouch", "TouchReleased"):
            self.assertIn(f"void ALatebrakeCarPawn::{method}(", self.car)
        self.assertIn("NAME TouchPitHold", self.cmake)
        self.assertIn("TouchPitHoldTests.cpp", self.cmake)


if __name__ == "__main__":
    unittest.main()
