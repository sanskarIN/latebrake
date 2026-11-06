"""Synthetic fixtures for an offline handling telemetry inspector.

These are fabricated diagnostic strings and NEVER evidence of gameplay.
"""
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from AnalyzeDriveTelemetry import (  # noqa: E402
    MARKER, MAX_LOG_BYTES, DriveTelemetryError,
    parse_record, summarize_log
)


def line(**changes):
    values = {
        "driver": "player", "mode": "arcade",
        "forward_cmps": "2800.0", "lateral_cmps": "120.0",
        "yaw_radps": "0.120", "throttle": "0.800", "steer": "0.320",
        "contact": "1.000", "rear": "1.000", "front": "1.000",
        "motor_force": "1250000.0", "opposing_force": "0.0",
        "service_force": "0.0", "lateral_force": "-80000.0",
        "yaw_torque": "1600000.0", "draft_pct": "2.500",
    }
    values.update({k: str(v) for k, v in changes.items()})
    return MARKER + (
        "driver={driver} mode={mode} "
        "forward_cmps={forward_cmps} lateral_cmps={lateral_cmps} "
        "yaw_radps={yaw_radps} throttle={throttle} steer={steer} "
        "contact={contact} rear={rear} front={front} "
        "motor_force={motor_force} opposing_force={opposing_force} "
        "service_force={service_force} lateral_force={lateral_force} "
        "yaw_torque={yaw_torque} draft_pct={draft_pct}"
    ).format(**values)


class DriveTelemetryAuditTests(unittest.TestCase):
    def reject(self, **overrides):
        with self.assertRaises(DriveTelemetryError):
            parse_record(line(**overrides))

    def test_real_force_fields_are_signed_and_summarized(self):
        observation = parse_record(line())
        self.assertEqual(observation["forward_cmps"], 2800.0)
        self.assertEqual(observation["motor_force"], 1250000.0)
        summary = summarize_log("\n".join([
            "[time] LogTemp: Display: " + line(),
            line(driver="autonomous", mode="simulation",
                 forward_cmps="-1200.0", throttle="0.500",
                 motor_force="0.0", opposing_force="180000.0",
                 lateral_cmps="-200.0", lateral_force="40000.0",
                 draft_pct="0.0"),
            line(contact="0.000", rear="0.000", front="0.000",
                 forward_cmps="300.0", lateral_cmps="0.0",
                 throttle="0.0", steer="0.0", motor_force="0.0",
                 opposing_force="0.0", service_force="0.0",
                 lateral_force="0.0", yaw_torque="0.0", draft_pct="0.0"),
        ]))
        self.assertEqual(summary["samples"], 3)
        self.assertEqual(summary["drivers"], {"player": 2, "autonomous": 1})
        self.assertEqual(summary["modes"], {"arcade": 2, "simulation": 1})
        self.assertEqual(summary["flagged_samples"]["zero_contact_samples"], 1)
        self.assertEqual(summary["flagged_samples"]["positive_draft_samples"], 1)
        self.assertEqual(summary["flagged_samples"]["opposite_pedal_braking_samples"], 1)
        self.assertEqual(summary["maximums"]["abs_motor_force"], 1250000.0)
        self.assertFalse(summary["gameplay_verified"])
        self.assertFalse(summary["ready_for_release"])

    def test_invalid_telemetry_never_grants_status(self):
        for bad in ("", "LATEBRAKE_DRIVE v=2 motor_force=5", "hi"):
            with self.assertRaises(DriveTelemetryError):
                summarize_log(bad)
        with self.assertRaises(DriveTelemetryError):
            summarize_log(line() + " " + MARKER + line())
        with self.assertRaises(DriveTelemetryError):
            summarize_log("prefix " + line().replace(" draft_pct=2.500", ""))
        with self.assertRaises(DriveTelemetryError):
            summarize_log("x" * 8193 + "\n" + line())
        with self.assertRaises(DriveTelemetryError):
            summarize_log("\0" + line())

    def test_malformed_and_nonfinite_samples_fail_closed(self):
        self.reject(driver="unknown")
        self.reject(mode="nitro")
        self.reject(forward_cmps="nan")
        self.reject(lateral_cmps="inf")
        self.reject(contact="1.200")
        self.reject(contact="-1.0")
        self.reject(rear="0.90", contact="0.20")
        self.reject(front="0.90", contact="0.20")
        self.reject(draft_pct="6.100")
        self.reject(yaw_radps="12.500")
        self.reject(motor_force="100000001.0")
        self.reject(throttle="1.200")
        self.reject(steer="-1.200")
        self.reject(motor_force="150000.0", opposing_force="-100000.0")
        self.reject(motor_force="150000.0", service_force="-100000.0")
        self.reject(opposing_force="-100000.0", service_force="-100000.0",
                    motor_force="0.0")
        self.reject(rear="0.000", motor_force="150000.0")
        self.reject(front="0.000", yaw_torque="150000.0")
        self.reject(contact="0.000", lateral_force="-80000.0")
        self.reject(motor_force="-100000.0", throttle="0.800")
        self.reject(opposing_force="100000.0", motor_force="0.0")
        self.reject(service_force="100000.0", motor_force="0.0")
        self.reject(lateral_force="100000.0")

    def test_finite_stress_and_offline_boundaries(self):
        records = []
        for i in range(5000):
            values = {
                "driver": "player" if i % 2 else "autonomous",
                "mode": "arcade" if i % 3 else "simulation",
                "forward_cmps": (i % 5000) * (-1 if i % 2 else 1),
                "lateral_cmps": 0.0,
                "throttle": 0.0, "steer": 0.0,
                "motor_force": 0.0, "lateral_force": 0.0,
                "yaw_torque": 0.0, "draft_pct": 0.0,
            }
            records.append(line(**values))
        summary = summarize_log("\n".join(records))
        self.assertEqual(summary["samples"], 5000)
        self.assertEqual(MAX_LOG_BYTES, 32 * 1024 * 1024)
        script = (ROOT / "scripts/AnalyzeDriveTelemetry.py").read_text(
            encoding="utf-8")
        for forbidden in ("import unreal", "import requests", "urllib.request",
                          "git push", "gh release", "SaveGame(", "socket.connect("):
            self.assertNotIn(forbidden, script)


if __name__ == "__main__":
    unittest.main()
