# Sector timing and theoretical lap (unreleased v0.5.1)

> This is development **source**, not a verified playable game. Unreal Engine
> 5.8 compilation, in-editor driving, a real Win64 game package, and Android
> device qualification remain open requirements.

## What's tracked

The same engine-independent `RaceProgress` already driving ordered checkpoints
now keeps positive best times for each **matching** gate sector.

- A sector starts at the previous accepted checkpoint and ends at the next
  accepted ordered checkpoint. For sector 1 on lap 1, the start is the race
  start; every subsequent lap uses the preceding gate.
- `LAST SPLIT` shows time for the most recently accepted sector, including
  any recovery/time penalties charged inside that sector.
- `DELTA` compares the last valid sector **only** against the prior best for
  that exact checkpoint, not an unrelated sector or another racing mode.
  A negative delta is an improvement; a positive delta is slower. The first
  observation has no prior comparison, so it is explicitly labelled first.
- `THEORETICAL LAP (THIS RUN)` sums the best strictly positive observations
  of **all** sectors in the current run. It is intentionally not labelled
  as an achieved lap or a world record. It stays unavailable until every
  sector has at least one valid observation.
- `FASTEST LAP` continues to mean the fastest **actually completed** lap.
  A theoretical lap can be faster than anything the driver has ever
  completed in a single lap.

A restart, event switch, or new career event resets per-sector records.
Neither sector bests nor theoretical lap is persisted to `SaveGame`.
Historical event fastest-lap storage and the v2 career schema are unchanged.

## Correctness and limits

- Only ordered checkpoint transitions update split state.
- Repeated or incorrect gate callbacks cannot affect the prior sector value.
- Zero-second or nonfinite sector timings never set unbeatable records.
- The number of checkpoints is bounded to 1–256, using fixed-capacity
  allocation-free storage for this early game prototype.
- Extreme nonfinite / overflowing theoretical sums are rejected rather
  than displayed as valid racing times.
- Finish freezes the split state; resetting clears all comparisons.
- Desktop/non-compact HUD shows delta and ideal lap only when meaningful.
  The compact Android HUD intentionally prioritizes driving controls.
- This is **not** spatial validation, anti-cheat, a race replay or a saved
  ghost. The current four-gate prototype still requires installed playtesting.

## Portable regression test

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
```

The `SectorTiming` target includes both the portable `RaceProgress.cpp`
implementation and isolated timing tests. Its cases include first records,
ordered/duplicate gates, valid and invalid splits, positive/negative deltas,
independent sector records, reset, finish, recovery penalty accounting,
fixed-capacity bounds and clock overflow.

## Still required on a real UE 5.8 installation

1. Confirm actual gate overlap time vs HUD split values at different frame
   rates, including low-FPS spikes and pause/resume.
2. Drive deliberately faster and slower laps; verify signs and exact
   same-sector comparisons.
3. Trigger a five-second recovery penalty within a sector; ensure no
   artificially fast theoretical lap is awarded.
4. Restart, switch quick race modes, and start a career event; confirm
   no stale sector records or misleading pre-record deltas.
5. Play at desktop sizes, ultrawide and Android portrait/landscape. Verify
   telemetry does not overlap the minimap, race UI or touch pads.
6. Verify the existing real saved fastest-lap data is not overwritten by
   theoretical times. Test with a restored older save profile.

Do not claim these tests have been performed until actual engine, hardware,
logs and screenshots are recorded under issues #2 and #13.
