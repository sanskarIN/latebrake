"""Negative and positive fixtures for the UNVERIFIED Unreal suspension log inspector.

All records here are fabricated solely to verify parser policy. They are
NEVER evidence of real UE 5.8 gameplay, installed runtime or source integrity.
"""
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from AnalyzeSuspensionTelemetry import (  # noqa: E402
    MARKER, MAX_LOG_BYTES, SuspensionAuditError,
    parse_record, summarize_log,
)


def line(**changes):
    fields = {
        "driver": "player",
        "phase": "go",
        "contacts": "4",
        "weight_ratio": "1.000",
        "traction": "1.000",
        "drive_rear": "1.000",
        "steer_front": "1.000",
        "front_load": "1.000",
        "rear_load": "1.000",
        "arb_front_pct": "0.00",
        "arb_rear_pct": "0.00",
        "body_z_cm": "59.0",
        "vz_cmps": "0.0",
        "pitch_deg": "0.0",
        "roll_deg": "0.0",
        "fl_cm": "59.0",
        "fr_cm": "59.0",
        "rl_cm": "59.0",
        "rr_cm": "59.0",
    }
    fields.update({key: str(value) for key, value in changes.items()})
    return (
        MARKER +
        "driver={driver} phase={phase} contacts={contacts} "
        "weight_ratio={weight_ratio} traction={traction} "
        "drive_rear={drive_rear} steer_front={steer_front} "
        "front_load={front_load} rear_load={rear_load} "
        "arb_front_pct={arb_front_pct} arb_rear_pct={arb_rear_pct} "
        "body_z_cm={body_z_cm} vz_cmps={vz_cmps} "
        "pitch_deg={pitch_deg} roll_deg={roll_deg} "
        "fl_cm={fl_cm} fr_cm={fr_cm} rl_cm={rl_cm} rr_cm={rr_cm}"
    ).format(**fields)


class SuspensionTelemetryAuditTests(unittest.TestCase):
    def reject(self, **changes):
        with self.assertRaises(SuspensionAuditError):
            parse_record(line(**changes))

    def test_world_spring_load_and_default_four_tire_line(self):
        point = parse_record(line())
        self.assertEqual(point["contacts"], 4)
        self.assertEqual(point["driver"], "player")
        self.assertEqual(point["rear_load"], 1.0)
        self.assertEqual(point["front_load"], 1.0)
        self.assertEqual(point["drive_rear"], 1.0)
        self.assertEqual(point["steer_front"], 1.0)

    def test_two_real_axles_and_grid_state_can_be_summarized(self):
        records = [
            "LogTemp: Display: " + line(phase="grid", vz_cmps="28.0"),
            "[2026.11.03-12.34.56:000][100]LogTemp: Display: " + line(),
            line(driver="autonomous", contacts="2", weight_ratio="0.500",
                 traction="0.450", drive_rear="0.450", steer_front="0.000",
                 front_load="0.000", rear_load="1.000",
                 fl_cm="-1.0", fr_cm="-1.0"),
            line(contacts="2", weight_ratio="0.500",
                 traction="0.450", drive_rear="0.000", steer_front="0.450",
                 front_load="1.000", rear_load="0.000",
                 rl_cm="-1.0", rr_cm="-1.0"),
            line(contacts="0", weight_ratio="0.000", traction="0.000",
                 drive_rear="0.000", steer_front="0.000",
                 front_load="0.000", rear_load="0.000",
                 fl_cm="-1.0", fr_cm="-1.0",
                 rl_cm="-1.0", rr_cm="-1.0"),
        ]
        summary = summarize_log("\n".join(records))
        self.assertEqual(summary["status"],
                         "UNVERIFIED_OFFLINE_SUSPENSION_DIAGNOSTIC")
        self.assertIs(summary["gameplay_verified"], False)
        self.assertIs(summary["ready_for_release"], False)
        self.assertEqual(summary["samples"], 5)
        self.assertEqual(summary["drivers"]["player"], 4)
        self.assertEqual(summary["drivers"]["autonomous"], 1)
        self.assertEqual(summary["phases"]["grid"], 1)
        self.assertEqual(summary["contact_counts"]["2"], 2)
        self.assertEqual(summary["contact_counts"]["0"], 1)
        self.assertEqual(summary["flagged_samples"]["front_only_support"], 1)
        self.assertEqual(summary["flagged_samples"]["rear_only_support"], 1)
        self.assertEqual(summary["flagged_samples"]["airborne_during_go"], 1)

    def test_realistic_high_roll_flags_but_is_not_automatic_rejection(self):
        summary = summarize_log(line(
            phase="grid", roll_deg="24.0", pitch_deg="-11.0",
            vz_cmps="-420.0", arb_front_pct="-7.99", arb_rear_pct="8.00"
        ))
        self.assertEqual(summary["flagged_samples"]["grid_settle_concerns"], 1)
        self.assertEqual(summary["flagged_samples"]["large_body_pitch_or_roll"], 1)
        self.assertEqual(
            summary["flagged_samples"]["hard_vertical_motion_with_road_contact"], 1
        )
        self.assertEqual(summary["maximums"]["abs_roll_deg"], 24.0)

    def test_missing_or_tampered_log_cannot_claim_qa(self):
        for bad in (
            "", "LogTemp: Display: Ready", "LATEBRAKE_SUSPENSION v=2",
            "LATEBRAKE_SUSPENSION v=1 without fields",
        ):
            with self.subTest(bad=bad), self.assertRaises(SuspensionAuditError):
                summarize_log(bad)
        with self.assertRaises(SuspensionAuditError):
            summarize_log(line() + " " + MARKER + line())
        with self.assertRaises(SuspensionAuditError):
            summarize_log("prefix " + line().replace(" rr_cm=59.0", ""))
        with self.assertRaises(SuspensionAuditError):
            summarize_log("\0" + line())
        with self.assertRaises(SuspensionAuditError):
            summarize_log("x" * 8193 + "\n" + line())

    def test_nonfinite_percent_boost_violations_fail_closed(self):
        self.reject(driver="attacker")
        self.reject(phase="offline")
        self.reject(contacts="5")
        self.reject(weight_ratio="nan")
        self.reject(body_z_cm="inf")
        self.reject(traction="1.250")
        self.reject(traction="-0.010")
        self.reject(drive_rear="0.900", traction="0.100")
        self.reject(steer_front="0.900", traction="0.100")
        self.reject(front_load="-0.010")
        self.reject(rear_load="1.200")
        self.reject(arb_front_pct="8.500")
        self.reject(arb_rear_pct="-9.000")
        self.reject(vz_cmps="15001.0")
        self.reject(fl_cm="-2.0")
        self.reject(rr_cm="300.0")
        self.reject(drive_rear="0.001", rear_load="0.000")
        self.reject(steer_front="0.100", front_load="0.000")
        self.reject(contacts="1", traction="0.400")
        self.reject(contacts="0", weight_ratio="0.500")
        self.reject(contacts="1", drive_rear="0.200")
        self.reject(contacts="1", steer_front="0.200")

    def test_log_analyzer_stays_offline_and_never_publishes(self):
        script = (ROOT / "scripts/AnalyzeSuspensionTelemetry.py").read_text(
            encoding="utf-8"
        )
        self.assertEqual(MAX_LOG_BYTES, 32 * 1024 * 1024)
        for forbidden in (
            "import unreal", "import requests", "urllib.request",
            "git push", "gh release", "SaveGame(", "socket.connect(",
        ):
            self.assertNotIn(forbidden, script)
        for needed in (
            'status": "UNVERIFIED_OFFLINE_SUSPENSION_DIAGNOSTIC"',
            '"gameplay_verified": False', '"ready_for_release": False',
            "abs_pitch_deg", "front_only_support", "rear_only_support",
            "LATEBRAKE_SUSPENSION v=1 ", "parse_record(",
            "MAX_LOG_BYTES", "args.log.is_symlink()",
        ):
            self.assertIn(needed, script)


if __name__ == "__main__":
    unittest.main()
