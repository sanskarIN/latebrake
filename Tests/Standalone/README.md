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


### v0.5.50 — fail-closed race restart placement

`RestartPlacementGuardTests.cpp` links the real production
`RaceProgress.cpp` plus player race-clock, checkpoint, paid recovery
and driving policies. It exercises unverified and rejected grid placement,
late duplicate engine results and verified explicit retries. The existing
`PlayerDriveReadinessTests.cpp` now exhaustively checks the new seventh
readiness bit in all 128 combinations.

```sh
ctest --test-dir Tests/Standalone/build -R '^(RestartPlacementGuard|PlayerDriveReadiness)$' --output-on-failure
```

Unreal bindings are also checked by
`Tests/Editor/RestartPlacementBindingTests.py`, but live engine
teleports and Windows/Android builds remain separately required.


### v0.5.51 — denied-restart recovery escape regression

`RestartPlacementGuardTests.cpp` now simulates a healthy old-position
chassis after a rejected engine RESTART, verifies that paid recovery
does not even invoke the teleport callback, then checks the intended
five-second charge after an explicitly verified restart. The Unreal
`RecoverPlayer()` callsite is checked in Python to require
`HasLocalPlayerDriver()` instead of only physical-body validity.

`PlayerDriveReadinessTests.cpp` also statically rejects a six-value
aggregate (omitted grid permission) and exercises all 128 complete
seven-bit states; `ManualRacePauseGuardTests.cpp` explicitly supplies
the verified start bit so the pause behavior itself remains tested.

```sh
ctest --test-dir Tests/Standalone/build -R '^(RestartPlacementGuard|PlayerDriveReadiness|ManualRacePauseGuard|RecoveryTeleportTransaction)$' --output-on-failure
```


### v0.5.52 — verified restart belongs to one pawn

`RestartPlacementGuardTests.cpp` now supplies the GameMode's
`sameVerifiedPawn` identity predicate to `CanRace`. It tests a
healthy replacement pawn after verified RESTART: engine-independent
race clock, ordered checkpoint eligibility, staged paid RECOVER and
motor force all remain denied without another accepted grid move.
A re-possessed original verified pawn may regain its own authorization;
a new actor must pass a fresh explicit RESTART.

`Tests/Editor/RestartPlacementBindingTests.py` separately requires the
actual GameMode to bind a generation-aware `TWeakObjectPtr` only after
successful placement and invalidate it before the next restart. The
test source does not execute Unreal, UHT, Chaos or a live pawn lifecycle.


### v0.5.53 — corner traction and wrong-way UI

`CornerTractionAssistTests.cpp` exercises actual non-allocating C++20
traction policy for both handling modes, full straight/launch demand,
strong lateral-slide intervention, backward/neutral throttle invariants,
finite bounds, monotonicity and nonfinite input rejection.

`WrongWayIndicatorTests.cpp` covers all four circuit directions,
corners/lane exclusion, sideways and low-speed motion, sustained
0.8-second opposite direction and immediate reset on world interruption.
`Tests/Editor/HandlingAndWayfindingBindingTests.py` asserts the real
Chaos pawn calls traction after its validity gate, while the HUD uses
the portable warning and resets it on teleport/session/pawn changes.

```sh
ctest --test-dir Tests/Standalone/build -R '^(CornerTractionAssist|WrongWayIndicator|SpeedSensitiveSteering)$' --output-on-failure
```

A passing policy suite is **not** a UE 5.8 compile or real Win64/
Android HUD/driving qualification. See
[handling and wayfinding QA](../../docs/HANDLING_WAYFINDING_QA.md).


### v0.5.60 — multi-subsystem racing expansion

New portable C++20 production-linked targets:

- `DrivingSessionStats` compiles real `Private/Racing/DrivingSessionStats.cpp`.
  It checks elapsed/distance units, brake-edge counting, peak speed,
  drift time, pause/teleport discontinuities, reset and bad frames.
- `CircuitRouteAdvisor` compiles `Private/Track/CircuitRouteAdvisor.cpp`.
  It checks the real circuit's next-gate coordinates, heading signs,
  wraparound, 53.5m direct start range, absent/rejected input.
- `RivalCornerAnticipation` compiles the real
  `Private/AI/RivalCornerAnticipation.cpp`. It checks all four bends,
  approach speed control, stop/steering preservation, bad AI inputs.

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(DrivingSessionStats|CircuitRouteAdvisor|RivalCornerAnticipation)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*IntegrationTests.py' -v
```

New source contracts verify `GameMode::RestartRace` resets telemetry,
actual Chaos Tick feeds its policy after finite guards, actual HUD
uses the player's authoritative next gate and the AI applies the
corner policy after computing `PlanRivalTurn`. No suite compiles
licensed UE 5.8 C++/UHT or replaces installed gameplay QA.


### v0.5.61 — on-road guidance and inbound rival corner checks

`OffTrackAdvisorTests.cpp` runs strict C++20 tests on the actual
four-rectangle paved circuit surface, including all shoulders, nearest
pavement displacement, nonfinite/extreme pose rejection and sustained
warning debounce. `RivalCornerAnticipationTests.cpp` extends the
v0.5.60 production-linked AI suite to reject beyond-gate and cross-lane
radial false positives on all four corners while preserving valid
inbound brake responses.

`Tests/Editor/OffTrackAndRivalApproachBindingTests.py` checks that the
real Unreal HUD hosts the advisor, resets it after discontinuities
and does not use it for race-scoring authorization.

```sh
ctest --test-dir Tests/Standalone/build -R '^(OffTrackAdvisor|RivalCornerAnticipation|WrongWayIndicator)$' --output-on-failure
```

Passing source tests does not imply licensed UE 5.8 compilation, Chaos
steering reliability or installed Win64/Android gameplay acceptance.


### v0.5.62 — physically grounded live race radar

`RaceMiniMapTests.cpp` compiles the real new
`Private/Track/RaceMiniMap.cpp` production projection. It tests
next-gate geometry, cardinal/wrapped yaw, player/rival marker order,
direct separation units, absent Time Attack opponent, disabled Chaos
physics, rejected restart, NaN/Infinity, off-display location and
zero-distance overlap.

`Tests/Editor/RaceMiniMapBindingTests.py` binds the actual Unreal
GameMode rival position check (real actor physics plus finite 3D pose)
to the full-width HUD radar, independently of ranking and reward
calculations.

```sh
ctest --test-dir Tests/Standalone/build -R '^(RaceMiniMap|CircuitMap)$' --output-on-failure
```

This is source-only validation; see [race radar QA](../../docs/RACE_RADAR_QA.md)
for the engine/UHT, cooked map, viewport and installed-device gate.

### v0.5.63 — real close-rival warning and results-menu scope fix

`RivalProximityIndicatorTests.cpp` compiles against the actual production
`Private/Track/RaceMiniMap.cpp` and tests the proximity state machine with
real geometric separations. It covers continuous 15m/0.6s entry, 20m/21m
exit, zero-distance overlap, race mode and rival eligibility, malformed
samples, long sessions and explicit resets.

`Tests/Editor/RivalProximityBindingTests.py` confirms the Unreal HUD uses a
single radar sample for the minimap and both-viewport notice, and that the
results-menu `Race` variable is declared before its use. These Python
source checks do not substitute for an actual UE compiler.

```sh
ctest --test-dir Tests/Standalone/build -R '^(RivalProximityIndicator|RaceMiniMap)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'RivalProximityBindingTests.py' -v
```

See [rival proximity device QA](../../docs/RIVAL_PROXIMITY_QA.md).

### v0.5.65 — grounded direct rival separation trends

`RivalRelativeMotionTests.cpp` links the actual production
`Private/Track/RaceMiniMap.cpp` and checks 0.4s sustained signed
CLOSING/SEPARATING, 2m/s jitter threshold, 90m/s jump reset, frame
timing rejection, missing/invalid rival state, reversing direction,
candidate boundedness and explicit teleport/session resets.
`Tests/Editor/RivalRelativeMotionBindingTests.py` binds the one real
HUD radar sample to the desktop and compact advisory renderers.

```sh
ctest --test-dir Tests/Standalone/build -C Release -R '^(RaceMiniMap|RivalProximityIndicator|RivalRelativeMotion)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'RivalRelativeMotionBindingTests.py' -v
```

No Unreal runtime, cooked map or installed Android/Win64 game has
been tested through this target; see
[installed device QA](../../docs/RIVAL_RELATIVE_MOTION_QA.md).

### v0.5.66 — bounded race HUD vertical layout

`HudVerticalPlanTests.cpp` strictly compiles the production
`UI/HudVerticalPlan.h` with the real `HudLayout` and touch-control
geometry. It checks the last permitted telemetry row fits before a
reserved save failure warning, PAUSE/FINISH has its own baseline, and
both remain clear of the physical lower Android steering and throttle
pads. It includes Windows/desktop, mobile landscape/portrait, minimum
dimensions and NaN/Infinity rejection.

`Tests/Editor/HudVerticalPlanBindingTests.py` checks actual Unreal
HUD callsite ordering and fixed footer. No source test qualifies Unreal
rendered font bounds, horizontal action-button overlap or actual
installed device gameplay.

```sh
ctest --test-dir Tests/Standalone/build -C Release -R '^(HudLayout|TouchControlLayout|HudVerticalPlan)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'HudVerticalPlanBindingTests.py' -v
```

See [real device QA](../../docs/HUD_VERTICAL_QA.md).

### v0.5.67 — font-measured phone-safe HUD text

`HudTextBoundsTests.cpp` compiles the production `HudTextBounds.h`
with actual `HudLayout` and `TouchControlLayout` code. It checks
the right edge of a phone's text lane against the closest action-pad
rectangles, full-width desktop fallback, invalid/nonfinite viewport
and font measurements, ellipsis suffix edge cases and logarithmic
prefix search. The Unreal integration source check ensures the
actual HUD passes `GetTextSize` measured width into the same
portable decision, including race/menu/footer/status messages.

```sh
ctest --test-dir Tests/Standalone/build -C Release -R '^(TouchControlLayout|HudVerticalPlan|HudTextBounds)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'HudTextBoundsBindingTests.py' -v
```

Pixel-accurate UE Canvas drawing, font DPI, Android device safe areas
and installed playable binaries still require actual licensed
UE 5.8 builds and device QA. See
[HUD text QA](../../docs/HUD_TEXT_WIDTH_QA.md).

### v0.5.68 — Results/Career menu vertical clipping safety

`HudMenuVerticalPlanTests.cpp` compiles the production layout policy
with the real `HudLayout` and `TouchControlLayout`, checking that
ordinary menu text, honest `MORE MENU DETAILS HIDDEN` disclosure and
the separately pinned SAVE FAILED banner clear actual Android
steering pixels and stay inside normal desktop viewports.

The Unreal source-binding test checks real `MenuLine` usage, clipped
row accounting and independent safe-banner rendering. This does not
verify Unreal/UMG typography, menu scroll interactions or Android APKs.

```sh
ctest --test-dir Tests/Standalone/build -C Release -R '^(HudVerticalPlan|HudTextBounds|HudMenuVerticalPlan)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'HudMenuVerticalBindingTests.py' -v
```

See [menu UI QA](../../docs/HUD_MENU_VERTICAL_QA.md).

### v0.5.69 — recoverable Results/Career menu detail pages

`MenuDetailPagerTests.cpp` compiles the production
`UI/MenuDetailPager.h` and verifies bounded multi-page lists,
partial tail-page visibility, next/previous boundary behavior,
changing viewport row capacity, changing page contents, and
page-category/close/reopen isolation.

`Tests/Editor/MenuDetailPaginationBindingTests.py` checks the
actual Unreal HUD saves all menu details before choosing a page,
and verifies that dedicated keyboard Up/Down actions and menu-only
Android UP/DOWN pads are routed while paused without stealing the
original RESTART/RECOVER driving actions. The independent SAVE
failure warning still does not depend on page contents.

```sh
ctest --test-dir Tests/Standalone/build -C Release -R '^(MenuNavigation|HudMenuVerticalPlan|MenuDetailPager)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'MenuDetailPaginationBindingTests.py' -v
```

See [real runtime paging QA](../../docs/MENU_DETAIL_PAGING_QA.md).
Source validation is not actual UE5.8 UHT/Editor build or phone QA.
