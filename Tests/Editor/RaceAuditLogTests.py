"""Synthetic race audit parser and actual GameMode callsite contracts.

No test here creates an Unreal game, invokes licensed UHT, or proves that
the text came from an installed game. Test fixtures are intentionally fake.
"""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from AnalyzeRaceAudit import RaceAuditError, analyze_race_audit  # noqa: E402


def begin(attempt=1, laps=1, gates=4, track=1, grid=-1):
    return (
        f"LogLatebrakeRaceAudit: Display: LATEBRAKE_RACE_AUDIT v=1 "
        f"action=begin attempt={attempt} gates={gates} laps={laps} "
        f"track={track} grid={grid}"
    )


def gate(attempt=1, gate_index=0, lap=0, next_index=1,
         elapsed="1.000000", active=0, finished=0):
    return (
        f"LogLatebrakeRaceAudit: Display: LATEBRAKE_RACE_AUDIT v=1 "
        f"action=gate attempt={attempt} gate={gate_index} lap={lap} "
        f"next={next_index} elapsed={elapsed} active={active} "
        f"finished={finished}"
    )


def run(attempt=1, laps=1, grid=-1):
    lines = [begin(attempt, laps=laps, grid=grid)]
    for i in range(laps * 4):
        lap = (i + 1) // 4
        lines.append(gate(
            attempt, i % 4, lap, (i + 1) % 4,
            elapsed=f"{i + 1}.000000",
            active=lap, finished=int(lap == laps),
        ))
    return lines


class AnalyzeRaceAuditTests(unittest.TestCase):
    def test_finished_circuit_and_quick_time_attack(self):
        report = analyze_race_audit("\n".join(run()), require_finish=True)
        self.assertEqual(report["attempt_count"], 1)
        self.assertEqual(report["accepted_gate_events"], 4)
        self.assertEqual(report["reported_finished_attempts"], 1)
        self.assertTrue(report["attempts"][0]["finished"])
        self.assertEqual(report["attempts"][0]["active_timed_laps"], 1)
        self.assertFalse(report["ready_for_release"])
        self.assertFalse(report["installed_gameplay_verified"])

    def test_career_three_laps_four_gate_laps_and_restart(self):
        first = [begin(laps=3), gate(elapsed="2.000000")]
        replay = run(attempt=2, laps=3, grid=1)
        report = analyze_race_audit("\n".join(first + replay), require_finish=True)
        self.assertEqual(report["attempt_count"], 2)
        self.assertEqual(report["accepted_gate_events"], 13)
        self.assertEqual(report["reported_finished_attempts"], 1)
        self.assertEqual(report["attempts"][0]["completed_laps"], 0)
        self.assertEqual(report["attempts"][1]["completed_laps"], 3)

    def test_independent_clock_baselines_for_attempts(self):
        logs = run(attempt=1) + run(attempt=2, grid=1)
        report = analyze_race_audit("\n".join(logs), require_finish=True)
        self.assertEqual(report["reported_finished_attempts"], 2)

    def test_incomplete_real_attempt_must_not_count_as_finish(self):
        text = "\n".join(run()[:-1])
        self.assertEqual(analyze_race_audit(text)["reported_finished_attempts"], 0)
        with self.assertRaisesRegex(RaceAuditError, "no complete ordered race finish"):
            analyze_race_audit(text, require_finish=True)

    def test_reject_skipped_duplicate_or_forged_gate(self):
        start = begin()
        for sample in (
            gate(gate_index=1, next_index=2),
            gate(gate_index=0, next_index=2),
            gate(gate_index=0, lap=1, next_index=1),
            gate(gate_index=0, finished=1),
            gate(gate_index=0, attempt=2),
        ):
            with self.subTest(sample=sample):
                with self.assertRaises(RaceAuditError):
                    analyze_race_audit(start + "\n" + sample)
        with self.assertRaisesRegex(RaceAuditError, "duplicated"):
            analyze_race_audit("\n".join([start, gate(), gate()]))

    def test_reject_corrupted_attempt_boundaries(self):
        examples = (
            begin(attempt=2),
            begin() + "\n" + begin(),
            begin() + "\n" + begin(attempt=3, grid=1),
            begin() + "\n" + begin(attempt=2, grid=-1),
            gate(),
            begin(attempt=MAX_ID + 1),
        )
        for log in examples:
            with self.subTest(log=log[:60]):
                with self.assertRaises(RaceAuditError):
                    analyze_race_audit(log)

    def test_reject_blocked_track_grid_and_missing_state(self):
        for bad_start in (begin(track=0), begin(grid=0)):
            with self.subTest(bad_start=bad_start):
                with self.assertRaisesRegex(RaceAuditError, "blocked"):
                    analyze_race_audit(bad_start + "\n" + gate())
        for invalid in (begin(track=2), begin(grid=-2),
                        begin(gates=999), begin(laps=101)):
            with self.subTest(invalid=invalid):
                with self.assertRaises(RaceAuditError):
                    analyze_race_audit(invalid)

    def test_reject_bad_active_clock_or_out_of_order_times(self):
        valid = "\n".join([begin(), gate(elapsed="2.000000")])
        for bad in (
            gate(gate_index=1, next_index=2, elapsed="1.900000"),
            gate(gate_index=1, next_index=2, elapsed="nan"),
            gate(gate_index=1, next_index=2, elapsed="-1.000000"),
            gate(gate_index=1, next_index=2, elapsed="3.000000", active=1),
            gate(gate_index=1, next_index=2, elapsed="999999.000000"),
        ):
            with self.subTest(bad=bad):
                with self.assertRaises(RaceAuditError):
                    analyze_race_audit(valid + "\n" + bad)

    def test_reject_fake_finish_or_active_laps(self):
        logs = run(laps=2)
        tampered = logs.copy()
        tampered[-1] = gate(gate_index=3, lap=2, next_index=0,
                            elapsed="8.000000", active=4, finished=1)
        with self.assertRaisesRegex(RaceAuditError, "active lap"):
            analyze_race_audit("\n".join(tampered))
        tampered = logs.copy()
        tampered[-1] = gate(gate_index=3, lap=2, next_index=0,
                            elapsed="8.000000", active=2, finished=0)
        with self.assertRaisesRegex(RaceAuditError, "finish flag"):
            analyze_race_audit("\n".join(tampered))

    def test_reject_malformed_extra_fields_and_unsupported_schema(self):
        for bad in (
            begin().replace("v=1", "v=2"),
            begin() + " extra=1",
            gate() + " extra=1",
            "LATEBRAKE_RACE_AUDIT unknown payload",
            begin() + "\n" + "LATEBRAKE_RACE_AUDIT action=begin attempt=2",
        ):
            with self.subTest(bad=bad[:60]):
                with self.assertRaisesRegex(RaceAuditError, "malformed"):
                    analyze_race_audit(bad)

    def test_normal_unreal_lines_are_ignored_but_empty_audit_rejected(self):
        report = analyze_race_audit(
            "LogInit: running\n" + begin() + "\nLogTemp: unrelated"
        )
        self.assertEqual(report["accepted_gate_events"], 0)
        with self.assertRaisesRegex(RaceAuditError, "no complete"):
            analyze_race_audit("LogInit: no audit")

    def test_game_mode_source_only_logs_after_real_acceptance(self):
        source = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text(
            encoding="utf-8"
        )
        header = (ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h").read_text(
            encoding="utf-8"
        )
        register = source.split("void ALatebrakeRaceGameMode::RegisterCheckpoint(", 1)[1]
        register = register.split("void ALatebrakeRaceGameMode::", 1)[0]
        self.assertLess(
            register.index("if (!Race.PassCheckpoint(CheckpointIndex))"),
            register.index("action=gate attempt=%d"),
        )
        self.assertLess(register.index("action=gate attempt=%d"),
                        register.index("if (!Race.IsFinished())"))
        self.assertIn("Race.GetActiveTimedLapCount()", register)
        self.assertIn("Race.GetNextCheckpoint()", register)
        self.assertIn("Race.IsFinished() ? 1 : 0", register)
        self.assertEqual(register.count("LATEBRAKE_RACE_AUDIT"), 1)
        start = source.split("void ALatebrakeRaceGameMode::BeginPlay()", 1)[1]
        self.assertLess(start.index("Race.Reset();"), start.index("StartRaceAuditAttempt(-1);"))
        restart = source.split("void ALatebrakeRaceGameMode::RestartRace()", 1)[1]
        self.assertLess(restart.index("Race.Reset();"),
                        restart.index("StartRaceAuditAttempt(bPlacedOnGrid ? 1 : 0)"))
        self.assertIn("#if !UE_BUILD_SHIPPING", header)
        self.assertIn("int32 RaceAuditAttempt = 0;", header)
        self.assertIn("std::numeric_limits<int32>::max()", source)
        self.assertNotIn("LATEBRAKE_RACE_AUDIT", (
            ROOT / "Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp"
        ).read_text(encoding="utf-8"))


MAX_ID = 2147483647

if __name__ == "__main__":
    unittest.main()
