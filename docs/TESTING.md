# Test plan

## Executable automated checks

Portable race rules: `cmake -S Tests/Standalone -B Tests/Standalone/build && cmake --build Tests/Standalone/build && ctest --test-dir Tests/Standalone/build --output-on-failure`.

The suite checks checkpoint ordering, duplicates, invalid indices, lap count, finish behavior, timer safety (including NaN/infinite deltas), frozen finish clock, restart/reset and invalid configuration clamping.

GitHub Actions matrix: Ubuntu, Windows and macOS. This validates portable C++ source, **not Unreal packaging**.

## Unreal integration test checklist (manual until installed)

- [ ] Unreal Header Tool generates classes, editor target builds without warnings/errors
- [ ] Correct default pawn appears at south straight; camera follows without distortions
- [ ] Starter car responds to W/S, A/D, space, R, T, C and handling mode buttons
- [ ] Pause/resume does not leave stuck throttle/brake input
- [ ] Car rests on visible collidable road; barriers do not block checkpoint circuit
- [ ] Only expected gate increments progress; gates cannot be bypassed/repeated for credit
- [ ] Three completed laps show finish, freeze timer and update best race
- [ ] Restart resets timer/gates without corrupting previous best
- [ ] Relaunch restores personal best and completed-races count
- [ ] Empty, corrupt and unwritable save slots are handled without crash
- [ ] Inputs/camera/UI are readable on supported desktop aspect ratios
- [ ] Packaged Windows build launches and can complete a race
- [ ] Android touch/control implementation and real-device test pass
- [ ] At least one release packaging workflow produces verifiable artifacts

Track the engine version, machine, GPU, driver, device temperatures, resolution, frame times, loading time, memory, and crash logs for every qualification.

## Observed CI result

As of 2026-10-19, commit `f8723f7937faf9503734719f9a1aeaacf640727b` passed the Ubuntu, Windows and macOS portable C++20 jobs: https://github.com/sanskarIN/latebrake/actions/runs/39221329905 . Later commits require their own verification.

## v0.1.1 regressions

Tests now cover current/last/best lap, sector splits, a slower finish that does not overwrite the fastest lap, invalid checkpoint sequences, timer overflow, invalid directional crossings and minimum speed checks. Unreal-specific QA must test the actual car overlap velocity, save migration, HUD layout and lap recording.

Recovery penalty assertions verify positive additions, rejection of NaN/infinity/negative durations and frozen finish clock behavior. In Unreal, verify that a manual recovery adds five seconds, does not count a skipped gate, and that restart clears the penalty along with the run.

## v0.2.0 portable and engine qualification

Additional CTest targets: RivalPilot (heading/turn, overspeed braking, wraparound, invalid inputs) and RaceRewards (event thresholds, clean recovery bonus, invalid inputs), along with the existing lap and event catalog tests.

Unreal-specific checklist:
- [ ] Compile new `ALatebrakeRivalCar` reflection class in UE 5.8 and validate actor initialization
- [ ] Circuit mode spawns exactly one rival and restart reinitializes its pilot
- [ ] Time Attack mode removes the rival and resets to one lap
- [ ] Keys 3 and 4 select modes with independent best times
- [ ] Rival overlaps never advance player checkpoints or trigger player results
- [ ] Player gate order and required directional speed still work
- [ ] Credits awarded once, survive save/reload and ignore unearned wins
- [ ] Android viewport labels, gamepad and mobile touch are tested on real devices

## v0.2.1 additional regression coverage

The portable RivalRoute suite checks waypoint wrapping, approximate lap completion, invalid distance/speed/delta rejection, stuck recovery after 8 seconds, movement reset of the stuck timer, full finish lockout and route reset. SteeringFilter tests cover bounded steering steps, reversals, invalid frame deltas, out-of-range steering inputs, NaN, overflow and convergence. RivalPilot additionally tests early corner braking and negative speed rejection.

Unreal validation still required:
- [ ] Rival consumes planned commands before base physics and recovers without resetting its route
- [ ] Stuck detection works near barriers, with no oscillating recovery loop
- [ ] Rival approximate lap display never claims an authoritative finish position
- [ ] New steering responsiveness is driveable at varied frame rates on real devices

## v0.3.0 garage qualification

The portable `GarageState` target tests vehicle ownership and purchase costs, preventing repeat charges, insufficient credit handling, bounded balance, upgrade-level caps, invalid vehicle IDs, invalid saved fields, snapshot restoration and the fictional preset cabin dimensions. The existing five portable test targets must still pass.

Unreal integration required:
- [ ] Starter player pawn receives garage performance modifiers without affecting the AI rival
- [ ] Three garage silhouettes and tuning differences render and feel correct in editor
- [ ] Q/E/U/I correctly unlock, select and upgrade only with earned credits
- [ ] Garage SaveGame fields survive save/restart/reload, including older v0.2.x slots
- [ ] HUD is readable at 720p, 1080p, ultrawide and Android sizes
- [ ] Win64/Android real game packages can launch, race and preserve garage progression

## v0.3.1 regression tests

Portable `HudLayout` checks standard/compact screen dimensions and invalid viewport values; `RecordValidation` checks invalid saved times, NaN/infinity, negative race totals and saturating record counters; expanded `GarageState` covers negative ownership masks and model-defined purchase/upgrading quotes.

Additional Unreal playtest gates:
- [ ] Garage feedback text visible and correct for successful/unaffordable transactions.
- [ ] Q/E/U/I do not charge any credits for invalid operations; test with both fresh and old profiles.
- [ ] HUD readable on desktop, portrait/narrow screens and Android without overlap or clipping.
- [ ] Deserialize corrupted/old SaveGame values and confirm recovered safe progression.
- [ ] Existing race, rival, touch and car movement remain functional after v0.3.1 changes.

## v0.3.2 verification and in-engine regression gates

Portable `DriverInputSafety` exercises keyboard/AI axis sanitation, invalid viewport/touch coordinates, throttle-rise limiting, reversal limiting, invalid frame deltas and overflow cases. `GarageTuningKey` asserts the live-physics key changes for selection/upgrade but not for credit or purchase balance updates. Cross-platform CI compiles these exact header-only C++ routines.

Engine tests remain required:
- [ ] Unreal 5.8 project/game targets compile after cache, input and save changes
- [ ] Keyboard/gamepad/touch controls recover from pause without held acceleration/braking
- [ ] Verify smoothed throttle remains responsive in Arcade and simulation-inspired modes
- [ ] Validate fatal/nonfinite simulated motion recovery without crashes
- [ ] Ensure every selection or upgrade changes player tuning exactly once, including respawn
- [ ] Test failed/unwritable SaveGame slots to confirm the warning appears and clears after success
- [ ] Build real Win64 and Android archives and test physics, race events and saves on hardware

## v0.4.0 course and environment checks

New `CircuitLayout` portable test target checks shared four-gate geometry, road coverage, legal recovery positions, gate driving direction and invalid indexes; `TrackDecorations` verifies the deterministic lane/curb arrays stay on the road with the two grandstands outside it; `CircuitMap` verifies normalized minimap projection, viewport bound clamping and invalid-value handling.

Unreal Editor and physical performance gates:
- [ ] Launch UE 5.8 starter course and ensure asset references, lighting, collision, road surface and gates function
- [ ] Drive in circuit and time attack; ensure gate positions and recovery orientations align with visible roads and AI route
- [ ] Confirm decoration meshes have NO physics collision and never block a vehicle or compromise performance
- [ ] Verify no floating/overlapping grandstands, curbs, stripes or start banner from driving camera
- [ ] Test minimap next checkpoint, player marker and HUD collision at 1080p, ultrawide and compact sizes
- [ ] Measure draw calls, FPS and memory on Windows and Android; batch decorative actors into instanced meshes if needed
- [ ] Produce and test real packaged games before claiming gameplay or stable release readiness

## v0.5.0 career and menu regression matrix

Portable C++20 checks now cover:
- Immutable stable event identifiers, rounds and prerequisite ordering.
- Time, clean, checkpoint-precision, consistency, required-vehicle, and
  endurance medal thresholds with invalid-value rejection.
- Career unlock/replay, medal upgrades, championship points and once-only
  completion/series bonuses after save/reload.
- Legacy/unknown schema treatment, invalid medal and orphan unlock handling.
- Slowest-lap tracking and reset for consistency objectives.
- Paused-menu navigation state, invalid page rejection, touch hitboxes.

Mandatory **unverified** Unreal/device checks include UHT reflection,
`FCoreDelegates` lifecycle registration, `BindTouch` while paused,
real level cooking, career action routing, HUD viewport/safe-area overlap,
multi-finger interruption recovery, SaveGame v2 round-trips, and installed
Android/Win64 gameplay/performance. No CTest run can close these gates.

## Engine-free packaging regression coverage (v0.5.1 development)

On ordinary GitHub runners, Windows PowerShell checks the syntax of all
authoring, compilation, packaging and verification scripts. The
`Test-PackagingPolicy.ps1` suite deliberately generates temporary,
invalid EXE/APK fixtures, checks they are rejected, then deletes them;
**no synthetic executable is uploaded**. An editor-only Python source
syntax check compiles `CreateStarterMap.py` without running Unreal.
The existing C++20 CMake test suite still runs on all three OS runners.

These checks do not compile the Unreal game or serialize a real `.umap`.
The first real Windows and Android build must satisfy
[the installed gameplay matrix](EXECUTABLE_BUILD_PIPELINE.md), with
truthful platform-specific test evidence and package hashes.

## Additional v0.5.1 automation checks

The `Tests/Editor/UnrealMapScriptTests.py` suite exercises seven
**fake-engine-only** cases: new level, existing level preservation, failed
load, failed creation, failed save, undersized output and missing editor
subsystem. Synthetic map fixtures exist only under deleted temporary test
directories; no fixture is a real Unreal asset.

The PowerShell packaging policy now rejects renamed random bytes or
unpulled Git LFS pointers as maps, and refuses to write a candidate manifest
without real package/engine prerequisites. Package output must start empty
to prevent stale EXE/APK false positives. These are **source-level
safeguards, not a licensed UE Editor, UHT or installed platform run**.

Pending hardware regression: when pause/menu is open, Escape, gamepad Pause
and Android pause/close must return to driving *without* resuming behind
the menu. Verify with an actual Unreal executable.

## New executable preflight safeguards (unreleased)

- An editor Python script writes a **fresh success receipt** under `Saved/`
  only after a genuine loaded/created level has been saved; the PowerShell
  launcher clears any stale receipt before invocation and validates
  map/package name plus saved file length. Offline stub tests verify
  successful receipts and the absence of receipts on failed saves.
- A C++ GameMode startup marker logs the number of checkpoint actors
  instantiated by the runtime-built circuit.
  `scripts/Verify-RuntimeStartupLog.ps1` accepts exactly one
  `LATEBRAKE_STARTUP_READY` with four gates from a **real packaged game
  log**. CI includes only negative fixtures (missing/garbage logs) so it
  cannot falsely claim a genuine runnable game.
- The dedicated Android PlayerController configures touch-event delivery
  and disables stock virtual joysticks, pending actual UE compilation and
  physical two-finger QA.

## Unreleased v0.5.1 — failed SaveGame write regression

The new portable C++20 `ProfileWriteCheckpoint` tests exercise the
same garage/career/record state machine used by the Unreal GameMode.
They cover failed-save rollback of credits, medals, unlocked rounds,
completed races, race and lap records, car purchases and tuning,
successful write commit, repeated rollback idempotency, scope-exit
fallback, retry after failure and a one-time championship bonus after
an unsaved final round.

The actual `ALatebrakeRaceGameMode` adapter now snapshots state before
race awards and rolls it back when `SaveGameToSlot` reports failure.
Garage vehicle selection, unlocking, engine/grip spending and the speed
unit toggle also revert unsaved changes and show a warning.

**Still requires Unreal/device validation:** force a genuine local
SaveGame failure (for example unavailable storage or app sandbox
write denial), finish both a quick race and a career round, verify
credits and medals revert, restore storage, retry the round, and verify
the one-time reward is actually persisted after reloading the game.
Test garage purchases/upgrades, the settings toggle, app interruption
and an already-existing save profile. Source-level tests cannot prove
the engine's filesystem durability or handle power-loss corruption.

## v0.5.1 portable multi-touch driving state tests

A new `TouchDriveState` C++20 test target runs in the existing three-OS
portable CI matrix. It checks two independent held fingers for steering
and gas, gas-to-brake drag with an immediate neutral gap, bounded steering
beyond horizontal pad edges, neutral steering after leaving its vertical
band, ownership isolation from extra fingers, cancellation of NaN/Infinity
and invalid viewport geometry, release/cancel idempotency, and suppression
of moves after lifecycle/menu reset. Dragging an already captured finger
over PAUSE is **not** a discrete press and must not activate UI commands.

The pawn now uses this state machine for HUD highlights and physics input,
with a standalone-pause gate that rejects pedal and steering presses while
paused. These tests run without Unreal Engine; the C++ Unreal adapter and
its actual Android touch events are **not** compiled by these checks.

An actual UE 5.8 APK must be installed on a phone to validate simultaneous
touches, pad coordinates, pause/resume, app focus, and orientation. Save
screenshots/videos and performance data only from real playtests.
