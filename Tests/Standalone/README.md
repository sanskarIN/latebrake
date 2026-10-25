# Portable C++20 test suite

The portable suite compiles `Source/Latebrake/Private/Racing/RaceProgress.cpp` from the game source rather than copying the implementation into test fixtures. This allows Linux, Windows and macOS CI to catch regressions in game-independent race rules without installing Unreal Engine.

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --parallel
ctest --test-dir Tests/Standalone/build --output-on-failure
```

The v0.5.15 `RaceSessionEpochTests.cpp` links the actual `RaceProgress.cpp`
and uses a real directional circuit gate to verify that exiting menus,
pausing, and restarting cannot reuse a pre-transition checkpoint sample.
The Unreal GameMode routes successful Quick Race/career/replay exits
through the shared epoch policy; a passing portable test is not a
substitute for an actual Unreal package/playtest.

The v0.5.16 `TouchFingerRangeTests.cpp` test covers the 16-finger
ownership policy shared between the Unreal press tracker and portable
drive state. Untrackable IDs cannot acquire steering, throttle or brake;
invalid viewport geometry still cancels held drive inputs.

The v0.5.17 `LiveRacePaceProjectionTests.cpp` test links actual
`RaceProgress.cpp` to validate estimated finish clocks from the last
three completed timed laps. It excludes invalid/finished/zero-clock
cases and limits slow-lap estimates to not predate elapsed time.
These estimates are presentation only, not official race records.

The v0.5.18 `RivalDriveReadinessTests.cpp` directly links real
`RaceProgress.cpp` to reject AI driving without a ready circuit or
local driver, during pause/menu, after finish or in the wrong event.
These pure policy checks do not replace a real engine physics test.

The v0.5.19 `RivalRouteTests.cpp` exercises the full finite AI
recovery budget: every accepted teleport is counted, no further
teleport is allowed after exhaustion, and race reset refreshes state.
Source-level route tests are not substitutes for Unreal physics QA.

The v0.5.20 `RecentTimedLapsTests.cpp` now rejects duplicates
and backdated lap IDs, both in a fresh session and after the fixed-size
buffer wraps. Valid strictly increasing records and reset remain supported.
These are engine-independent history guarantees, not installed-game QA.

The v0.5.21 touch regression exercises screen geometry interruption
for both `Press` and `Move`, including unknown/negative touch IDs.
An invalid viewport must cancel **previously held** throttle, brake
and steering, not just refuse the incoming new touch. Stale moves
cannot reacquire control until a fresh valid press. Physical Android
rotation/resume behavior remains an Unreal/device QA requirement.

The v0.5.22 touch tests verify that a malformed press from an
existing steering or pedal owner cancels **only that owner's held
control**, while another valid finger and a valid duplicate press
remain unaffected. A stray invalid or out-of-range finger can never
steal input. Actual Android input/focus QA remains open.

The v0.5.23 `TouchPressTrackerTests.cpp` also exercises the
`AdmitTouchPress` API now invoked directly by the Unreal pawn's
`TouchPressed` callback. An already-down finger's malformed repeated
press must cancel only its own throttle or steering, retain the
physical-down latch (no repeat action), and zero the relevant smoothed
output in the engine caller. Genuine first presses—including malformed
first presses—are still passed to `ApplyTouch` for viewport validation.
The tests verify admission/drive ownership but cannot execute the Unreal
callback, sample smoothing in the engine, or qualify physical Android
touch delivery. See [Android issue #13](https://github.com/sanskarIN/latebrake/issues/13).

The v0.5.26 `PlayerRecoveryReadinessTests.cpp` links the original
`RaceProgress.cpp` and exercises the same circuit, pause/menu, local
player and finished-race readiness rule used for real gate scoring.
`TryChargePlayerRecoveryIfReady` must reject inactive races **without
charging time or incrementing recovery count**, and must still charge
a full five-second penalty for valid recovery. Overflow/precision
rejection continues to fail closed. The actual Unreal recovery and
mobile pause flows remain subject to genuine installed-game QA.

The v0.5.27 `RaceVehiclePhysicsReadinessTests.cpp` validates the
portable snapshot policy for the real `ALatebrakeCarPawn::IsRacePhysicsReady`
entry point: chassis/mesh presence, simulation, blocking physical collision,
finite 3D position and finite 3D velocity. It links original
`RaceProgress.cpp` and proves an unusable but still-possessed pawn does not
accumulate phantom lap time or qualify for checkpoints. Checkpoint samplers
also discard their baseline while physics is invalid, avoiding cross-failure
trajectory interpolation. These source tests **cannot** establish that
UE 5.8 actually cooks a mesh, drives the chassis or blocks track actors;
the true Unreal build and installed driving tests remain required.

v0.5.28 adds `PausedTouchReleaseTests.cpp` which exercises the real
portable touch press/drive trackers across a menu pause: previously held
drive fingers and the PAUSE action keep their single-shot state, while
each physical release clears only that finger. New presses can reclaim
GAS/STEER or toggle PAUSE after the release; moving a freed finger alone
cannot reacquire an input. The `Tests/Editor/TouchPauseBindingContractsTests.py`
static source contract separately asserts that Unreal's `IE_Released`
binding has `bExecuteWhenPaused = true` and calls both tracker release
methods. Neither suite invokes Unreal touch delivery or Android hardware.

The v0.5.29 `TouchResidualSafetyTests.cpp` exercises the real portable
input-cancellation policy used by the Unreal Pawn: finger-scoped zeroing
of already-smoothed throttle and steering after a release, nonfinite
movement, GAS-to-neutral/BRAKE drag or leaving the steering band. A
second finger's independent command and the physical-down action latch
must remain unchanged. The
`Tests/Editor/TouchResidualBindingContractsTests.py` static source
regressions verify actual Unreal `TouchReleased` and `ApplyTouch` use
the helper. These fixtures do **not** constitute hardware physics QA.

The v0.5.30 `CheckpointVelocityContinuityTests.cpp` links the original
`RaceProgress.cpp` and demonstrates that a valid-position sample with
nonfinite velocity is a **full checkpoint trace discontinuity**, not
a future gate-scoring baseline. All four genuine ordered gates, corrupt
initial readings, NaN/±infinity, unchanged session revision, diagnostic
counts and clean subsequent crossings are covered. The corresponding
[engine/device QA plan](../../docs/CHECKPOINT_VELOCITY_QA.md) remains
required; these hosted C++20 tests do not validate real Unreal physics.

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
straddling simulated menu or standalone pause/resume revision changes,
followed by legitimate forward driving
in a new session. Pawn resets, teleport changes, malformed coordinates
and revision wrapping are also covered. The player-readiness suite
additionally tests the scoring guard after a real finish.
These do not execute Unreal actor Tick, real pause events or packaged
physical gameplay.


### v0.5.49 — checked rival recovery outcomes

`RivalRouteTests.cpp` now tests a pending recovery transaction,
exact-once physical outcome reporting, restart/round resets and a full
finite budget with mixed successful and failed attempts.
`RivalRecoveryOutcomeTests.cpp` links the actual production
`RecoveryTeleportVerification.cpp` and exercises reported outcomes for
healthy and rejected/misaligned/unstable engine snapshots:

```sh
ctest --test-dir Tests/Standalone/build -R '^(RivalRoute|RivalRecoveryOutcome)$' --output-on-failure
```

These test the pure C++ decision and accounting layers, **not** the
actual Unreal engine's teleport, Chaos simulation or live game build.
See [Rival recovery QA](../../docs/RIVAL_RECOVERY_QA.md).
