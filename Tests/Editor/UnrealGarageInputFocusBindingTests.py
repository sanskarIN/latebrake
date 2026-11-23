"""Prevent garage gamepad bindings from disrupting live race inputs.

This is a STATIC source-binding test. Pure C++ policy tests run separately;
real controller button event ordering needs licensed Unreal Editor QA.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
CPP = ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp"
POLICY = ROOT / "Source/Latebrake/Public/UI/GarageInputFocusPolicy.h"
INPUT = ROOT / "Config/DefaultInput.ini"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"

ROUTES = {
    "CycleGaragePaint": "GarageCyclePaint",
    "CycleGarageStripe": "GarageCycleStripe",
    "CycleGarageWheels": "GarageCycleWheels",
    "ResetGarageLook": "GarageResetAppearance",
    "BrowseGarageNext": "GarageBrowseNext",
    "BrowseGaragePrevious": "GarageBrowsePrevious",
}
COLLISIONS = {
    "GarageCyclePaint": ("Gamepad_RightThumbstick", "Handbrake"),
    "GarageCycleStripe": ("Gamepad_LeftThumbstick", "Nitro"),
    "GarageCycleWheels": ("Gamepad_LeftShoulder", "LookBack"),
    "GarageBrowseNext": ("Gamepad_RightShoulder", "Camera"),
    "GarageBrowsePrevious": ("Gamepad_FaceButton_Right", "Recover"),
}


class UnrealGarageInputFocusBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn = CPP.read_text(encoding="utf-8")
        cls.policy = POLICY.read_text(encoding="utf-8")
        cls.input = INPUT.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_shared_controller_keys_are_real_gameplay_bindings(self):
        for shortcut,(key,race_action) in COLLISIONS.items():
            for action in (shortcut,race_action):
                self.assertIn(
                    f'+ActionMappings=(ActionName="{action}",Key={key})',
                    self.input, f"{action} needs explicit focus gate on {key}")

    def test_all_six_pawn_callbacks_guard_before_clear_and_gamemode(self):
        self.assertIn('#include "UI/GarageInputFocusPolicy.h"', self.pawn)
        for method,action in ROUTES.items():
            with self.subTest(method=method):
                marker = f"void ALatebrakeCarPawn::{method}()"
                self.assertEqual(self.pawn.count(marker), 1)
                function = self.pawn.split(marker,1)[1].split(
                    "\n}\n",1)[0]
                gate = function.index("!latebrake::CanConsumeGarageShortcut(")
                read_open = function.index("Mode->IsMenuOpen()")
                read_page = function.index("Mode->GetMenuPage()")
                refusal = function.index("return;",gate)
                clear = function.index("ClearDriverInputs();")
                dispatch = function.index("Mode->" + action + "();")
                self.assertLess(gate,read_open)
                self.assertLess(read_open,read_page)
                self.assertLess(read_page,refusal)
                self.assertLess(refusal,clear)
                self.assertLess(clear,dispatch)
                self.assertIn("if (!Mode ||",function)
                self.assertNotIn("Mode->" + action + "();",function[:refusal])

    def test_input_policy_is_fail_closed_and_pure(self):
        self.assertIn('#include "UI/MenuNavigation.h"',self.policy)
        self.assertIn("inline constexpr bool CanConsumeGarageShortcut",self.policy)
        self.assertIn("return menuOpen && visiblePage == MenuPage::Garage;",self.policy)
        for effect in ("ClearDriverInputs(", "SaveRecords(", "Garage.Unlock(",
                       "Garage.Select(", "SetThrottle(", "AddForce("):
            self.assertNotIn(effect,self.policy)
        self.assertIn("GarageInputFocusTests.cpp",self.cmake)
        self.assertIn("NAME GarageInputFocus",self.cmake)


if __name__ == "__main__":
    unittest.main()
