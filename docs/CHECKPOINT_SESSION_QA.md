# LateBrake v0.5.11 — Checkpoint Pause/Resume Session QA

**Source-only, unreleased development. No gameplay or device certification.**

## Source problem

Checkpoint actors sample the moving player on tick and use the two latest
positions to check forward plane crossings. When world ticking is suspended
by a menu pause, the first position after unpause can be far from the
last position before the pause. A plausible crossing across that interval
is *not* evidence of continuous active-race driving.

## Source contract

- Each `ToggleMenu()` pause/resume and the GameMode-controlled
  `ToggleRacePause()` Escape/gamepad path changes
  `CheckpointSessionRevision`. Every checkpoint Actor reads it.
- `CheckpointSessionTrace` clears its previous sample on revision
  change and treats the next sample as `InitialBaseline`, regardless
  of the car's separate teleport/recovery generation.
- Missing GameMode, incomplete track, paused world, open menu, missing
  player or pawn swap cannot contribute stale position samples.
- Bounded non-shipping checkpoint diagnostic windows are reset on a
  new session, not mixed across menu interruptions.
- `RegisterCheckpoint` refuses scoring without a valid local player,
  if the track failed startup, while paused or with a menu open.
  The timer likewise treats menu focus as non-racing time.

## C++ source test

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
```

`CheckpointSessionTraceTests.cpp` checks all four directional gates
with original `RaceProgress.cpp`: plausible motion across a pause
gets no award; real forward crossing following the new baseline
still earns the correct ordered checkpoint. It also exercises teleport
revision changes, pawn reset, invalid coordinates and session wrap.
`PlayerRaceClockReadinessTests.cpp` independently guards awards.

## Genuine Unreal qualification — outstanding

1. Compile installed licensed Unreal Engine 5.8 UHT/editor/game targets.
2. Generate and cook the actual `/Game/Maps/LatebrakeCircuit` map.
3. Run a real Win64 candidate with checkpoint diagnostics enabled.
   Drive to the approach side of each gate, open the pause menu, and
   resume after interruption or repositioning in a disposable QA level.
   Verify *no* gate or finish/credits is granted from a crossing
   made solely between pre-pause and post-pause positions.
4. Verify uninterrupted forward driving still scores each of the
   four ordered gates, including frame hitches inside the sweep cap.
5. Repeat with gamepad, keyboard and Android touch pause/menu focus;
   verify no touch action unexpectedly awards a gate or recovery.
6. Test missing-player pawn, track-startup failure, race finish and
   restart with normal and corrupted input. Record source SHA,
   installed package SHA, engine version and real device logs.

**Passing the C++ test is not proof of Unreal Tick semantics, vehicle
physics, map cooking, on-screen UI or Android input.** Do not attach
a playable EXE/APK/AAB to a release before real qualification.
