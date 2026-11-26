"""Synthetic parser fixtures; none constitutes an actual Unreal playtest."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/AnalyzeFleetAudit.py"
SPEC = importlib.util.spec_from_file_location("fleet_audit", SCRIPT)
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


def event(action, fields, attempt=1):
    return f"LogTemp: Display: LATEBRAKE_FLEET_AUDIT v=1 action={action} attempt={attempt} {fields}"


def begin(attempt=1):
    return event("begin", "gates=4 laps=1", attempt)


def roster(attempt=1):
    return event("roster", "frame=2 id0=100 id1=101 id2=102 id3=103 id4=104 id5=105", attempt)


def go(attempt=1):
    return event("go", "frame=40 stable=0.300000000 startup=3.250000000", attempt)


def gate(slot, ordinal, seconds, attempt=1):
    return event("gate", f"slot={slot} id={100 + slot} ordinal={ordinal} clock={seconds:.9f}", attempt)


def complete(attempt=1):
    lines = [begin(attempt), roster(attempt), go(attempt)]
    lines += [gate(slot, 1, 1.0, attempt) for slot in range(6)]
    lines += [
        event("recovery", "slot=0 id=100 ordinal=1 verified=1", attempt),
        event("recovery", "slot=3 id=103 ordinal=1 verified=0", attempt),
        gate(2, 2, 2.0, attempt),
        gate(2, 3, 3.0, attempt),
        gate(2, 4, 4.0, attempt),
        gate(0, 2, 5.0, attempt),
        gate(0, 3, 6.0, attempt),
        gate(0, 4, 7.0, attempt),
        event("finish", "clock=7.000000000 fees=5.000000000 timing=1 review=1 completed_routes=1 saved=0", attempt),
    ]
    return lines


class FleetAuditTests(unittest.TestCase):
    def analyze(self, lines, **kwargs):
        return AUDIT.analyze_fleet_audit("\n".join(lines), **kwargs)

    def reject(self, lines, message):
        with self.assertRaisesRegex(AUDIT.FleetAuditError, message):
            self.analyze(lines)

    def test_complete_ordered_trace_is_structural_only(self):
        result = self.analyze(complete(), require_finish=True, require_all_rivals=True)
        self.assertEqual(result["reported_complete_attempts"], 1)
        self.assertEqual(result["reported_all_rivals_moved_attempts"], 1)
        row = result["attempts"][0]
        self.assertEqual(row["cars"][2]["accepted_gates"], 4)
        self.assertEqual(row["cars"][3]["failed_recoveries"], 1)
        self.assertFalse(row["finish"]["saved_rewards_reported"])
        for key in ("source_authenticity_verified", "unreal_build_verified",
                    "installed_gameplay_verified", "saved_data_integrity_verified",
                    "ready_for_release"):
            self.assertIs(result[key], False)

    def test_attempt_gap_after_solo_or_career_is_allowed(self):
        result = self.analyze([begin(1), event("fault", "reason=2 slot=-1"), *complete(4)])
        self.assertEqual([a["attempt"] for a in result["attempts"]], [1, 4])
        self.assertEqual(result["attempts"][0]["phase"], "faulted")
        self.assertEqual(result["reported_complete_attempts"], 1)
        self.reject([begin(4), begin(3)], "attempt ID did not increase")

    def test_unknown_and_duplicate_actor_identity(self):
        self.reject([begin(), roster().replace("id5=105", "id5=104")], "duplicate identity")
        self.reject([begin(), roster(), go(), gate(1, 1, 1).replace("id=101", "id=999")],
                    "actor identity changed")
        self.reject([begin(), roster(), roster()], "duplicate or late roster")
        self.reject([begin(), go()], "GO without exact bound roster")

    def test_startup_and_terminal_guards(self):
        self.reject([begin(), roster(), go().replace("stable=0.300000000", "stable=0.100000000")],
                    "continuous grid readiness")
        self.reject([begin(), roster(), go().replace("startup=3.250000000", "startup=1.000000000")],
                    "qualified startup window")
        self.reject([begin(), roster(), go().replace("frame=40", "frame=1")], "predates roster")
        self.reject([begin(), roster(), gate(1, 1, 1)], "outside running attempt")
        self.reject([begin(), event("fault", "reason=9 slot=3"), roster()], "duplicate or late roster")
        self.reject([*complete(), gate(1, 2, 8)], "outside running attempt")

    def test_gate_sequence_clock_and_recovery_guards(self):
        prefix = [begin(), roster(), go()]
        self.reject([*prefix, gate(1, 2, 1)], "skipped or duplicated")
        self.reject([*prefix, gate(1, 1, 1), gate(1, 1, 2)], "skipped or duplicated")
        self.reject([*prefix, gate(1, 1, 1), gate(1, 2, 1)], "strictly advance")
        self.reject([*prefix, gate(1, 1, 2), gate(2, 1, 1)], "shared active clock")
        self.reject([*prefix, gate(1, 1, 0)], "positive active clock")
        self.reject([*prefix, event("recovery", "slot=1 id=101 ordinal=2 verified=1")],
                    "recovery ordinal")
        self.reject([*prefix, event("recovery", "slot=0 id=100 ordinal=1 verified=0")],
                    "unverified player recovery")
        self.reject([*prefix, gate(0, 1, 1),
                     event("finish", "clock=1.000000000 fees=0.000000000 timing=1 review=1 completed_routes=0 saved=0")],
                    "all ordered player gates")

    def test_finish_cannot_invent_time_rewards_or_routes(self):
        lines = complete()
        for old, new, reason in (
            ("clock=7.000000000", "clock=8.000000000", "finish clock"),
            ("fees=5.000000000", "fees=0.000000000", "omit a verified"),
            ("completed_routes=1", "completed_routes=2", "invents opponent"),
            ("completed_routes=1", "completed_routes=0", "disagree"),
            ("timing=1", "timing=0", "ineligible timing"),
            ("saved=0", "saved=1", "malformed"),
        ):
            with self.subTest(reason=reason):
                self.reject([*lines[:-1], lines[-1].replace(old, new)], reason)

    def test_refused_receipt_never_qualifies_complete_run(self):
        lines = complete()
        lines.insert(-2, event("refused", "kind=gate slot=5 ordinal=2"))
        report = self.analyze(lines)
        self.assertEqual(report["reported_complete_attempts"], 0)
        self.assertEqual(len(report["attempts"][0]["refused_receipts"]), 1)
        with self.assertRaisesRegex(AUDIT.FleetAuditError, "without refused receipts"):
            self.analyze(lines, require_finish=True)

    def test_all_rivals_requirement_is_explicit(self):
        lines = [line for line in complete() if "action=gate" not in line or "slot=5" not in line]
        self.assertEqual(self.analyze(lines, require_finish=True)["reported_complete_attempts"], 1)
        with self.assertRaisesRegex(AUDIT.FleetAuditError, "all five rivals"):
            self.analyze(lines, require_all_rivals=True)

    def test_grammar_bounds_and_truncated_trace(self):
        self.reject(["unrelated log"], "no experimental")
        self.reject([begin().replace("v=1", "v=2")], "malformed fleet")
        self.reject([begin() + " extra=0"], "malformed or extra")
        self.reject([begin().replace("attempt=1", "attempt=18446744073709551616")],
                    "attempt outside accepted range")
        self.reject([begin() + " " * 1100], "oversized")
        self.reject([begin() + " " + begin()], "ambiguous")
        report = self.analyze([begin(), roster(), go()])
        self.assertEqual(report["attempts"][0]["phase"], "running")
        with self.assertRaisesRegex(AUDIT.FleetAuditError, "no complete"):
            self.analyze([begin(), roster(), go()], require_finish=True)

    def test_cli_hashes_exact_bytes_without_writing_game_files(self):
        with tempfile.TemporaryDirectory() as temp:
            log = Path(temp) / "real-shaped-fixture.log"
            data = ("\r\n".join(complete()) + "\r\n").encode("utf-8")
            log.write_bytes(data)
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(log), "--require-finish", "--require-all-rivals"],
                capture_output=True, text=True, check=True,
            )
            report = json.loads(result.stdout)
            import hashlib
            self.assertEqual(report["log_sha256"], hashlib.sha256(data).hexdigest())
            self.assertEqual(report["log_bytes"], len(data))
            self.assertEqual(log.read_bytes(), data)


class FleetAuditSourceBindingTests(unittest.TestCase):
    def test_logs_are_after_real_receipt_acceptance(self):
        mode = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text()
        runtime = (ROOT / "Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp").read_text()
        self.assertLess(mode.index("FleetAttempt.CommitGreen("),
                        mode.index("LATEBRAKE_FLEET_AUDIT v=1 action=go"))
        self.assertLess(mode.index("Race.PassCheckpoint("),
                        mode.index("LATEBRAKE_FLEET_AUDIT v=1 action=gate"))
        self.assertLess(runtime.index("FleetReviewLedger.RecordAcceptedGate("),
                        runtime.index("LATEBRAKE_FLEET_AUDIT v=1 action=gate"))
        self.assertLess(runtime.index("FleetReviewLedger.RecordRecoveryResult("),
                        runtime.index("LATEBRAKE_FLEET_AUDIT v=1 action=recovery"))
        finish = mode.index("LATEBRAKE_FLEET_AUDIT v=1 action=finish")
        save = mode.index("latebrake::ProfileWriteCheckpoint SavedProfile", finish)
        self.assertIn("return;", mode[finish:save])
        self.assertIn("saved=0", mode[finish:save])

    def test_explicit_private_launch_flag_preserves_default(self):
        script = (ROOT / "scripts/Run-WindowsPlaytest.ps1").read_text()
        self.assertIn("[switch]$ExperimentalSixCarGrid", script)
        self.assertIn("if ($ExperimentalSixCarGrid)", script)
        self.assertIn("$argsForUnreal += '-LatebrakeExperimentalSixCarGrid'", script)
        self.assertLess(script.index("Test-CandidateManifest.ps1"),
                        script.index("if ($ExperimentalSixCarGrid)"))


if __name__ == "__main__":
    unittest.main()
