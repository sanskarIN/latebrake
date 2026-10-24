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

## Live current-lap sector potential (next source milestone)

The non-compact racing HUD now shows **LAP POTENTIAL (BEST SPLITS)** and
**VS BEST** after at least one genuine positive timed lap. This is a
within-session calculation from the real `RaceProgress` history, not
persistent ghost data, a guarantee of achievable pace or a new record.

- Current lap time already spent remains counted in full.
- For the currently driven sector, only the positive remainder of its
  best previously observed split is estimated; the clock is never rewound.
- Every later sector uses its best valid measured split **from this run**.
- A sector with no valid positive split suppresses the estimate instead
  of treating an unknown split as zero.
- Negative **VS BEST** means the *potential* could beat the achieved best
  lap, **not** that the player has earned a record or an award.
- Reset, official finish, invalid clocks or a seven-day recording-limit
  overrun hide the estimate. Compact/mobile HUDs do not show this extra line.

Portable `CurrentLapPotentialTests.cpp` links the genuine game
`RaceProgress.cpp` and covers first-lap suppression, a slow current
sector, newly improved splits, zero-second sectors, one-gate events,
restart/finish and record-limit rejection. These assertions do not
replace Unreal 5.8 Editor, HUD rendering and real Win64/Android QA.

## Live sector target (v0.5.33 source milestone)

The expanded/non-compact HUD additionally compares **only the currently
driven sector** with its **best positive completed split for that same gate**
in this race. The display says either:

- `SECTOR N  Xs UNTIL PREVIOUS BEST` while the unfinished sector has not
  exceeded that best duration.
- `SECTOR N  OVER PREVIOUS BEST  +Xs` after the current sector has
  already taken longer than the previously measured best.

A partially completed sector is **never** labeled "ahead by X seconds";
elapsed time left before the prior best is not a predicted finish time.
A real timed lap must exist. Reset, finish, an unavailable positive
reference or an invalid / unrepresentable clock hides the feedback.

The implementation in `Racing/LiveSectorTarget.h` only reads
`RaceProgress`. Its C++20 regression links the real
`RaceProgress.cpp` and checks best-sector separation, exact-best timing,
overtime, newly improved references, missing/zero-length splits, finish,
restart, single-sector configuration and excessively long clocks.

No record, medal, credit, SaveGame field, or track crossing changes here.
Cross-platform source CI cannot verify actual Unreal HUD rendering,
driver physics, Win64 installation or Android hardware QA.

## Recovery/time penalty accounting (v0.5.36 source)

The portable `RaceProgress` now tracks an allocation-free count and sum of
**successfully charged** positive time penalties during the current event.
Rejected recovery charges, malformed values, paused actions and post-finish
attempts do not increment either counter. Reset clears both values.

The full charged fee is already included in `TIME`, completed lap times and
sector timing. The expanded HUD and results menu expose the count and
`+Xs ALREADY IN TIME` indicator. **Do not add it again to final results.**
The counter is not a separate recovery authorization and is not a saved
profile attribute. Unreal in-game rendering and device recovery behavior
still need evidence.
