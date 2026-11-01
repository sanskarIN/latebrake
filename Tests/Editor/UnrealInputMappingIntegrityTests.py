"""Unreal legacy input config integrity tests (source-only, no UE runtime).

Reject malformed combined +ActionMappings rows before Windows/Android
packaging. Unlike substring checks, this parses every mapping record and
checks that native Pawn input bindings have separate configured mappings.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
INPUT = ROOT / "Config" / "DefaultInput.ini"
PAWN = ROOT / "Source" / "Latebrake" / "Private" / "Vehicles" / "LatebrakeCarPawn.cpp"

ACTION = re.compile(
    r'\+ActionMappings=\(ActionName="([A-Za-z][A-Za-z0-9_]*)",'
    r'Key=([A-Za-z0-9_]+)\)'
)
AXIS = re.compile(
    r'\+AxisMappings=\(AxisName="([A-Za-z][A-Za-z0-9_]*)",'
    r'Scale=(-?[0-9]+(?:\.[0-9]+)?),Key=([A-Za-z0-9_]+)\)'
)


def parse_bindings(text):
    actions, axes = [], []
    for number, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith(("#", ";", "[")):
            continue
        if "ActionMappings=" in line or "AxisMappings=" in line:
            action = ACTION.fullmatch(line)
            axis = AXIS.fullmatch(line)
            if action:
                actions.append(action.groups())
            elif axis:
                axes.append(axis.groups())
            else:
                raise ValueError(f"Malformed Unreal input binding at line {number}: {line}")
    return actions, axes


class UnrealInputMappingIntegrityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ini = INPUT.read_text(encoding="utf-8")
        cls.pawn = PAWN.read_text(encoding="utf-8")
        cls.actions, cls.axes = parse_bindings(cls.ini)

    def test_every_mapping_is_individually_parseable(self):
        self.assertGreaterEqual(len(self.actions), 30)
        self.assertGreaterEqual(len(self.axes), 4)
        self.assertEqual(len(self.actions), len(set(self.actions)),
                         "same action/key appears more than once")
        self.assertEqual(len(self.axes), len(set(self.axes)),
                         "same axis/scale/key appears more than once")
        self.assertFalse(any('),+' in line for line in self.ini.splitlines()))
        self.assertFalse(any(line.startswith("ActionMappings=")
                             for line in self.ini.splitlines()))

    def test_keyboard_and_gamepad_can_confirm_menu_independently(self):
        actions = set(self.actions)
        self.assertIn(("MenuActivate", "Enter"), actions)
        self.assertIn(("MenuActivate", "Gamepad_FaceButton_Top"), actions)
        self.assertIn(("MenuMouseClick", "LeftMouseButton"), actions)
        self.assertEqual(
            sum(action == "MenuMouseClick" for action, _ in self.actions), 1
        )

    def test_actual_player_pawn_uses_only_configured_action_and_axis_names(self):
        bound_actions = set(re.findall(
            r'BindAction\s*\(\s*TEXT\("([^"]+)"\)', self.pawn
        ))
        bound_axes = set(re.findall(
            r'BindAxis\s*\(\s*TEXT\("([^"]+)"\)', self.pawn
        ))
        self.assertIn("MenuMouseClick", bound_actions)
        self.assertIn("MenuActivate", bound_actions)
        self.assertIn("Throttle", bound_axes)
        self.assertIn("Steer", bound_axes)
        available_actions = {name for name, key in self.actions}
        available_axes = {name for name, scale, key in self.axes}
        self.assertFalse(bound_actions - available_actions,
                         f"Unmapped pawn actions: {bound_actions - available_actions}")
        self.assertFalse(bound_axes - available_axes,
                         f"Unmapped pawn axes: {bound_axes - available_axes}")

    def test_reject_concatenated_unreal_array_entries(self):
        original = ('+ActionMappings=(ActionName="MenuActivate",Key=Enter)\n'
                    '+ActionMappings=(ActionName="MenuMouseClick",Key=LeftMouseButton)')
        corrupted = original.replace('\n', ',')
        with self.assertRaisesRegex(ValueError, "Malformed Unreal input binding"):
            parse_bindings(corrupted)

    def test_reject_missing_or_malformed_key_bindings(self):
        invalid = (
            '+ActionMappings=(ActionName="MenuMouseClick")',
            '+ActionMappings=(ActionName="MenuActivate",Key=Enter,Extra=true)',
            'ActionMappings=(ActionName="MenuActivate",Key=Enter)',
            '+AxisMappings=(AxisName="Steer",Scale=bogus,Key=A)',
        )
        for line in invalid:
            with self.subTest(line=line), self.assertRaisesRegex(
                ValueError, "Malformed Unreal input binding"
            ):
                parse_bindings(line)


if __name__ == "__main__":
    unittest.main()
