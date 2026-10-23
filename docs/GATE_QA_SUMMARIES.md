# v0.5.4 — Bounded Checkpoint QA Summaries (development)

**Unreleased source development only. No Win64 EXE, Android APK or Unreal game verification.**

## Motivation

In v0.5.3, individual checkpoint decisions can be explained, but
logging rejection events on every driven frame can produce excessive
noise. v0.5.4 adds fixed-memory event counts and a separate periodic
summary mode to help QA teams understand common failure categories.

## Portable C++20 API

`Racing/GateDiagnosticCounters.h` defines `GateDiagnosticCounters`:

- `Observe(CheckpointTraceResult)`: increments one trace-category count
  and, for `GateRejected` only, one classified gate-rejection count.
  Initial baselines and teleport rebase events do not become geometry
  rejection counts.
- `TotalSamples()`, `CountTrace(...)`, `CountGateRejection(...)`:
  inspect the current window; unknown enum values fail closed.
- `Merge(other)`: combine fixed-size snapshots with saturating
  32-bit counts, including a self-merge.
- `Reset()`: clear all previous statistics before the next window.

No heap allocation, identifiers, exact player positions or network
subsystems are involved. Counts never wrap around after overflow.

## Development console levels (UNVERIFIED in Unreal)

- `latebrake.CheckpointDiagnostics 0`: default, silent. No QA counters
  collected or transmitted.
- `latebrake.CheckpointDiagnostics 1`: v0.5.3 opt-in local
  transition/event logs. No periodic summary.
- `latebrake.CheckpointDiagnostics 2`: v0.5.4 opt-in local summary
  approximately every ten seconds, per checkpoint. It counts accepted
  gates, intentional teleport resets, invalid positions, wrong-way
  velocity, excessive frame travel, lateral/vertical misses and
  noncrossing samples. It does not print per-frame coordinates, and
  mode 2 does not enable mode 1 per-event logging.

The code exists only as **non-shipping diagnostic logging**. No claim
is made that this console variable has been executed on UE 5.8,
packaged Win64, or physical Android devices.


## v0.5.31: separate invalid-velocity samples in mode 2

The trace-continuity guard introduced in v0.5.30 drops stale
checkpoint baselines on NaN/infinite horizontal velocity readings.
The bounded diagnostic counter already classified such rejections,
but the original v0.5.4 summary text did not expose its total.

Opt-in, **non-shipping** `latebrake.CheckpointDiagnostics 2` source now
appends `invalid-velocity=<uint32>` to each local window's one-line
summary. The value counts `GateRejected / InvalidVelocity` results
only; it is not a new game scoring category and does not contain
coordinates, personal data, or a per-frame log.

The local parser remains backward compatible: historical
`v0.5.4`-format summary rows have no `invalid-velocity` column and
their JSON output supplies zero for it, whereas new rows count it
explicitly. Malformed, duplicated, overflowing or sample-budget-
inconsistent new columns are rejected rather than silently discarded.

Python parser and C++20 counter regressions are synthetic. To certify
the real runtime, run the licensed UE 5.8 game, enable diagnostics,
cause controlled invalid velocity in a **disposable QA level**,
capture an actual device log, and confirm that exactly the expected
new category appears. There is currently **no such real test evidence**.

## Summarize a captured local Unreal test log

```sh
python3 scripts/AnalyzeCheckpointQA.py /path/to/your/Latebrake.log --require-all-gates
```

The script reads only an existing file and prints a JSON summary to
standard output. It requires all four gate indices with
`--require-all-gates`. It rejects incomplete/oversized counter rows,
impossible category totals, invalid checkpoint indices, missing reports,
and file inputs larger than 32 MiB. Its output always records
`source_authenticity_verified: false` and
`unreal_build_or_gameplay_verified: false`: **a text parser cannot
authenticate the real engine, installed hardware, source build SHA or
playability**. Synthetic CI fixtures exist solely for parser regression
testing. Do not attach them as QA evidence.

The reporter intentionally aggregates counts only. Capture independent
real device/build identifiers, the exact source revision, genuine
runtime startup marker verification, and human race progression testing
to satisfy open release blockers. Local logs can themselves contain
user-sensitive paths; review before sharing them.

## Required acceptance

- CI: strict C++20 `GateDiagnosticCounters` target on Windows,
  Ubuntu and macOS; preserve existing `GateDiagnostic` parity tests.
- Genuine UE 5.8/UHT game/editor compile and a cookable source-owned
  circuit. Verify console mode 0, mode 1 and mode 2 on installed game.
- Test 30/60 FPS and artificial hitches, all four gates, long
  displacements, deliberate short recovery, restart, pawn re-possession.
- Verify per-gate window reset on controller/pawn loss and mode
  transitions. Check QA overhead and avoid logging personal data.
- Record actual device models, build SHA, generated log and manual
  results in issues #2, #13 and #31. Do not close QA blockers based only
  on portable tests.
