# Restart-grid placement verification — v0.5.50 source development

> **Not a playable or installed-device qualification.** This feature has
> source-level regression coverage only. Genuine UE 5.8 Editor/UHT and game
> compilation, cooked circuit, Windows/Android packaged runtime and Chaos
> failure-injection QA remain outstanding.

## Why this gate exists

Before v0.5.50, `ALatebrakeRaceGameMode::RestartRace` reset the clock,
penalty history and checkpoints, then called the Blueprint-compatible
**void** pawn `RecoverTo`. That method intentionally ignores its
`TryRecoverTo` return value. A failed engine teleport could therefore
leave a healthy simulated car at its **old position** while the reset
race began timing. Physics readiness alone cannot prove it reached the
expected start grid.

The source now distinguishes an ordinary initial player spawn (which
must still pass the existing live physics readiness checks) from an
**explicit RESTART** requiring verified engine placement.

## Engine integration

- `Racing/RestartPlacementGuard.h` is an engine-free finite state
  machine: `InitialSpawn`, `AwaitingPlacement`,
  `RejectedPlacement`, `VerifiedPlacement`. Its initial state
  preserves the normal Unreal `APlayerStart` path.
- After career replay and save checks succeed, the GameMode calls
  `OnRestartRequested()` **before** race reset, engine unpause or
  physical relocation. Previously valid grid authorization expires.
- `ALatebrakeCarPawn::TryRecoverTo(GetStartPosition(), GetStartRotation())`
  applies the existing production teleport verifier to the actual
  position/rotation, post-move body and residual motion. Missing pawns
  and unverified moves report failure. Only the corresponding pending
  attempt may be acknowledged.
- `HasLocalPlayerDriver()` requires both a verified start-placement
  state and live chassis readiness. The existing authoritative clock,
  ordered-gate scoring, paid recovery and rival drive conditions use
  the shared local-player-readiness boundary.
- The player's **and rival's shared base** `ALatebrakeCarPawn::Tick`
  also passes the new placement bit to `CanApplyPlayerDrive`.
  On failure it neutralizes commands and motion before any thrust or
  yaw torque, including cases where Unreal remains unpaused.
- The HUD shows a distinct restart-grid warning instead of misleadingly
  reporting only an ordinary unavailable physics chassis.

A rejection is **latched**. Menu close, focus regain or a late/duplicate
callback cannot silently restore racing. Another explicit RESTART may
re-attempt the placement. The game does not apply a player recovery
time fee for resetting to the initial grid.

This safeguard **does not claim** that an unsuccessfully moved body
was automatically rolled back, is road-safe, or has been physically
recovered; it only refuses further scoring/driving authorization.

## v0.5.51 correction — paid RECOVER and omitted drive permission

A post-v0.5.50 audit revealed that the actual player `RecoverPlayer()`
callsite still passed **only** `Car->IsRacePhysicsReady()` into the
staged penalty/teleport transaction. That contradicted the intended
single authoritative `HasLocalPlayerDriver()` boundary. A rejected
restart does not necessarily make Chaos physics invalid: a healthy
body remaining at its **old** position was enough to attempt a paid
recovery despite frozen timing and checkpoint scoring.

The Unreal GameMode now passes `HasLocalPlayerDriver()` to
`TryChargePlayerRecoveryAfterVerifiedTeleport`. The same check
explicitly requires **both** a ready live chassis and a verified
restart placement. Failed attempts cannot invoke the engine
teleport callback, count a recovery, or charge a penalty.

The seven-field `PlayerDriveConditions` aggregate additionally
defaults `RestartPlacementReady` to **false**, not true. A future
caller forgetting the seventh argument fails closed; all legitimate
Unreal/portable fixtures explicitly provide that bit. The real player
pawn already forwards `Mode->IsRestartPlacementReady()`.

Required QA additions:

1. With a physically healthy but wrongly positioned car, induce
   a rejected RESTART and press **RECOVER**. Confirm neither the
   five-second fee nor the engine teleport occurs and clock/gates
   remain denied.
2. Explicitly RESTART again with a **verified** grid placement and
   confirm a subsequent paid RECOVER succeeds exactly once, with the
   unchanged normal five-second fee.
3. Build a new C++ callsite that omits the seventh driving-policy
   field. Confirm it is **denied**, rather than silently allowing
   acceleration after an unverified restart. Check all 128 seven-bit
   combinations and manual pause/resume behavior.
4. Repeat on installed Win64 and Android with real Chaos telemetry.
   Pure policy tests do not prove actual movement failure can be
   induced safely on a real vehicle.

## v0.5.52 — verified grid authorization belongs to one pawn

A verified `RESTART` previously authorized **any later possessed pawn**
with apparently healthy physics. This matters when an editor, respawn
manager, game mode or future vehicle-swapping flow destroys/replaces the
player actor after grid placement: the new actor did not actually pass
the old `TryRecoverTo` check, but could still race.

The actual Unreal GameMode now holds a `TWeakObjectPtr` to the exact
pawn instance that passed verified restart placement. The
engine-neutral `RestartPlacementGuard::CanRace(bool isSameVerifiedPawn)`
requires identity match **only after an explicit verified restart**;
normal initial APlayerStart spawning retains its existing readiness
contract. Unreal weak identity also prevents a destroyed UObject's
recycled raw address from being treated as the same live car.

On a subsequent RESTART, the old weak handle is invalidated **before**
any teleport. On successful placement the newly verified pawn is bound;
on failure no handle is authorized. Gate scoring, player race clock,
the staged five-second player recovery, rival eligibility, motor force
and the existing HUD warning all share the check.

Extend the real UE and device acceptance matrix:

1. Complete a verified RESTART, then replace the possessed pawn through
   development-only actor replacement. Ensure the new pawn is physically
   ready but has not completed its own grid teleport.
2. Confirm **no** clock increment, ordered checkpoint, player RECOVER
   teleport/fee, autonomous rival progress or motor force on the new
   pawn. The UI must warn that restart authorization is unavailable.
3. Destroy the original verified pawn; spawn another actor and attempt
   to occupy the same memory slot in a controlled test. Weak UObject
   identity must reject the new generation regardless of pointer reuse.
4. Re-possess the original still-live verified pawn (where supported);
   verify its own authorization is retained and no unrelated pawn
   inherits it. Attempt a new explicit RESTART on a replacement:
   only a verified grid teleport may restore normal play.
5. Repeat at safe installed Win64 and Android test setups with a
   legitimate paused/foreground transition, hold-release touch inputs
   and fresh checkpoint samples; retain engine/version/candidate hashes.

These are **requirements**, not claims that Unreal/UHT/Chaos or real
installed-device actor replacement has been tested.

## Portable test procedures

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(RestartPlacementGuard|PlayerDriveReadiness|RecoveryTeleportTransaction)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

`RestartPlacementGuardTests.cpp` links the **real** production
`RaceProgress.cpp` and verifies initial-spawn eligibility, pending/
rejected blocking, no phantom elapsed time, checkpoint denial, paid
recovery denial, repeated/unexpected outcome rejection and fresh
verified-retry eligibility. The portable drive matrix covers the new
seventh readiness bit independently. Python contracts assert that the
**actual** Unreal restart and pawn motor paths call the shared rules.
Passing these tests does **not** compile Unreal.

## Required live-engine acceptance

1. Compile `LatebrakeEditor` and the relevant game targets using a real
   authorized UE 5.8 workstation. Record engine build and source hashes.
2. Build a real cookable circuit, package a Win64 candidate, test a
   regular successful RESTART from a running race and after a finished
   result; confirm position, yaw, existing five-second paid RECOVER
   penalty logic, clock reset, and ordered-gate rebase.
3. With a **development-only** test double for engine movement, force a
   rejected `SetActorLocationAndRotation`, nonfinite/misaligned
   post-move pose and disabled post-move chassis. Confirm frozen clock,
   rejected gate awards, zero driving torque, blocked paid recoveries,
   visible warning and no credit/medal/save contamination.
4. From rejected state, close the menu, foreground the app and resume
   world updates; the latch must remain blocking. Trigger a fresh
   RESTART with a valid placement: check normal inputs and timing resume.
5. Repeat on installed Android with touch steering, accelerator, BRAKE,
   manual pause, orientation change and resumed application, including
   focus interruption while the restart placement is rejected.
6. Confirm the AI remains stopped while no local verified restart exists.
   Exercise successful and failed rival recovery telemetry separately.
7. Attach device/package SHA256, screenshots, logs and gameplay results
   to [Android QA issue #13](https://github.com/sanskarIN/latebrake/issues/13).
   Do not promote a stable/playable release until the full gate passes.
