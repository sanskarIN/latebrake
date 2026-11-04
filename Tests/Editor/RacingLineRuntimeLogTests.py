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


def event(message):
    return "LogLatebrakeStartup: Display: " + message


def sample(*extra):
    return "\n".join([event(READY), event(START)] + [event(x) for x in extra])


class RacingLineRuntimeLogTests(unittest.TestCase):
    def test_ready_and_two_genuine_toggle_marker_shapes(self):
        self.assertEqual(mod.verify(sample()), ("LatebrakeCircuit", []))
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
                   sample().replace("checkpoints=4/4", "checkpoints=3/4"))
        for log in invalid:
            with self.subTest(log=log[:65]):
                with self.assertRaises(ValueError):
                    mod.verify(log)

    def test_failure_markers_and_fake_categories_are_not_accepted(self):
        for log in (sample("LATEBRAKE_RACING_LINE_INCOMPLETE reason=mesh"),
                    sample("LATEBRAKE_STARTUP_BLOCKED reason=geometry"),
                    "LogTemp: Display: " + READY + "\n" + event(START)):
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
