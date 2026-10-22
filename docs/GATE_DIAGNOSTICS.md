# v0.5.3 Checkpoint Decision Diagnostics (development)

**Source-only work in progress — not a v0.5.3 release or playable game.**

## Purpose

v0.5.2 changed checkpoint credit from sphere overlaps to directional
3D plane crossing. In a real Unreal Engine test build, a missed sector
could result from an invalid physics sample, velocity direction, large
frame hitch, wrong gate order, lane miss, elevated pawn or explicit
recovery/restart. A boolean by itself does not explain the gate math.

## Portable APIs

- `EvaluateCircuitGatePlane(gateIndex, previous, current, vx, vy)`
  returns one `CircuitGateDecision`. `Crossed` is the only accepting
  value. It tests the **same input and constraints** as the unchanged
  `CrossesCircuitGatePlane` boolean used by gameplay.
- `CircuitGateDecisionName(reason)` returns a static reason label,
  suitable for an opt-in textual diagnostic.
- `CheckpointMotionTrace::SampleDetailed(gateIndex, generation, position,
  vx, vy)` returns `CheckpointTraceResult` with a trace status and
  gate reason. Initial baselines, teleport generation rebases and
  invalid positions *never* award a gate.
- `CheckpointTraceDecisionName(reason)` exposes a static trace label.
  Existing `Sample(...)` still returns `bool` using the detailed
  evaluator. No heap allocations or Unreal Engine headers are required.

## Invariants

1. A legitimate forward cross at all four gates still credits only
   the expected next checkpoint in authoritative `RaceProgress`.
2. Reject >1,200 cm total 3D frame displacement. The 600 cm legacy
   visual radius is not a sample distance rule.
3. Recovery/restart increments the pawn teleport revision; its first
   new-generation location only resets the baseline.
4. Diagnostics are not automatically sent to telemetry endpoints;
   there is no network traffic, per-frame disk writing or logging.
5. Do not call portable CTest or diagnostic parity "Unreal QA".

## Tests and required real-engine follow-up

Run `cmake -S Tests/Standalone -B Tests/Standalone/build`,
`cmake --build Tests/Standalone/build --config Release`, and
`ctest --test-dir Tests/Standalone/build -C Release --output-on-failure`.
`GateDiagnostic` tests acceptance, rejection classes and teleport
rebases on every direction.

To qualify later, compile real UE 5.8 Editor/UHT/game targets on licensed
build agents, author/cook the actual starter map, and instrument an
**explicitly opt-in development build** to record gate, position sample,
velocity, time step, teleport generation, and named rejection reasons.
Remove personally identifying data before sharing traces. Test at
30 FPS / 60 FPS and artificial hitches, including full-lap, restart
and respawn. Record installed Win64/Android device and build SHA. Keep
[checkpoint qualification #31](https://github.com/sanskarIN/latebrake/issues/31)
open until those real-world results are documented.
