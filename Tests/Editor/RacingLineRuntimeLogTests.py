"""Synthetic parser regression tests; never evidence of an Unreal build."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "scripts/Verify-RacingLineRuntimeLog.py"
spec = importlib.util.spec_from_file_location("guide_log_validator", SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

READY = "LATEBRAKE_RACING_LINE_READY marks=88/88 straight=48 corner=40 collision=off"
START = "LATEBRAKE_STARTUP_READY map=LatebrakeCircuit checkpoints=4/4"
MATERIAL = "LATEBRAKE_RACING_LINE_MATERIAL authored=yes"
FALLBACK = "LATEBRAKE_RACING_LINE_MATERIAL authored=no"


def event(message):
    return "LogLatebrakeStartup: Display: " + message


def sample(*extra):
    return "\n".join([event(READY), event(START)] + [event(x) for x in extra])


class RacingLineRuntimeLogTests(unittest.TestCase):
    def test_ready_and_two_genuine_toggle_marker_shapes(self):
        self.assertEqual(mod.verify(sample()), ("LatebrakeCircuit", []))
        # Real Unreal logs often prepend a timestamp and thread ID.
        timestamped = "\n".join(
            "[2026.11.04-10.00.00:000][  0]" + line
            for line in sample().splitlines()
        )
        self.assertEqual(
            mod.verify(timestamped)[0], "LatebrakeCircuit"
        )
        log = sample("LATEBRAKE_RACING_LINE_VISIBILITY visible=off",
                     "LATEBRAKE_RACING_LINE_VISIBILITY visible=on")
        self.assertEqual(mod.verify(log, require_toggle=True)[1], ["off", "on"])

    def test_missing_wrong_duplicated_and_reordered_guide_fail(self):
        invalid = ("", event(START), sample() + "\n" + event(READY),
                   event(START) + "\n" + event(READY),
                   sample().replace("marks=88/88", "marks=87/88"),
                   sample().replace("corner=40", "corner=39"),
                   sample().replace("collision=off", "collision=on"),
                   sample().replace("LatebrakeCircuit", "WrongMap"),
                   sample().replace("LatebrakeCircuit", "BogusLatebrakeCircuit"),
                   sample().replace("checkpoints=4/4", "checkpoints=3/4"))
        for log in invalid:
            with self.subTest(log=log[:65]):
                with self.assertRaises(ValueError):
                    mod.verify(log)

    def test_failure_markers_and_fake_categories_are_not_accepted(self):
        for log in (sample("LATEBRAKE_RACING_LINE_INCOMPLETE reason=mesh"),
                    event(MATERIAL) + "\n" +
                    sample("LATEBRAKE_RACING_LINE_INCOMPLETE reason=dynamic_material section=1"),
                    event(MATERIAL) + "\n" +
                    sample("LATEBRAKE_RACING_LINE_INCOMPLETE reason=material_assignment section=0"),
                    sample("LATEBRAKE_STARTUP_BLOCKED reason=geometry"),
                    "LogTemp: Display: " + READY + "\n" + event(START),
                    "LogTemp: Display: quoted " + event(READY) +
                        "\n" + event(START)):
            with self.subTest(log=log[-65:]):
                with self.assertRaises(ValueError):
                    mod.verify(log)

    def test_toggle_cannot_be_invented_or_malformed(self):
        for log in (sample(), sample("LATEBRAKE_RACING_LINE_VISIBILITY visible=off"),
                    sample("LATEBRAKE_RACING_LINE_VISIBILITY visible=on"),
                    sample("LATEBRAKE_RACING_LINE_VISIBILITY visible=off",
                           "LATEBRAKE_RACING_LINE_VISIBILITY visible=off"),
                    sample("LATEBRAKE_RACING_LINE_VISIBILITY visible=unknown")):
            with self.subTest(log=log[-65:]):
                with self.assertRaises(ValueError):
                    mod.verify(log, require_toggle=True)

    def test_strict_authored_material_rejects_missing_fallback_and_late_status(self):
        accepted = event(MATERIAL) + "\n" + sample()
        self.assertEqual(
            mod.verify(accepted, require_authored_material=True)[0],
            "LatebrakeCircuit",
        )
        for log in (
            sample(),
            event(FALLBACK) + "\n" + sample(),
            sample() + "\n" + event(MATERIAL),
            event(MATERIAL) + "\n" + event(MATERIAL) + "\n" + sample(),
            event("LATEBRAKE_RACING_LINE_MATERIAL authored=unknown")
            + "\n" + sample(),
            "LogTemp: Display: " + MATERIAL + "\n" + sample(),
        ):
            with self.subTest(log=log[:60]):
                with self.assertRaises(ValueError):
                    mod.verify(log, require_authored_material=True)

    def test_strict_authored_material_command_line(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp) / "synthetic.log"
            log.write_text(
                event(MATERIAL) + "\n" + sample(
                    "LATEBRAKE_RACING_LINE_VISIBILITY visible=off",
                    "LATEBRAKE_RACING_LINE_VISIBILITY visible=on",
                ), encoding="utf-8",
            )
            command = [
                sys.executable, str(SCRIPT), str(log),
                "--require-toggle", "--require-authored-material",
            ]
            valid = subprocess.run(command, text=True, capture_output=True)
            self.assertEqual(valid.returncode, 0, valid.stderr)
            log.write_text(
                event(FALLBACK) + "\n" + sample(
                    "LATEBRAKE_RACING_LINE_VISIBILITY visible=off",
                    "LATEBRAKE_RACING_LINE_VISIBILITY visible=on",
                ), encoding="utf-8",
            )
            rejected = subprocess.run(command, text=True, capture_output=True)
            self.assertNotEqual(rejected.returncode, 0)

    def test_real_command_line_acceptance_and_rejection(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp) / "synthetic.log"
            valid = sample("LATEBRAKE_RACING_LINE_VISIBILITY visible=off",
                           "LATEBRAKE_RACING_LINE_VISIBILITY visible=on")
            log.write_text(valid, encoding="utf-8")
            command = [sys.executable, str(SCRIPT), str(log), "--require-toggle"]
            result = subprocess.run(command, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("SOURCE-LOG ONLY", result.stdout)
            log.write_text(sample(), encoding="utf-8")
            result = subprocess.run(command, text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
