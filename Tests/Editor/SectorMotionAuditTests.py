"""Synthetic fixtures for the sector audit. No Unreal runtime is exercised."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"
sys.path.insert(0, str(SCRIPTS))
try:
    from AnalyzeSectorMotionAudit import SectorAuditError, analyze_sector_audit
finally:
    sys.path.remove(str(SCRIPTS))


def begin(attempt=1, laps=1):
    return (f"Log: LATEBRAKE_RACE_AUDIT v=1 action=begin attempt={attempt} "
            f"gates=4 laps={laps} track=1 grid=1")


def gate(ordinal, elapsed, attempt=1, laps=1):
    lap, next_gate = divmod(ordinal, 4)
    return (f"Log: LATEBRAKE_RACE_AUDIT v=1 action=gate attempt={attempt} "
            f"gate={(ordinal - 1) % 4} lap={lap} next={next_gate} "
            f"elapsed={elapsed:.6f} active={lap} finished={int(lap == laps)}")


def observation(ordinal, elapsed, attempt=1, penalty=0, fee=0,
                quality=4, available=True, coverage=.9):
    base = (f"LogTemp: LATEBRAKE_SECTOR_AUDIT v=1 attempt={attempt} "
            f"ordinal={ordinal} gate={(ordinal - 1) % 4} "
            f"elapsed={elapsed:.9f} penalty={penalty:.9f} available={int(available)}")
    if not available:
        return base
    observed = coverage
    fields = (
        ("active", 1), ("fee", fee), ("official", 1+fee),
        ("observed", observed), ("coverage", coverage),
    )
    detail = " ".join(f"{k}={v:.9f}" for k, v in fields)
    motion = (
        ("distance_cm", 100*observed), ("speed_integral", 100*observed),
        ("peak", 110), ("lateral_integral", 10*observed),
        ("low", 0), ("slip", 0), ("reverse", 0), ("partial", 0), ("off", 0),
    )
    return (f"{base} quality={quality} {detail} samples=21 intervals=20 "
            f"breaks={int(quality == 2)} rejected=0 recovery=0 jump=0 "
            + " ".join(f"{k}={v:.9f}" for k, v in motion))


def run(attempt=1, laps=1):
    lines = [begin(attempt, laps)]
    for n in range(1, laps*4+1):
        lines += [gate(n, n, attempt, laps), observation(n, n, attempt)]
    return lines


def analyze(lines, finish=False):
    return analyze_sector_audit("\n".join(lines), finish)


class SectorMotionAuditTests(unittest.TestCase):
    def test_join_all_accepted_gates_and_exclude_standing_reference(self):
        report = analyze(run(laps=2), True)
        self.assertEqual(report["matched_sector_records"], 8)
        attempt = report["attempts"][0]
        self.assertTrue(attempt["finished"])
        self.assertEqual(attempt["quality_counts"]["qualified"], 8)
        self.assertEqual(attempt["rolling_gate_summary"]["0"]["best_ordinal"], 5)
        self.assertEqual(attempt["rolling_gate_summary"]["1"]["best_ordinal"], 2)
        self.assertEqual(float(attempt["sectors"][0]["mean_kmh"]), 3.6)
        self.assertFalse(report["installed_gameplay_verified"])
        self.assertFalse(report["unreal_build_verified"])
        self.assertFalse(report["ready_for_release"])

    def test_unavailable_records_do_not_invent_motion(self):
        lines = run()
        lines[2] = observation(1, 1, available=False)
        a = analyze(lines, True)["attempts"][0]
        self.assertEqual(a["quality_counts"]["unavailable"], 1)
        self.assertNotIn("mean_kmh", a["sectors"][0])
        self.assertNotIn("0", a["rolling_gate_summary"])

    def test_active_time_and_fee_separation(self):
        lines = run()
        for n in range(2, 5):
            lines[2*n-1] = gate(n, n+5)
            lines[2*n] = observation(n, n+5, penalty=5,
                                     fee=5 if n == 2 else 0,
                                     quality=3 if n == 2 else 4)
        a = analyze(lines, True)["attempts"][0]
        self.assertEqual(a["quality_counts"]["penalized"], 1)
        self.assertEqual(a["sectors"][1]["official_seconds"], "6.000000000")
        self.assertNotIn("1", a["rolling_gate_summary"])

    def test_interruption_and_sparse_observations(self):
        lines = run()
        lines[4] = observation(2, 2, quality=2)
        lines[6] = observation(3, 3, quality=1, coverage=.2)
        a = analyze(lines, True)["attempts"][0]
        self.assertEqual(a["quality_counts"]["interrupted"], 1)
        self.assertEqual(a["quality_counts"]["insufficient_coverage"], 1)
        self.assertEqual(set(a["rolling_gate_summary"]), {"3"})

    def test_restart_and_unfinished_attempt_do_not_share_clock(self):
        lines = run()[:5] + run(attempt=2)
        result = analyze(lines, True)
        self.assertEqual(result["attempt_count"], 2)
        self.assertFalse(result["attempts"][0]["finished"])
        self.assertTrue(result["attempts"][1]["finished"])
        self.assertEqual(result["attempts"][1]["sectors"][0]["elapsed_seconds"], "1.000000000")

    def test_orphan_missing_duplicate_and_late_records(self):
        valid = run()
        for lines in (
            valid[:2] + valid[3:],
            valid[:-1],
            valid[:3] + [valid[2]] + valid[3:],
            [valid[2]] + valid,
            valid[:2] + [valid[4]] + valid[3:],
        ):
            with self.subTest(lines=lines), self.assertRaises(SectorAuditError):
                analyze(lines)

    def test_reject_mutated_metrics(self):
        substitutions = (
            ("attempt=1", "attempt=2"),
            ("ordinal=1", "ordinal=2"),
            ("gate=0", "gate=2"),
            ("elapsed=1.000000000", "elapsed=2.000000000"),
            ("penalty=0.000000000", "penalty=0.500000000"),
            ("active=1.000000000", "active=2.000000000"),
            ("official=1.000000000", "official=2.000000000"),
            ("coverage=0.900000000", "coverage=0.100000000"),
            ("samples=21", "samples=1"),
            ("intervals=20", "intervals=0"),
            ("breaks=0", "breaks=1"),
            ("recovery=0", "recovery=1"),
            ("jump=0", "jump=1"),
            ("distance_cm=90.000000000", "distance_cm=999999999999.000000000"),
            ("peak=110.000000000", "peak=1.000000000"),
            ("lateral_integral=9.000000000", "lateral_integral=999.000000000"),
            ("low=0.000000000", "low=1.000000000"),
            ("slip=0.000000000", "slip=1.000000000"),
            ("reverse=0.000000000", "reverse=1.000000000"),
            ("off=0.000000000", "off=0.100000000"),
            ("quality=4", "quality=3"),
            ("quality=4", "quality=1"),
            ("peak=110.000000000", "peak=nan"),
            ("samples=21", "samples=18446744073709551616"),
            ("available=1", "available=0"),
        )
        for old, new in substitutions:
            with self.subTest(old=old, new=new), self.assertRaises(SectorAuditError):
                lines = run()
                self.assertIn(old, lines[2])
                lines[2] = lines[2].replace(old, new, 1)
                analyze(lines)

    def test_no_finish_no_available_and_legacy_logs(self):
        with self.assertRaises(SectorAuditError):
            analyze(run()[:3], True)
        all_missing = run()
        for n in range(1, 5):
            all_missing[n*2] = observation(n, n, available=False)
        with self.assertRaises(SectorAuditError):
            analyze(all_missing, True)
        self.assertEqual(analyze(all_missing)["matched_sector_records"], 4)
        with self.assertRaises(SectorAuditError):
            analyze([s for s in run() if "SECTOR_AUDIT" not in s])

    def test_invalid_authoritative_gate_cannot_be_legitimized_by_motion(self):
        lines = run()
        lines[1] = lines[1].replace("gate=0", "gate=3")
        lines[2] = lines[2].replace("gate=0", "gate=3")
        with self.assertRaisesRegex(SectorAuditError, "base race audit rejected"):
            analyze(lines)

    def test_cli_hashes_exact_bytes_and_refuses_invalid(self):
        with tempfile.TemporaryDirectory() as directory:
            p = Path(directory) / "game.log"
            p.write_text("\n".join(run()), encoding="utf-8")
            command = [sys.executable, str(SCRIPTS/"AnalyzeSectorMotionAudit.py"), str(p),
                       "--require-finish"]
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            payload = json.loads(result.stdout)
            self.assertEqual(len(payload["log_sha256"]), 64)
            self.assertEqual(payload["log_bytes"], p.stat().st_size)
            p.write_text("\n".join(run()[:-1]), encoding="utf-8")
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(result.stdout, "")

    def test_native_adapter_is_nonshipping_and_after_real_receipt(self):
        game = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text()
        adapter = (ROOT / "Source/Latebrake/Private/Game/LatebrakeSectorMotionRuntime.cpp").read_text()
        checkpoint = game.split("void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1)[1]
        self.assertLess(checkpoint.index("LATEBRAKE_RACE_AUDIT v=1 action=gate"),
                        checkpoint.index("LogPlayerSectorMotionAudit(CheckpointIndex"))
        self.assertIn("#if !UE_BUILD_SHIPPING\nvoid ALatebrakeRaceGameMode::LogPlayerSectorMotionAudit", adapter)
        self.assertIn("Row.Ordinal==AcceptedOrdinal && Row.Gate==Gate", adapter)
        self.assertIn("available=0", adapter)
        self.assertIn("M.ObservedSeconds,Row.Coverage", adapter)


if __name__ == "__main__":
    unittest.main()
