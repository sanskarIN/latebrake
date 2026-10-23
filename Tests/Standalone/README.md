# Portable C++20 test suite

The portable suite compiles `Source/Latebrake/Private/Racing/RaceProgress.cpp` from the game source rather than copying the implementation into test fixtures. This allows Linux, Windows and macOS CI to catch regressions in game-independent race rules without installing Unreal Engine.

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --parallel
ctest --test-dir Tests/Standalone/build --output-on-failure
```

These checks **do not** validate physics feel, Unreal Header Tool generation, runtime map, input events, save APIs, cooked binaries or Android devices. See `docs/TESTING.md` for the Unreal-specific acceptance plan.

## New in v0.2.1

The `RivalRoute` target verifies waypoint/lap tracking and eight-second stuck recovery; the `SteeringFilter` target validates frame-step smoothing, invalid inputs and overflow protection. Both test the exact same header-only C++20 logic used in the Unreal-source gameplay classes. These are not substitutes for a real Unreal Engine build or playable package.

## v0.3.1 tests

`HudLayout` exercises viewport-dependent UI spacing and corrupted geometry; `RecordValidation` guards saved race time and completion counters. Updated `GarageState` tests cover invalid masks, owned-car cycling, costs and affordability. All are pure C++20 checks and do not certify Unreal Engine runtime rendering, game builds or SaveGame IO.

## v0.3.2 tests

`DriverInputSafety` covers normalized axis/touch input, nonfinite sensor values, frame invalidity and throttle smoothing. `GarageTuningKey` covers when live vehicle tuning should be reapplied versus when a balance change is irrelevant. These suites test real portable header implementations only; game-mode, physics, Android touch, camera and saving still require a licensed Unreal Engine build and manual validation.

## v0.4.0 track suites

`CircuitLayout` tests road-gate-recovery consistency against the same pure geometry read by Unreal player/rival code. `TrackDecorations` validates that 28 planned lane stripes and 56 curb markers remain on asphalt while spectator stands are outside the drivable road. `CircuitMap` validates minimap normalization, clamping and NaN/infinite-position safeguards. None test Unreal asset cooking, drawing, physics/collision or real gameplay.


## v0.5.8 collision response contracts

`CollisionResponsePolicyTests.cpp` compiles with strict C++20.
It checks a lightweight, allocation-free decision on three blocking
channel flags. The Unreal GameMode supplies the flags from actual
engine components in a future authorized build, but this test does
not invoke Unreal or simulate 3D vehicle physics.


## v0.5.9 real RaceProgress timing policy

`latebrake_race_clock_readiness_tests` compiles actual
`Source/Latebrake/Private/Racing/RaceProgress.cpp` together with
`RaceClockReadiness.h`. It verifies source startup, pause, finish
and restart timing policy in C++20, plus checkpoint eligibility
under startup failure, pause, finish and reset. Unreal game Tick, real world
pause, HUD and physical gameplay still require licensed engine QA.

## v0.5.10 player-pawn race clock guard

`latebrake_player_race_clock_readiness_tests` links the real
`RaceProgress.cpp` with `PlayerRaceClockReadiness.h`. It checks
that a ready course without a valid local driver pawn cannot accrue
elapsed race time, that losing and returning a driver cannot create
a ghost lap duration, and that pause, finish and restart remain safe.
Unreal possession, spawns, HUD visibility and packaged devices are
**not** tested by the portable suite.

## v0.5.11 pause-session checkpoint trace

`latebrake_checkpoint_session_trace_tests` compiles the portable
`CheckpointSessionTrace.h` and links actual `RaceProgress.cpp`. All
four gate directions are tested against short plausible crossings
straddling a pause/resume, followed by legitimate forward driving
in a new session. Pawn resets, teleport changes, malformed coordinates
and revision wrapping are also covered. The player-readiness suite
additionally tests the scoring guard after a real finish.
These do not execute Unreal actor Tick, real pause events or packaged
physical gameplay.
