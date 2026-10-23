# v0.5.30 — Invalid-velocity checkpoint continuity QA

**Status:** Unreal-independent source change only. Not a playable game, Unreal compile, installed Windows build or Android validation.

## Problem

A checkpoint actor samples two consecutive player positions and evaluates a directed crossing from their motion. The existing `CheckpointMotionTrace` already rejects nonfinite positions and resets on recovery/restart teleport generations. It previously **retained** the last valid position when the new sample's **velocity** was NaN or infinity. A later normal-looking sample could be evaluated against a baseline measured during the corrupt physics interval, potentially granting a gate crossing that was not demonstrably continuous.

This issue can occur even when the position is finite. A malformed velocity sample must not become a trusted movement baseline.

## Source behavior in v0.5.30

- `CheckpointMotionTrace::SampleDetailed` checks **both horizontal velocity components** for finiteness **before** it captures an initial baseline or evaluates a gate.
- A nonfinite velocity resets the stored position and teleport generation, fails the current sample as `GateRejected / InvalidVelocity`, and does **not** credit `RaceProgress`.
- The next sample with finite position and finite velocity can only establish an `InitialBaseline`; it cannot award the old crossing. A **subsequent independent forward segment** can earn the gate using the unchanged directional, lane, height and maximum-frame-travel rules.
- These guards also apply inside `CheckpointSessionTrace` when session and teleport revisions remain unchanged. Previously valid samples are never joined across the corrupt physics interval.
- Existing non-shipping diagnostic mode 1 can classify the rejection as `invalid-velocity`. The fixed-memory category counter includes that rejection. Starting with v0.5.31 source development, opt-in mode-2 summary lines explicitly append an `invalid-velocity` count, while the updated parser still accepts older v0.5.4 lines without the field. Do not treat aggregate counts alone as authenticated Unreal evidence.

## Automated regression coverage

`Tests/Standalone/CheckpointVelocityContinuityTests.cpp` links the **real** `RaceProgress.cpp`, tests all four actual course gates and executes:

1. The **first** sample contains corrupt velocity. Confirm no valid baseline is acquired.
2. A valid pre-gate baseline is followed by NaN, +infinity, -infinity or both corrupt velocity components. Confirm the baseline is discarded on every failure.
3. The next valid position lands across the gate plane. It must return `InitialBaseline`, not `Crossed`, and the authoritative next-gate index must remain unchanged.
4. Only a fresh approach and independent forward sweep can count. Completing the four correct gates finishes exactly one actual `RaceProgress` lap.
5. Per-gate diagnostic counters classify every invalid-velocity sample. The same constraints apply to an unchanged checkpoint session/teleport revision.

Run the portable checks:

```bash
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
```

This verifies **only the source decision rules** on C++20 test compilers.

## Genuine Unreal game QA still required

On an authorized UE 5.8 build with a real cooked `/Game/Maps/LatebrakeCircuit`:

1. Confirm the existing `ALatebrakeCheckpoint::Tick` sends physical player sample positions and velocity to `MotionTrace.SampleDetailed`.
2. In a **disposable editor QA level** with proper instrumentation, simulate one invalid horizontal velocity reading *before* a gate followed by an apparent crossing on the next valid tick. There should be no awarded gate from the corrupt interval.
3. Verify a fresh valid approach → forward crossing still awards the correct gate once and does not interfere with ordinary low-FPS sweeps or recovery/restart epoch resets.
4. Check local `latebrake.CheckpointDiagnostics 1` output for `invalid-velocity`, with mode 0 silent by default. Do not infer success from a synthetic log.
5. Repeat with desktop and Android package candidates, preserving their source SHA, cooked map fingerprint and actual installed-playtest evidence.

The main release blockers remain [Unreal gate qualification #31](https://github.com/sanskarIN/latebrake/issues/31) and [Android device qualification #13](https://github.com/sanskarIN/latebrake/issues/13).

**No actual Unreal Engine build, forced corrupt runtime sample, installed executable or Android device test has been run in this source development step.**
